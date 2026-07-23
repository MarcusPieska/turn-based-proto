//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_TESTER_UTIL_H
#define P1_TESTER_UTIL_H

#include "map_config.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "p1_adj_land_altitude.h"
#include "p1_gen_climate.h"
#include "p1_map_size.h"
#include "p1_pipeline_steps.h"
#include "p1_tester_cli.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - P1 tester helpers -
//================================================================================================================================

static const char* P1_OUT_ROOT = "/home/w/Projects/simple-map-gen";
static const char* P1_MAPS_ROOT = "/home/w/Projects/simple-map-gen/maps";
static const char* P1_SEED_FILE = "seed.txt";

static const f32 P1_TEST_RADIAL_LIM = 0.45f;

//================================================================================================================================
//=> - P1 tester input kinds -
//================================================================================================================================

static const u8 P1_TIN_NONE = 0u;
static const u8 P1_TIN_MK = 1u;
static const u8 P1_TIN_C14 = 2u;
static const u8 P1_TIN_CENS = 3u;
static const u8 P1_TIN_EARLY = 4u;

//================================================================================================================================
//=> - P1_TesterCfg -
//================================================================================================================================

struct P1_TesterCfg {
    cstr m_exe;
    u32 m_step;
    u8 m_in_kind;
    u16 m_in_step;
    cstr m_out_pri;
    cstr m_out_sec;
};

static const P1_TesterCfg g_p1_tester_tbl[] = { 
    {"p1_gen_cont_outlines_tester", 1u, P1_TIN_NONE, 0u, "01_outline.ppm", ""},
    {"p1_adj_outline_fill_tester", 2u, P1_TIN_EARLY, 1u, "02_outline_fill.ppm", ""},
    {"p1_gen_noise_perlin_tester", 3u, P1_TIN_EARLY, 3u, "03_noise_perlin.ppm", ""},
    {"p1_gen_land_depth_tester", 4u, P1_TIN_EARLY, 1u, "04_land_depth.ppm", ""},
    {"p1_gen_shaped_outline_tester", 7u, P1_TIN_EARLY, 4u, "07_shaped_outline_merge.ppm", ""},
    {"p1_gen_river_prob_tester", P1_STEP_RIVER_PROB, P1_TIN_EARLY, 7u, "08_river_prob.ppm", "08_river_prob_wat.ppm"},
    {"p1_gen_ocean_index_tester", P1_STEP_OCEAN_INDEX, P1_TIN_EARLY, 7u, "09_ocean_index.ppm", ""},
    {"p1_gen_river_dynamic_pts_tester", P1_STEP_RIVER_PTS, P1_TIN_EARLY, 7u, "10_river_pts.ppm", ""},
    {"p1_gen_river_pts_tester", 108u, P1_TIN_EARLY, P1_STEP_OCEAN_INDEX, "108_river_pts_static.ppm", ""},
    {"p1_gen_river_sectors_tester", P1_STEP_RIVER_SECTORS, P1_TIN_EARLY, P1_STEP_RIVER_PTS, "11_river_sectors.ppm", ""},
    {"p1_gen_river_sect_adj_tester", P1_STEP_RIVER_SECT_ADJ, P1_TIN_EARLY, P1_STEP_RIVER_SECTORS, "12_river_sect_adj.ppm", ""},
    {"p1_gen_coastal_mtn_limits_tester", P1_STEP_COASTAL_MTN_LIMITS, P1_TIN_NONE, 0u, "13_coastal_mtn_limits.ppm", ""},
    {"p1_gen_river_network_tester", P1_STEP_RIVER_NETWORK, P1_TIN_EARLY, P1_STEP_RIVER_PTS, "14_river_network.ppm", ""},
    {"p1_gen_river_lines_tester", P1_STEP_RIVER_LINES, P1_TIN_EARLY, P1_STEP_RIVER_SECTORS, "15_river_lines.ppm", "15_river_lines_dist.ppm"},
    {"p1_adj_coastal_mtn_rivers_tester", P1_STEP_COASTAL_MTN_RIVERS, P1_TIN_NONE, 0u, "16_coastal_mtn_rivers.ppm", ""},
    {"p1_adj_river_lakes_tester", P1_STEP_RIVER_LAKES, P1_TIN_NONE, 0u, "17_river_lakes.ppm", ""},
    {"p1_adj_river_inlets_tester", P1_STEP_RIVER_INLETS, P1_TIN_NONE, 0u, "18_river_inlets.ppm", ""},
    {"p1_gen_watershed_mountains_tester", P1_STEP_WATERSHED_MTN, P1_TIN_NONE, 0u, "19_watershed_mountains.ppm", ""},
    {"p1_gen_watershed_mountain_line_sets_tester", P1_STEP_WATERSHED_MTN, P1_TIN_NONE, 0u, "19_watershed_mountain_line_sets.ppm", ""},
    {"p1_gen_distance_to_river_tester", P1_STEP_DISTANCE_TO_RIVER, P1_TIN_NONE, 0u, "20_distance_to_river.ppm", ""},
    {"p1_gen_river_dist_tester", P1_STEP_RIVER_DIST, P1_TIN_NONE, 0u, "21_river_dist_up.ppm", "21_river_dist_dn.ppm"},
    {"p1_gen_nearness_to_watershed_mtn_tester", P1_STEP_NEARNESS_WATERSHED_MTN, P1_TIN_NONE, 0u, "22_nearness_watershed_mtn.ppm", ""},
    {"p1_adj_land_altitude_tester", P1_STEP_LAND_ALTITUDE, P1_TIN_C14, 0u, "23_land_altitude.ppm", "23_land_altitude_joint.ppm"},
    {"p1_adj_ensure_coasts_tester", P1_STEP_ENSURE_COASTS, P1_TIN_CENS, P1_STEP_LAND_ALTITUDE, "24_ensure_coasts.ppm", ""},
    {"p1_adj_ensure_seas_tester", P1_STEP_ENSURE_SEAS, P1_TIN_CENS, P1_STEP_ENSURE_COASTS, "25_ensure_seas.ppm", ""},
    {"p1_adj_ensure_river_valleys_tester", P1_STEP_ENSURE_RIVER_VALLEYS, P1_TIN_CENS, P1_STEP_ENSURE_SEAS, "26_ensure_river_valleys.ppm", ""},
    {"p1_adj_ensure_mtn_foothills_tester", P1_STEP_ENSURE_MTN_FOOTHILLS, P1_TIN_CENS, P1_STEP_ENSURE_RIVER_VALLEYS, "27_ensure_mtn_foothills.ppm", ""},
    {"p1_gen_wind_pattern_adv_tester", P1_STEP_WIND, P1_TIN_MK, P1_STEP_ENSURE_MTN_FOOTHILLS, "28_wind_pattern_adv_dir.ppm", "28_wind_pattern_adv_str.ppm"},
    {"p1_gen_rain_orographic_tester", P1_STEP_RAIN, P1_TIN_MK, P1_STEP_WIND, "29_rain_oro_rain.ppm", ""},
    {"p1_gen_climate_tester", P1_STEP_CLIMATE, P1_TIN_MK, P1_STEP_RAIN, "30_climate.ppm", "30_climate_terrain.ppm"},
    {"p1_gen_desert_river_cull_tester", P1_STEP_DESERT_CULL, P1_TIN_MK, P1_STEP_CLIMATE, "31_desert_river_cull_upstream.ppm", "31_desert_river_cull_downstream.ppm"},
    {"p1_gen_loess_boost_tester", P1_STEP_LOESS, P1_TIN_MK, P1_STEP_CLIMATE, "32_loess_boost.ppm", ""},
    {"p1_adj_grassland_loess_tiles_tester", P1_STEP_GRASS_LOESS, P1_TIN_MK, P1_STEP_LOESS, "33_grassland_loess_tiles.ppm", ""},
    {"p1_make_map_tester", P1_STEP_MAKE_MAP, P1_TIN_MK, P1_STEP_MAKE_MAP, "35_make_map_terrain.ppm", "35_make_map_climate.ppm"},
    {"p1_gen_rich_coast_fertility_tester", P1_STEP_RICH_COAST_FERT, P1_TIN_MK, P1_STEP_CLIMATE, "36_rich_coast_fertility.ppm", ""},
    {"p1_adj_coast_fertility_tester", P1_STEP_COAST_FERT_ADJ, P1_TIN_MK, P1_STEP_CLIMATE, "37_coast_fertility_adj.ppm", ""},
    {"p1_adj_ensure_adj_rules_tester", P1_STEP_ENSURE_ADJ, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "41_ensure_adj_rules.ppm", "41_ensure_adj_rules_terrain.ppm"},
    {"p1_gen_forest_overlay_tester", P1_STEP_FOREST_OVERLAY, P1_TIN_MK, P1_STEP_COAST_FERT_ADJ, "39_forest_overlay.ppm", ""},
    {"p1_adj_delta_swamps_tester", P1_STEP_DELTA_SWAMPS, P1_TIN_MK, P1_STEP_FOREST_OVERLAY, "40_delta_swamps.ppm", "40_delta_swamps_climate.ppm"},
    {"r1_adj_general_placement_tester", P1_STEP_RESOURCES, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "42_resources.ppm", ""},
    {"r1_gen_res_sectors_tester", P1_STEP_RESOURCES, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "43_res_sectors.ppm", "43_res_sectors_terrain.ppm"},
    {"r1_adj_gemstone_placements_tester", P1_STEP_RESOURCES, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "44_gemstone_placements.ppm", ""},
    {"r1_adj_metal_placements_tester", P1_STEP_RESOURCES, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "45_metal_placements.ppm", ""},
    {"r1_adj_livestock_placements_tester", P1_STEP_RESOURCES, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "46_livestock_placements.ppm", ""},
    {"r1_adj_food_crop_placements_tester", P1_STEP_RESOURCES, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "47_food_crop_placements.ppm", ""},
    {"r1_adj_spice_placements_tester", P1_STEP_RESOURCES, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "48_spice_placements.ppm", ""},
    {"r1_adj_cash_crop_placements_tester", P1_STEP_RESOURCES, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "49_cash_crop_placements.ppm", ""},
    {"r1_adj_produce_placements_tester", P1_STEP_RESOURCES, P1_TIN_MK, P1_STEP_DELTA_SWAMPS, "50_produce_placements.ppm", ""},
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
    return g_p1_tester_cfg != nullptr ? g_p1_tester_cfg->m_out_pri : nullptr;
}

static cstr p1_tester_out_sec () {
    return g_p1_tester_cfg != nullptr ? g_p1_tester_cfg->m_out_sec : nullptr;
}

static u8 p1_tester_in_kind () {
    return g_p1_tester_cfg != nullptr ? g_p1_tester_cfg->m_in_kind : P1_TIN_NONE;
}

static u16 p1_tester_in_step () {
    return g_p1_tester_cfg != nullptr ? g_p1_tester_cfg->m_in_step : 0u;
}

bool p1_tester_clear_out (u32 seed);

static inline void p1_resolve_run_prm (i32 argc, char* argv[], P1_RunPrm* prm) {
    P1_TesterCli cli;
    cli.parse(argc, argv);
    if (prm != nullptr) {
        *prm = cli.prm();
    }
}

static inline void p1_resolve_test_args (i32 argc, char* argv[], u32* seed, u16* map_w, u16* map_h) {
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
    P1_Adj_LandAltitudePrm sp = p1_adj_land_altitude_prm_from_cfg(map_config_def());
    sp.m_w_noise = 0.7f;
    sp.m_w_near = 0.85f;
    sp.m_w_riv = 0.5f;
    sp.m_lim_hills = 0.4f;
    sp.m_lim_mtn = 0.90f;
    return sp;
}

static inline void p1_resolve_land_altitude_prm (i32 argc, char* argv[], P1_Adj_LandAltitudePrm* sp) {
    P1_TesterCli cli;
    cli.parse(argc, argv);
    if (sp != nullptr) {
        *sp = cli.lap();
    }
}

static inline bool p1_resolve_climate_rain_wt (i32 argc, char* argv[], u8* out_wt, bool* out_set) {
    if (out_wt == nullptr || out_set == nullptr) {
        return false;
    }
    P1_TesterCli cli;
    cli.parse(argc, argv);
    *out_wt = cli.rain_wt();
    *out_set = cli.rain_wt_set();
    return true;
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

static bool p1_make_batch_export_path (u32 seed, cstr kind, char* out, size_t cap) {
    if (kind == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    if (!p1_ensure_dir(P1_OUT_ROOT) || !p1_ensure_dir(P1_MAPS_ROOT)) {
        return false;
    }
    std::snprintf(out, cap, "%s/seed-%04u-%s.ppm", P1_MAPS_ROOT, static_cast<unsigned>(seed), kind);
    return true;
}

static bool p1_make_final_export_path (u32 seed, cstr layer, char* out, size_t cap) {
    if (layer == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    if (!p1_ensure_dir(P1_OUT_ROOT)) {
        return false;
    }
    char dir[256];
    std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", P1_OUT_ROOT, static_cast<unsigned>(seed));
    if (!p1_ensure_dir(dir)) {
        return false;
    }
    std::snprintf(out, cap, "%s/%s.ppm", dir, layer);
    return true;
}

static bool p1_make_out_path (u32 seed, cstr fname, char* out, size_t cap) {
    if (fname == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    if (!p1_ensure_dir(P1_OUT_ROOT)) {
        return false;
    }
    if (p1_tester_out_subdir()) {
        char dir[256];
        std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", P1_OUT_ROOT, static_cast<unsigned>(seed));
        if (!p1_ensure_dir(dir)) {
            return false;
        }
        std::snprintf(out, cap, "%s/%s", dir, fname);
        return true;
    }
    std::snprintf(out, cap, "%s/p1_seed_%u_%s", P1_OUT_ROOT, static_cast<unsigned>(seed), fname);
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

static bool p1_tester_whiteboard_chk () {
    const u32 n = WhiteboardMng::chkout();
    if (n != 0u) {
        std::printf("Whiteboard checkout leak: %u\n", n);
        return false;
    }
    return true;
}

#endif // P1_TESTER_UTIL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
