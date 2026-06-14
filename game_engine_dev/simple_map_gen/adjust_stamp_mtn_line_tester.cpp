//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "adjust_stamp_mtn_line.h"
#include "game_primitives.h"
#include "generate_river_lines.h"
#include "generate_river_network.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "generate_small_area_mtn_line_dp.h"
#include "generate_watershed_mtn_lines.h"
#include "map_loader.h"
#include "map_terrain_data.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path = "/home/w/Projects/simple-map-gen/adjust-stamp-mtn-line.ppm";
static const i32 g_hills_perc = 90;
static const i32 g_tile_cap_perc = 10;
static const u32 g_min_split_tiles = 25;
static const f64 g_split_angle_deg = 150.0;
static const f64 g_min_seg_len = 8.0;

static StampMtnLineParams oversize_params (
    u32 covered,
    u32 border_tile_n,
    const StampMtnLineOversizeLvl* lvls,
    u32 lvl_n) 
{
    StampMtnLineParams p = {};
    if (lvls == nullptr || lvl_n == 0 || border_tile_n == 0) {
        return p;
    }
    const u32 stamp_perc = covered * 100u / border_tile_n;
    u32 idx = 0;
    for (u32 i = 0; i < lvl_n; ++i) {
        if (stamp_perc >= static_cast<u32>(lvls[i].m_stamp_perc)) {
            idx = i;
        }
    }
    p.m_oversize_factor = lvls[idx].m_oversize_factor;
    p.m_oversize_px = lvls[idx].m_oversize_px;
    return p;
}

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool side_hills_ok (u16 pln, u16 hil, i32 perc) {
    const u32 tot = static_cast<u32>(pln) + static_cast<u32>(hil);
    if (tot == 0) {
        return false;
    }
    return static_cast<u32>(hil) * 100u >= tot * static_cast<u32>(perc);
}

static bool seg_hills_ok (const WatershedBorderSeg& s, i32 perc) {
    return side_hills_ok(s.a_plains, s.a_hills, perc) || side_hills_ok(s.b_plains, s.b_hills, perc);
}

static u32 tile_cap_n (u32 border_tile_n, i32 perc) {
    if (border_tile_n == 0 || perc <= 0) {
        return 0;
    }
    if (perc >= 100) {
        return border_tile_n;
    }
    return (border_tile_n * static_cast<u32>(perc) + 99u) / 100u;
}

static void print_seg (const WatershedMtnLinesResult* mtns, u16 si, i32 hills_perc) {
    if (mtns == nullptr || mtns->segs == nullptr || si >= mtns->seg_n) {
        return;
    }
    const WatershedBorderSeg& s = mtns->segs[si];
    std::printf("  seg %u: basins %u-%u tiles %u\n", si, s.basin_a, s.basin_b, s.tile_n);
    std::printf("    a pln/hil %u/%u  b pln/hil %u/%u  hills ok %s\n",
        s.a_plains,
        s.a_hills,
        s.b_plains,
        s.b_hills,
        seg_hills_ok(s, hills_perc) ? "yes" : "no");
}

//================================================================================================================================
//=> - Test -
//================================================================================================================================

i32 test_adjust_stamp_mtn_line_basic (
    u32 seed,
    const StampMtnLineOversizeLvl* oversize_lvls,
    u32 oversize_lvl_n) 
{
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {
        std::printf("failed to load map: %s\n", g_map_path);
        return -1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain_in = map.data();
    if (terrain_in == nullptr || w == 0 || h == 0) {
        std::printf("invalid map data\n");
        return -1;
    }
    const u32 terr_n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[terr_n];
    if (terrain == nullptr) {
        std::printf("out of memory\n");
        return -1;
    }
    for (u32 i = 0; i < terr_n; ++i) {
        terrain[i] = terrain_in[i];
    }
    RiverPtsResult* pts = Generate_RiverPts::generate(terrain, w, h, seed);
    if (pts == nullptr) {
        std::printf("Generate_RiverPts failed\n");
        return -1;
    }
    RiverSectorsResult* sectors = Generate_RiverSectors::generate(terrain, w, h, pts, seed);
    Generate_RiverPts::free_result(pts);
    if (sectors == nullptr) {
        std::printf("Generate_RiverSectors failed\n");
        return -1;
    }
    RiverNetworkResult* network = Generate_RiverNetwork::generate(terrain, w, h, sectors);
    if (network == nullptr) {
        Generate_RiverSectors::free_result(sectors);
        std::printf("Generate_RiverNetwork failed\n");
        return -1;
    }
    WatershedMtnLinesResult* mtns = Generate_WatershedMtnLines::generate(terrain, w, h, network);
    if (mtns == nullptr) {
        Generate_RiverNetwork::free_result(network);
        Generate_RiverSectors::free_result(sectors);
        std::printf("Generate_WatershedMtnLines failed\n");
        return -1;
    }
    const u32 border_tile_n = mtns->border_tile_n;
    const u32 tile_cap = tile_cap_n(border_tile_n, g_tile_cap_perc);
    std::printf(
        "selection: from end, hills>=%d%% on a side, tile cap %d%% (%u / %u border tiles)\n",
        g_hills_perc,
        g_tile_cap_perc,
        tile_cap,
        border_tile_n);
    SmallAreaMtnLineDpParams dp_params = {};
    dp_params.m_min_split_tile_n = g_min_split_tiles;
    dp_params.m_split_angle_deg = g_split_angle_deg;
    dp_params.m_min_seg_len = g_min_seg_len;
    Adjust_StampMtnLine adj(seed);
    u32 covered = 0;
    u32 stamped_n = 0;
    u32 total_stamp_px = 0;
    u32 tried_n = 0;
    const clock_t t0 = clock();
    for (u16 si = mtns->seg_n; si > 0; --si) {
        const u16 idx = static_cast<u16>(si - 1u);
        const WatershedBorderSeg& s = mtns->segs[idx];
        if (!seg_hills_ok(s, g_hills_perc)) {
            continue;
        }
        if (s.tile_n == 0) {
            continue;
        }
        if (covered >= tile_cap) {
            std::printf("stop: covered %u / %u border tiles\n", covered, tile_cap);
            break;
        }
        if (covered + s.tile_n > tile_cap) {
            std::printf("stop: seg %u would exceed tile cap\n", idx);
            break;
        }
        ++tried_n;
        const u32 stamp_perc = covered * 100u / border_tile_n;
        const StampMtnLineParams stamp_params = oversize_params(covered, border_tile_n, oversize_lvls, oversize_lvl_n);
        std::printf(
            "try seg %u: stamp %u%% oversize %.2f +%u px\n",
            idx,
            stamp_perc,
            stamp_params.m_oversize_factor,
            stamp_params.m_oversize_px);
        print_seg(mtns, idx, g_hills_perc);
        SmallAreaMtnLineDpResult* dp = Generate_SmallAreaMtnLineDp::generate(mtns, idx, dp_params);
        if (dp == nullptr) {
            std::printf("  Generate_SmallAreaMtnLineDp failed\n");
            continue;
        }
        const u32 px = adj.stamp_dp(terrain, w, h, dp, stamp_params);
        Generate_SmallAreaMtnLineDp::free_result(dp);
        if (px > 0) {
            ++stamped_n;
            total_stamp_px += px;
            covered += s.tile_n;
            std::printf("  stamped %u px, covered %u / %u border tiles\n", px, covered, tile_cap);
        } else {
            std::printf("  no pixels stamped\n");
        }
    }
    const clock_t t1 = clock();
    const double usec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC) * 1000000.0;
    Generate_WatershedMtnLines::free_result(mtns);
    Generate_RiverNetwork::free_result(network);
    Generate_RiverSectors::free_result(sectors);
    std::printf(
        "selection done: tried %u  stamped %u  total pixels %u  covered %u / %u  stamp %.0f us\n",
        tried_n,
        stamped_n,
        total_stamp_px,
        covered,
        tile_cap,
        usec);
    if (stamped_n == 0) {
        std::printf("no watershed pair stamped\n");
        delete[] terrain;
        return -1;
    }
    MapTerrainData out_map;
    if (!out_map.assign_copy(w, h, terrain)) {
        std::printf("out of memory\n");
        delete[] terrain;
        return -1;
    }
    delete[] terrain;
    if (!out_map.save_terrain_ppm(g_out_path)) {
        std::printf("failed to save map: %s\n", g_out_path);
        return -1;
    }
    std::printf("saved: %s\n", g_out_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 42;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    static const StampMtnLineOversizeLvl oversize_lvls[5] = {
        { 0, 1.2, 10 },
        { 10, 1.0, 5 },
        { 20, 0.8, 0 },
        { 30, 0.6, 0 },
        { 40, 0.4, 0 },
    };
    return test_adjust_stamp_mtn_line_basic(seed, oversize_lvls, 5);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
