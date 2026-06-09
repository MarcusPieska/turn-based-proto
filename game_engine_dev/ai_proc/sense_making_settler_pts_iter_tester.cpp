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

#include "generator_constants.h"
#include "map_loader.h"
#include "map_terrain_validate.h"
#include "starting_point_generator.h"
#include "generate_distance_land_to_water.h"
#include "sense_making_settler_pts.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* g_map_path = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_base = "/home/w/Projects/simple-map-gen/ai-settle-pts";

static const u16 k_iter_n = 100;
static const u16 k_iter_m = 200;

static const u16 k_latt_div = 10;
static const u16 k_min_dist = 4;
static const u16 k_max_dist = 6;
static const i16 k_mask_rad = 2;

int total_test_fails = 0;

//================================================================================================================================
//=> - IterPlayer -
//================================================================================================================================

struct IterPlayer {
    SpgCoordPair cap;
    SpgCoordPair cities[k_iter_m];
    u16 city_n;
};

//================================================================================================================================
//=> - IterFrame -
//================================================================================================================================

struct IterFrame {
    SmSettlerPtResult cand[k_iter_n];
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
    u8* own_buf,
    u16 w,
    u16 h,
    const u8* cls,
    u16 cx,
    u16 cy,
    u8 player) {
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
            if (own_buf[i] != 0) {
                continue;
            }
            if (!can_claim(cls[i])) {
                continue;
            }
            own_buf[i] = player;
        }
    }
}

static u16 build_src (
    const IterPlayer& pl,
    SpgCoordPair* src,
    u16 src_cap) {
    u16 n = 0;
    if (n < src_cap) {
        src[n] = pl.cap;
        n = static_cast<u16>(n + 1u);
    }
    for (u16 i = 0; i < pl.city_n; ++i) {
        bool dup = false;
        for (u16 j = 0; j < n; ++j) {
            if (src[j].x == pl.cities[i].x && src[j].y == pl.cities[i].y) {
                dup = true;
                break;
            }
        }
        if (dup) {
            continue;
        }
        if (n >= src_cap) {
            break;
        }
        src[n] = pl.cities[i];
        n = static_cast<u16>(n + 1u);
    }
    return n;
}

static bool save_iter_ppm (
    cstr path,
    const MapTerrainData& map,
    const u8* own_buf,
    u32 player_n,
    const IterPlayer* players,
    const IterFrame& frame) {
    if (path == nullptr || map.data() == nullptr || own_buf == nullptr || players == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        const u8 own = own_buf[i];
        if (own > 0 && static_cast<u32>(own) <= player_n) {
            player_rgb(static_cast<u32>(own) - 1u, player_n, &r, &g, &b);
        } else {
            MapTerrainValidate::rgb_from_class(map.data()[i], &r, &g, &b);
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u32 p = 0; p < player_n; ++p) {
        for (u32 i = 0; i < frame.cand[p].n; ++i) {
            set_px_rgb(rgb, w, h, frame.cand[p].pts[i].x, frame.cand[p].pts[i].y, 128, 128, 128);
        }
    }
    for (u32 p = 0; p < player_n; ++p) {
        set_px_rgb(rgb, w, h, players[p].cap.x, players[p].cap.y, 0, 0, 0);
    }
    for (u32 p = 0; p < player_n; ++p) {
        if (frame.pick[p].x != U16_KEY_NULL) {
            set_px_rgb(rgb, w, h, frame.pick[p].x, frame.pick[p].y, 0, 0, 0);
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

static bool mk_l2w (const MapTerrainData& map, Generate_DistanceLandToWater& gen) {
    if (map.data() == nullptr || map.width() == 0 || map.height() == 0) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* wl = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        const u8 cls = map.data()[i];
        if (cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]) {
            wl[i] = WL_OVERLAY_WATER_GRAY;
        } else {
            wl[i] = WL_OVERLAY_LAND_GRAY;
        }
    }
    const bool ok = gen.generate(wl, w, h);
    delete[] wl;
    return ok;
}

static i32 l2w_at (
    const MapArrayDistance& l2w,
    const MapTerrainData& map,
    u16 px,
    u16 py) {
    if (l2w.data() == nullptr || l2w.width() != map.width() || l2w.height() != map.height()) {
        return 0;
    }
    const u32 i = static_cast<u32>(py) * static_cast<u32>(map.width()) + static_cast<u32>(px);
    const u16 raw = l2w.data()[i];
    if (raw == 0xFFFFu) {
        return -1;
    }
    return static_cast<i32>(raw);
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main () {
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {
        std::printf("*** FAILED load map: %s\n", g_map_path);
        return 1;
    }

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
        return 1;
    }

    SpgPickCoords starts = gen.picks_coords();
    if (starts.n != static_cast<u32>(k_iter_n)) {
        std::printf("*** FAILED start count: got %u expected %u\n", starts.n, k_iter_n);
        return 1;
    }

    const u16 w = map.width();
    const u16 h = map.height();
    const u32 px_n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* own_buf = new u8[px_n];
    for (u32 i = 0; i < px_n; ++i) {
        own_buf[i] = 0;
    }

    MapTerrainData own;
    if (!own.assign_raw(w, h, own_buf)) {
        delete[] own_buf;
        std::printf("*** FAILED ownership map init\n");
        return 1;
    }

    IterPlayer players[k_iter_n];
    for (u16 p = 0; p < k_iter_n; ++p) {
        players[p].cap = starts.pts[p];
        players[p].city_n = 0;
        apply_claim_mask(own_buf, w, h, map.data(), players[p].cap.x, players[p].cap.y, static_cast<u8>(p + 1u));
    }

    SpgCoordPair src_buf[k_iter_m + 1u];
    char out_dir[512];
    char path[640];

    std::snprintf(out_dir, sizeof(out_dir), "%s/iter-m%u-n%u", g_out_base, k_iter_m, k_iter_n);
    if (!ensure_dir(g_out_base) || !ensure_dir(out_dir)) {
        delete[] own_buf;
        std::printf("*** FAILED output dir: %s\n", out_dir);
        return 1;
    }

    Generate_DistanceLandToWater l2w_gen(0);
    if (!mk_l2w(map, l2w_gen)) {
        delete[] own_buf;
        std::printf("*** FAILED land-to-water distance overlay\n");
        return 1;
    }

    for (u16 m = 0; m < k_iter_m; ++m) {
        const auto t0 = std::chrono::steady_clock::now();
        IterFrame frame = {};

        if (!own.assign_raw(w, h, own_buf)) {
            total_test_fails += 1;
            std::printf("*** FAILED own sync iter %u\n", static_cast<u32>(m + 1u));
            continue;
        }

        for (u16 p = 0; p < k_iter_n; ++p) {
            const u16 player = static_cast<u16>(p + 1u);
            const u16 src_n = build_src(players[p], src_buf, static_cast<u16>(k_iter_m + 1u));
            frame.cand[p] = SenseMakingSettlerPt::select(
                map,
                own,
                player,
                k_min_dist,
                k_max_dist,
                src_buf,
                src_n);
            frame.pick[p] = SenseMakingSettlerPt::pick(map, frame.cand[p], players[p].cap, l2w_gen.distance());
            if (frame.pick[p].x == U16_KEY_NULL) {
                continue;
            }
            if (players[p].city_n < k_iter_m) {
                players[p].cities[players[p].city_n].x = frame.pick[p].x;
                players[p].cities[players[p].city_n].y = frame.pick[p].y;
                players[p].city_n = static_cast<u16>(players[p].city_n + 1u);
            }
            apply_claim_mask(
                own_buf,
                w,
                h,
                map.data(),
                frame.pick[p].x,
                frame.pick[p].y,
                static_cast<u8>(player));
        }

        std::snprintf(path, sizeof(path), "%s/%04u_sms_iter.ppm", out_dir, static_cast<u32>(m + 1u));
        if (!save_iter_ppm(path, map, own_buf, k_iter_n, players, frame)) {
            total_test_fails += 1;
            std::printf("*** FAILED save: %s\n", path);
        }

        const double dt = secs_since(t0);
        if (frame.pick[0].x != U16_KEY_NULL) {
            std::printf("*** Iter %04u: %.1f ms  p0 l2w=%d  saved %s\n",
                static_cast<u32>(m + 1u),
                dt * 1000.0,
                l2w_at(l2w_gen.distance(), map, frame.pick[0].x, frame.pick[0].y),
                path);
        } else {
            std::printf("*** Iter %04u: %.1f ms  saved %s\n", static_cast<u32>(m + 1u), dt * 1000.0, path);
        }
    }

    delete[] own_buf;

    std::printf("=======================================================\n");
    std::printf(" ITER TEST FAILURES: %d\n", total_test_fails);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

