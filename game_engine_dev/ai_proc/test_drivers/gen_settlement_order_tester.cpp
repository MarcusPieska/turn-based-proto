//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "gen_settlement_order.h"
#include "gen_settlement_targets.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "starting_point_generator.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u16 G_PICK_N = 10u;
static const u16 G_LATT_DIV = 10u;
static const int G_DOT_R = 1;
static const u8 G_RIV_R = 40u;
static const u8 G_RIV_G = 100u;
static const u8 G_RIV_B = 220u;
static const u8 G_DOT_R0 = 255u;
static const u8 G_DOT_G0 = 236u;
static const u8 G_DOT_B0 = 236u;
static const u8 G_DOT_R1 = 48u;
static const u8 G_DOT_G1 = 0u;
static const u8 G_DOT_B1 = 0u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_dir[256];

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

void note_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        if (print_level > 0) {
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

static bool build_paths () {
    if (std::snprintf(g_dir, sizeof(g_dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", g_dir) <= 0) {
        return false;
    }
    return true;
}

static void latt_for_map (u16 w, u16 h, u16* rows, u16* cols) {
    u16 r = h / G_LATT_DIV;
    u16 c = w / G_LATT_DIV;
    if (r == 0) {
        r = 1;
    }
    if (c == 0) {
        c = 1;
    }
    *rows = r;
    *cols = c;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    static const u8* const k_rows[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS,
        TERR_MOUNTAINS, TERR_VOLCANO, TERR_INLAND_SEA, TERR_INLAND_LAKE};
    for (unsigned i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == cls) {
            *r = k_rows[i][1];
            *g = k_rows[i][2];
            *b = k_rows[i][3];
            return;
        }
    }
    *r = 0;
    *g = 0;
    *b = 0;
}

static void fill_terr_riv (std::vector<u8>& rgb, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(map.get_terrain(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0u) {
                r = G_RIV_R;
                g = G_RIV_G;
                b = G_RIV_B;
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
}

static u8 lerp_u8 (u8 a, u8 b, u32 i, u32 n) {
    if (n <= 1u) {
        return a;
    }
    const u32 t = i;
    const u32 d = n - 1u;
    return static_cast<u8>((static_cast<u32>(a) * (d - t) + static_cast<u32>(b) * t) / d);
}

static void paint_dot (std::vector<u8>& rgb, u16 w, u16 h, u16 cx, u16 cy, u8 r, u8 g, u8 b) {
    for (int dy = -G_DOT_R; dy <= G_DOT_R; ++dy) {
        for (int dx = -G_DOT_R; dx <= G_DOT_R; ++dx) {
            const int x = static_cast<int>(cx) + dx;
            const int y = static_cast<int>(cy) + dy;
            if (x < 0 || y < 0 || x >= static_cast<int>(w) || y >= static_cast<int>(h)) {
                continue;
            }
            const u32 i = (static_cast<u32>(y) * w + static_cast<u32>(x)) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
}

static bool write_ppm (cstr path, const std::vector<u8>& rgb, u16 w, u16 h) {
    FILE* f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    const size_t nbytes = rgb.size();
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, f) == nbytes;
    std::fclose(f);
    return ok;
}

static bool write_order_ppm (cstr path, const GameArraySimple& map, const GenSettlementOrder& ord, u16 p) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = ord.n(p);
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    fill_terr_riv(rgb, map);
    for (u32 i = 0; i < n; ++i) {
        const SpgCoordPair pt = ord.at(p, i);
        const u8 r = lerp_u8(G_DOT_R0, G_DOT_R1, i, n);
        const u8 g = lerp_u8(G_DOT_G0, G_DOT_G1, i, n);
        const u8 b = lerp_u8(G_DOT_B0, G_DOT_B1, i, n);
        paint_dot(rgb, w, h, pt.x, pt.y, r, g, b);
    }
    return write_ppm(path, rgb, w, h);
}

static u32 planned_n (const GameArraySimple& map) {
    u32 n = 0;
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            if (map.get_planned_city(x, y) != 0u) {
                ++n;
            }
        }
    }
    return n;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    note_result(build_paths(), "build map paths");

    MapTerrainData terr;
    note_result(MapLoader::load_terrain_ppm(g_terr, terr), "load terrain");
    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");

    const u16 mw = map.width();
    const u16 mh = map.height();
    const u32 tn = map.tile_n();
    std::vector<u8> clim(tn);
    std::vector<u8> ov(tn);
    std::vector<u8> riv(tn);
    for (u16 y = 0; y < mh; ++y) {
        for (u16 x = 0; x < mw; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(mw) + static_cast<u32>(x);
            clim[i] = map.get_climate(x, y);
            ov[i] = map.get_overlay(x, y);
            riv[i] = map.get_river(x, y);
        }
    }
    u16 latt_r = 0;
    u16 latt_c = 0;
    latt_for_map(mw, mh, &latt_r, &latt_c);
    StartingPointGeneratorParams par = {};
    par.map = &terr;
    par.climate = clim.data();
    par.overlay = ov.data();
    par.river = riv.data();
    par.pick_n = G_PICK_N;
    par.latt_rows = latt_r;
    par.latt_cols = latt_c;
    par.seed = G_SEED;
    StartingPointGenerator spg(par);
    note_result(spg.generate(), "generate starting points");
    const SpgPickCoords starts = spg.picks_coords();
    note_result(starts.n > 0u, "have starting points");
    std::printf("starts=%u map=%ux%u\n", starts.n, map.width(), map.height());

    GenSettlementTargetsRslt rslt = {};
    note_result(GenSettlementTargets::generate(map, starts.pts, starts.n, &rslt), "GenSettlementTargets::generate");
    const u32 plan_n = planned_n(map);
    std::printf("planned total=%u seed=%u river=%u pack=%u\n", rslt.m_n, rslt.m_seed_n, rslt.m_river_n, rslt.m_pack_n);
    note_result(plan_n == rslt.m_n && plan_n > 0u, "planned sites marked");

    WhiteboardMng::init(mw, mh);
    note_result(WhiteboardMng::width() == mw && WhiteboardMng::height() == mh, "WhiteboardMng::init");

    GenSettlementOrder ord;
    const auto t0 = std::chrono::steady_clock::now();
    const bool excl_ok = ord.gen_excl(map, starts.pts, starts.n);
    const auto t1 = std::chrono::steady_clock::now();
    const double excl_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    note_result(excl_ok, "GenSettlementOrder::gen_excl");
    note_result(ord.ok(), "excl whiteboards ok");
    note_result(ord.pn() == static_cast<u16>(starts.n), "excl player_n == starts");
    std::printf("excl wall_ms=%.3f pn=%u\n", excl_ms, ord.pn());

    std::vector<u8> claimed(tn, 0u);
    u32 excl_sum = 0;
    bool uniq = true;
    bool hd_tl = true;
    bool nonempty = true;
    for (u16 p = 0; p < ord.pn(); ++p) {
        const u32 n = ord.n(p);
        excl_sum += n;
        if (n == 0u) {
            nonempty = false;
        }
        if (ord.hd(p) != ord.st(p) || ord.tl(p) != ord.en(p)) {
            hd_tl = false;
        }
        std::printf(
            "excl p=%u n=%u st=%u en=%u hd=%u tl=%u\n",
            p,
            n,
            ord.st(p),
            ord.en(p),
            ord.hd(p),
            ord.tl(p));
        for (u32 i = 0; i < n; ++i) {
            const SpgCoordPair pt = ord.at(p, i);
            if (pt.x >= mw || pt.y >= mh) {
                uniq = false;
                continue;
            }
            const u32 ti = static_cast<u32>(pt.y) * static_cast<u32>(mw) + static_cast<u32>(pt.x);
            if (claimed[ti] != 0u || map.get_planned_city(pt.x, pt.y) == 0u) {
                uniq = false;
                continue;
            }
            claimed[ti] = 1u;
        }
        char ppm[384];
        std::snprintf(ppm, sizeof(ppm), "%s/gen_settlement_order_excl_p%02u.ppm", g_dir, p);
        note_result(write_order_ppm(ppm, map, ord, p), ppm);
        std::printf("wrote %s\n", ppm);
    }
    note_result(nonempty, "each excl list nonempty");
    note_result(hd_tl, "excl hd=st tl=en");
    note_result(uniq, "excl sites unique and planned");
    note_result(excl_sum <= plan_n, "excl sum <= planned");
    std::printf("excl sum=%u planned=%u\n", excl_sum, plan_n);

    double all_ms = 0.0;
    bool all_ok = true;
    bool all_ge = true;
    GenSettlementOrder all_ord;
    for (u32 s = 0; s < starts.n; ++s) {
        const auto a0 = std::chrono::steady_clock::now();
        const bool ok = all_ord.gen_all(map, starts.pts[s].x, starts.pts[s].y);
        const auto a1 = std::chrono::steady_clock::now();
        all_ms += std::chrono::duration<double, std::milli>(a1 - a0).count();
        if (!ok || all_ord.pn() != 1u) {
            all_ok = false;
        }
        if (all_ord.n(0) < ord.n(static_cast<u16>(s))) {
            all_ge = false;
        }
        if (all_ord.hd(0) != all_ord.st(0) || all_ord.tl(0) != all_ord.en(0)) {
            all_ok = false;
        }
        std::printf("all p=%u n=%u\n", s, all_ord.n(0));
        char ppm[384];
        std::snprintf(ppm, sizeof(ppm), "%s/gen_settlement_order_all_p%02u.ppm", g_dir, s);
        note_result(write_order_ppm(ppm, map, all_ord, 0), ppm);
        std::printf("wrote %s\n", ppm);
    }
    note_result(all_ok, "GenSettlementOrder::gen_all");
    note_result(all_ge, "all n >= excl n");
    std::printf("all wall_ms=%.3f (sum over %u starts)\n", all_ms, starts.n);

    std::printf("=======================================================\n");
    std::printf(" GEN SETTLEMENT ORDER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
