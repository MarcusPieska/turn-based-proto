//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <errno.h>
#include <sys/stat.h>

#include "city.h"
#include "city_array.h"
#include "city_blocking_mask.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "generator_constants.h"
#include "map_loader.h"
#include "sense_settling_pts_opt.h"
#include "starting_point_generator.h"
#include "whiteboard_mng.h" 

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* g_map_root = "/home/w/Projects/simple-map-gen/p1-seed-42";
static const char* g_out_base = "/home/w/Projects/simple-map-gen/ai-settle-pts";

static const u16 k_iter_n = 100;
static const u16 k_iter_m = 200;
static const u32 k_samp_cap = static_cast<u32>(k_iter_n) * static_cast<u32>(k_iter_m);

static const u16 k_latt_div = 10;
static const i16 k_mask_rad = 2;

int total_test_fails = 0;

//================================================================================================================================
//=> - ChronoFn -
//================================================================================================================================

struct ChronoFn {
    const char* name;
    u64* samp;
    u32 n;
    u64 total_ns;
};

static ChronoFn g_tm_sel = {"SenseSettlingPtsOpt::select_and_pick_pts", nullptr, 0, 0};

//================================================================================================================================
//=> - IterPlayer -
//================================================================================================================================

struct IterPlayer {
    CityArray cities;
};

//================================================================================================================================
//=> - IterFrame -
//================================================================================================================================

struct IterFrame {
    SmSettlerPt pick[k_iter_n];
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void latt_for_map (u16 w, u16 h, u16* rows, u16* cols) {
    u16 r = h / k_latt_div;
    u16 c = w / k_latt_div;
    if (r == 0) {
        r = 1;
    }
    if (c == 0) {
        c = 1;
    }
    *rows = r;
    *cols = c;
}

static u32 tile_idx (u16 w, u16 px, u16 py) {
    return static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_mtn (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool can_claim (u8 cls) {
    return !is_water(cls) && !is_mtn(cls);
}

static void player_rgb (u32 idx, u32 n, u8* r, u8* g, u8* b) {
    const f32 hue = (360.0f * static_cast<f32>(idx)) / static_cast<f32>(n);
    const f32 s = 0.85f;
    const f32 v = 0.90f;
    const f32 c = v * s;
    const f32 x = c * (1.0f - std::fabs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
    const f32 m = v - c;
    f32 rp = 0.0f;
    f32 gp = 0.0f;
    f32 bp = 0.0f;
    if (hue < 60.0f) {
        rp = c;
        gp = x;
    } else if (hue < 120.0f) {
        rp = x;
        gp = c;
    } else if (hue < 180.0f) {
        gp = c;
        bp = x;
    } else if (hue < 240.0f) {
        gp = x;
        bp = c;
    } else if (hue < 300.0f) {
        rp = x;
        bp = c;
    } else {
        rp = c;
        bp = x;
    }
    *r = static_cast<u8>((rp + m) * 255.0f);
    *g = static_cast<u8>((gp + m) * 255.0f);
    *b = static_cast<u8>((bp + m) * 255.0f);
}

static void set_px_rgb (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void apply_claim_mask (
    GameArraySimple& gmap,
    const u8* cls,
    u16 w,
    u16 h,
    u16 cx,
    u16 cy,
    u8 player)
{
    for (i16 dy = -k_mask_rad; dy <= k_mask_rad; ++dy) {
        for (i16 dx = -k_mask_rad; dx <= k_mask_rad; ++dx) {
            const i32 tx = static_cast<i32>(cx) + static_cast<i32>(dx);
            const i32 ty = static_cast<i32>(cy) + static_cast<i32>(dy);
            if (tx < 0 || ty < 0 || tx >= static_cast<i32>(w) || ty >= static_cast<i32>(h)) {
                continue;
            }
            const u16 px = static_cast<u16>(tx);
            const u16 py = static_cast<u16>(ty);
            const u32 i = tile_idx(w, px, py);
            if (gmap.get_civ_owner(px, py) != U8_KEY_NULL) {
                continue;
            }
            if (!can_claim(cls[i])) {
                continue;
            }
            gmap.set_civ_owner(px, py, player);
        }
    }
}

static bool ensure_dir (cstr path) {
    if (path == nullptr) {
        return false;
    }
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
}

static double secs_since (const std::chrono::steady_clock::time_point& t0) {
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}

static void tm_add (ChronoFn* t, u64 ns) {
    if (t == nullptr || t->samp == nullptr || t->n >= k_samp_cap) {
        return;
    }
    t->samp[t->n] = ns;
    t->n = t->n + 1u;
    t->total_ns = t->total_ns + ns;
}

static void tm_report (const ChronoFn& t) {
    const double total_us = static_cast<double>(t.total_ns) / 1.0e3;
    const double avg_us = (t.n == 0) ? 0.0 : total_us / static_cast<double>(t.n);
    std::printf("  %-40s calls=%u  total=%.2f us  avg=%.2f us\n", t.name, t.n, total_us, avg_us);
}

static bool tm_write (cstr path, const ChronoFn& a) {
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "%s", a.name);
    for (u32 i = 0; i < a.n; ++i) {
        const double us = static_cast<double>(a.samp[i]) / 1.0e3;
        std::fprintf(fp, " %.2f", us);
    }
    std::fprintf(fp, "\n");
    std::fclose(fp);
    return true;
}

static bool save_iter_ppm (
    cstr path,
    const GameArraySimple& gmap,
    u32 player_n,
    const IterPlayer* players,
    const IterFrame& frame)
{
    if (path == nullptr || players == nullptr) {
        return false;
    }
    const u16 w = gmap.width();
    const u16 h = gmap.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(gmap.get_climate(x, y), &r, &g, &b);
            if (gmap.get_river(x, y) != 0) {
                r = 40;
                g = 100;
                b = 220;
            }
            if (gmap.get_terrain(x, y) == TERR_MOUNTAINS[0]) {
                r = 120;
                g = 72;
                b = 40;
            }
            const u8 own = gmap.get_civ_owner(x, y);
            if (own != U8_KEY_NULL && static_cast<u32>(own) <= player_n
                && gmap.get_river(x, y) == 0 && gmap.get_terrain(x, y) != TERR_MOUNTAINS[0]) {
                u8 pr = 0;
                u8 pg = 0;
                u8 pb = 0;
                player_rgb(static_cast<u32>(own) - 1u, player_n, &pr, &pg, &pb);
                r = static_cast<u8>((static_cast<u16>(r) + static_cast<u16>(pr)) / 2u);
                g = static_cast<u8>((static_cast<u16>(g) + static_cast<u16>(pg)) / 2u);
                b = static_cast<u8>((static_cast<u16>(b) + static_cast<u16>(pb)) / 2u);
            }
            set_px_rgb(rgb, w, h, x, y, r, g, b);
        }
    }
    for (u32 p = 0; p < player_n; ++p) {
        const u16 cn = players[p].cities.get_city_count();
        for (u16 i = 0; i < cn; ++i) {
            const City* c = players[p].cities.get_city(i);
            if (c == nullptr) {
                continue;
            }
            set_px_rgb(rgb, w, h, c->get_x(), c->get_y(), 0, 0, 0);
        }
    }
    for (u32 p = 0; p < player_n; ++p) {
        if (frame.pick[p].x != U16_KEY_NULL) {
            set_px_rgb(rgb, w, h, frame.pick[p].x, frame.pick[p].y, 255, 255, 255);
        }
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main () {
    char terr_path[320];
    char clim_path[320];
    char riv_path[320];
    if (std::snprintf(terr_path, sizeof(terr_path), "%s/terrain.ppm", g_map_root) <= 0
        || std::snprintf(clim_path, sizeof(clim_path), "%s/climate.ppm", g_map_root) <= 0
        || std::snprintf(riv_path, sizeof(riv_path), "%s/rivers.ppm", g_map_root) <= 0) {
        std::printf("*** FAILED path build\n");
        return 1;
    }

    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(terr_path, map)) {
        std::printf("*** FAILED load map: %s\n", terr_path);
        return 1;
    }

    GameArraySimple gmap;
    if (!Factory_GameArraySimple::load_map_gen_data(&gmap, terr_path, clim_path, riv_path, nullptr)) {
        std::printf("*** FAILED load GameArraySimple\n");
        return 1;
    }

    WhiteboardMng::init(gmap.width(), gmap.height());

    u16 latt_rows = 0;
    u16 latt_cols = 0;
    latt_for_map(map.width(), map.height(), &latt_rows, &latt_cols);

    StartingPointGeneratorParams par = {};
    par.map = &map;
    par.pick_n = k_iter_n;
    par.latt_rows = latt_rows;
    par.latt_cols = latt_cols;

    StartingPointGenerator gen(par);
    if (!gen.generate() || !gen.picks_are_start_land()) {
        std::printf("*** FAILED generate starting points\n");
        WhiteboardMng::terminate();
        gmap.clear();
        return 1;
    }

    SpgPickCoords starts = gen.picks_coords();
    if (starts.n != static_cast<u32>(k_iter_n)) {
        std::printf("*** FAILED start count: got %u expected %u\n", starts.n, k_iter_n);
        WhiteboardMng::terminate();
        gmap.clear();
        return 1;
    }

    const u16 w = map.width();
    const u16 h = map.height();
    g_tm_sel.samp = new u64[k_samp_cap];
    if (g_tm_sel.samp == nullptr) {
        WhiteboardMng::terminate();
        gmap.clear();
        std::printf("*** FAILED alloc\n");
        return 1;
    }

    IterPlayer* players = new IterPlayer[k_iter_n];
    if (players == nullptr) {
        delete[] g_tm_sel.samp;
        WhiteboardMng::terminate();
        gmap.clear();
        std::printf("*** FAILED players alloc\n");
        return 1;
    }

    for (u16 p = 0; p < k_iter_n; ++p) {
        const u16 idx = players[p].cities.get_next_new_city_idx();
        City* c = players[p].cities.get_city(idx);
        if (c == nullptr) {
            total_test_fails += 1;
            continue;
        }
        c->init(static_cast<u16>(p + 1u), starts.pts[p].x, starts.pts[p].y);
        apply_claim_mask(gmap, map.data(), w, h, starts.pts[p].x, starts.pts[p].y, static_cast<u8>(p + 1u));
        CityBlockingMask::stamp(gmap, starts.pts[p].x, starts.pts[p].y);
    }

    char out_dir[512];
    char path[640];
    std::snprintf(out_dir, sizeof(out_dir), "%s/iter-sso-m%u-n%u", g_out_base, k_iter_m, k_iter_n);
    if (!ensure_dir(g_out_base) || !ensure_dir(out_dir)) {
        delete[] players;
        delete[] g_tm_sel.samp;
        WhiteboardMng::terminate();
        gmap.clear();
        std::printf("*** FAILED output dir: %s\n", out_dir);
        return 1;
    }

    for (u16 m = 0; m < k_iter_m; ++m) {
        const auto t0 = std::chrono::steady_clock::now();
        IterFrame frame = {};

        for (u16 p = 0; p < k_iter_n; ++p) {
            const u16 player = static_cast<u16>(p + 1u);
            const auto ts0 = std::chrono::steady_clock::now();
            const SmSettlerBestPts best = SenseSettlingPtsOpt::select_and_pick_pts(gmap, players[p].cities, player);
            const auto ts1 = std::chrono::steady_clock::now();
            tm_add(&g_tm_sel, static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(ts1 - ts0).count()));

            frame.pick[p] = best.n > 0 ? best.pts[0] : SmSettlerPt{U16_KEY_NULL, U16_KEY_NULL};
            if (frame.pick[p].x == U16_KEY_NULL) {
                continue;
            }
            const u16 idx = players[p].cities.get_next_new_city_idx();
            City* c = players[p].cities.get_city(idx);
            if (c == nullptr) {
                total_test_fails += 1;
                continue;
            }
            c->init(player, frame.pick[p].x, frame.pick[p].y);
            apply_claim_mask(gmap, map.data(), w, h, frame.pick[p].x, frame.pick[p].y, static_cast<u8>(player));
            CityBlockingMask::stamp(gmap, frame.pick[p].x, frame.pick[p].y);
        }

        std::snprintf(path, sizeof(path), "%s/%04u_sso_iter.ppm", out_dir, static_cast<u32>(m + 1u));
        if (!save_iter_ppm(path, gmap, k_iter_n, players, frame)) {
            total_test_fails += 1;
            std::printf("*** FAILED save: %s\n", path);
        }

        const double dt = secs_since(t0);
        std::printf("*** Iter %04u: %.1f ms  saved %s\n", static_cast<u32>(m + 1u), dt * 1000.0, path);
    }

    char tpath[640];
    std::snprintf(tpath, sizeof(tpath), "%s/sso_iter_timings.txt", out_dir);
    if (!tm_write(tpath, g_tm_sel)) {
        total_test_fails += 1;
        std::printf("*** FAILED write timings: %s\n", tpath);
    }

    if (WhiteboardMng::chkout() != 0) {
        total_test_fails += 1;
        std::printf("*** FAILED whiteboard leak: %u\n", WhiteboardMng::chkout());
    }

    std::printf("=======================================================\n");
    std::printf(" SSO ITER TEST FAILURES: %d\n", total_test_fails);
    std::printf(" hot-path timings (us samples in %s):\n", tpath);
    tm_report(g_tm_sel);
    std::printf("=======================================================\n");

    delete[] players;
    delete[] g_tm_sel.samp;
    WhiteboardMng::terminate();
    gmap.clear();

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
