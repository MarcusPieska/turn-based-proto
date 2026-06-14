//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_TESTER_UTIL_H
#define P1_TESTER_UTIL_H

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>

#include "game_primitives.h"
#include "p1_adj_land_altitude.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 tester helpers -
//================================================================================================================================

static const char* P1_OUT_ROOT = "/home/w/Projects/simple-map-gen";
static const char* P1_SEED_FILE = "seed.txt";

static const f32 P1_TEST_RADIAL_LIM = 0.45f;

static bool g_p1_out_subdir = true;

//================================================================================================================================
//=> - P1_TesterCfg -
//================================================================================================================================

struct P1_TesterCfg {
    cstr m_exe;
    u32 m_step;
    cstr m_out;
};

static const P1_TesterCfg g_p1_tester_tbl[] = {
    {"p1_gen_outline_tester", 1u, "01_outline.ppm"},
    {"p1_adj_outline_fill_tester", 2u, "02_outline_fill.ppm"},
    {"p1_gen_noise_perlin_tester", 3u, "03_noise_perlin.ppm"},
    {"p1_gen_land_depth_tester", 4u, "04_land_depth.ppm"},
    {"p1_gen_shaped_outline_tester", 5u, "07_shaped_outline_merge.ppm"},
    {"p1_gen_river_pts_tester", 8u, "08_river_pts.ppm"},
    {"p1_gen_river_sectors_tester", 9u, "09_river_sectors.ppm"},
    {"p1_gen_river_network_tester", 10u, "10_river_network.ppm"},
    {"p1_gen_river_lines_tester", 11u, "11_river_lines.ppm"},
    {"p1_adj_river_lakes_tester", 12u, "12_river_lakes.ppm"},
    {"p1_adj_river_inlets_tester", 13u, "13_river_inlets.ppm"},
    {"p1_gen_watershed_mountains_tester", 14u, "14_watershed_mountains.ppm"},
    {"p1_gen_watershed_mountain_line_sets_tester", 14u, "14_watershed_mountain_line_sets.ppm"},
    {"p1_gen_distance_to_river_tester", 15u, "15_distance_to_river.ppm"},
    {"p1_gen_nearness_to_watershed_mtn_tester", 16u, "16_nearness_watershed_mtn.ppm"},
    {"p1_adj_land_altitude_tester", 17u, "17_land_altitude.ppm"},
    {"p1_adj_ensure_coasts_tester", 18u, "18_ensure_coasts.ppm"},
    {"p1_adj_ensure_seas_tester", 19u, "19_ensure_seas.ppm"},
    {"p1_adj_ensure_river_valleys_tester", 20u, "20_ensure_river_valleys.ppm"},
    {"p1_adj_ensure_mtn_foothills_tester", 21u, "21_ensure_mtn_foothills.ppm"},
    {"p1_gen_climate_tester", 22u, "22_climate.ppm"},
    {"p1_gen_desert_river_cull_tester", 23u, "23_desert_river_cull_upstream.ppm"},
    {"p1_make_map_tester", 24u, "24_make_map.ppm"},
};

static const P1_TesterCfg* g_p1_tester_cfg = nullptr;

static cstr p1_exe_base (i32 argc, char* argv[]) {
    if (argc < 1 || argv == nullptr || argv[0] == nullptr) {
        return nullptr;
    }
    cstr base = argv[0];
    for (cstr p = argv[0]; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    return base;
}

static const P1_TesterCfg* p1_tester_cfg_lookup (cstr exe_base) {
    if (exe_base == nullptr) {
        return nullptr;
    }
    const size_t n = sizeof(g_p1_tester_tbl) / sizeof(g_p1_tester_tbl[0]);
    for (size_t i = 0; i < n; ++i) {
        if (std::strcmp(exe_base, g_p1_tester_tbl[i].m_exe) == 0) {
            return &g_p1_tester_tbl[i];
        }
    }
    return nullptr;
}

static bool p1_tester_checkout (i32 argc, char* argv[]) {
    g_p1_tester_cfg = p1_tester_cfg_lookup(p1_exe_base(argc, argv));
    if (g_p1_tester_cfg == nullptr) {
        std::printf("P1 tester checkout failed: unknown executable %s\n",
            p1_exe_base(argc, argv) != nullptr ? p1_exe_base(argc, argv) : "(null)");
        return false;
    }
    return true;
}

static u32 p1_tester_step () {
    return g_p1_tester_cfg != nullptr ? g_p1_tester_cfg->m_step : 0u;
}

static cstr p1_tester_out () {
    return g_p1_tester_cfg != nullptr ? g_p1_tester_cfg->m_out : nullptr;
}

static u32 p1_read_seed_file () {
    FILE* f = std::fopen(P1_SEED_FILE, "r");
    if (f == nullptr) {
        return 42u;
    }
    u32 seed = 0;
    if (std::fscanf(f, "%u", &seed) != 1) {
        std::fclose(f);
        return 42u;
    }
    std::fclose(f);
    return seed;
}

static u32 p1_resolve_seed (i32 argc, char* argv[]) {
    if (argc >= 2) {
        g_p1_out_subdir = false;
        return static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    g_p1_out_subdir = true;
    return p1_read_seed_file();
}

static void p1_resolve_run_prm (i32 argc, char* argv[], P1_RunPrm* prm) {
    if (prm == nullptr) {
        return;
    }
    *prm = p1_run_prm_def();
    prm->m_seed = p1_resolve_seed(argc, argv);
    if (argc >= 3) {
        prm->m_w = static_cast<u16>(std::strtoul(argv[2], nullptr, 10));
    }
    if (argc >= 4) {
        prm->m_h = static_cast<u16>(std::strtoul(argv[3], nullptr, 10));
    }
}

static void p1_resolve_test_args (i32 argc, char* argv[], u32* seed, u16* map_w, u16* map_h) {
    P1_RunPrm prm;
    p1_resolve_run_prm(argc, argv, &prm);
    if (seed != nullptr) {
        *seed = prm.m_seed;
    }
    if (map_w != nullptr) {
        *map_w = prm.m_w;
    }
    if (map_h != nullptr) {
        *map_h = prm.m_h;
    }
}

static inline P1_Adj_LandAltitudePrm p1_tester_land_altitude_prm () {
    P1_Adj_LandAltitudePrm sp = p1_adj_land_altitude_prm_def();
    sp.m_w_noise = 0.7f;
    sp.m_w_near = 0.85f;
    sp.m_w_riv = 0.5f;
    sp.m_lim_hills = 0.4f;
    sp.m_lim_mtn = 0.8f;
    return sp;
}

static void p1_resolve_land_altitude_prm (i32 argc, char* argv[], P1_Adj_LandAltitudePrm* sp) {
    if (sp == nullptr) {
        return;
    }
    *sp = p1_tester_land_altitude_prm();
    if (argc >= 5) {
        sp->m_w_noise = static_cast<f32>(std::strtod(argv[4], nullptr));
    }
    if (argc >= 6) {
        sp->m_w_near = static_cast<f32>(std::strtod(argv[5], nullptr));
    }
    if (argc >= 7) {
        sp->m_w_riv = static_cast<f32>(std::strtod(argv[6], nullptr));
    }
    if (argc >= 8) {
        sp->m_lim_hills = static_cast<f32>(std::strtod(argv[7], nullptr));
    }
    if (argc >= 9) {
        sp->m_lim_mtn = static_cast<f32>(std::strtod(argv[8], nullptr));
    }
}

static bool p1_ensure_dir (cstr path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

static bool p1_make_out_path (u32 seed, cstr fname, char* out, size_t cap) {
    if (fname == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    if (!p1_ensure_dir(P1_OUT_ROOT)) {
        return false;
    }
    if (g_p1_out_subdir) {
        char dir[256];
        std::snprintf(dir, sizeof(dir), "%s/p1-seed-%03u", P1_OUT_ROOT, static_cast<unsigned>(seed));
        if (!p1_ensure_dir(dir)) {
            return false;
        }
        std::snprintf(out, cap, "%s/%s", dir, fname);
        return true;
    }
    std::snprintf(out, cap, "%s/p1_seed_%03u_%s", P1_OUT_ROOT, static_cast<unsigned>(seed), fname);
    return true;
}

static bool p1_tester_make_out (u32 seed, char* out, size_t cap) {
    return p1_make_out_path(seed, p1_tester_out(), out, cap);
}

static bool p1_tester_make_step_out (
    u32 seed,
    u32 step,
    cstr suffix,
    char* out,
    size_t cap) 
{
    if (suffix == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    char fname[160];
    std::snprintf(fname, sizeof(fname), "%02u_%s.ppm", step, suffix);
    return p1_make_out_path(seed, fname, out, cap);
}

#endif // P1_TESTER_UTIL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
