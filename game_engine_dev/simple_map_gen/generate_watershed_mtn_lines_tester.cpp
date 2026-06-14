//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "game_primitives.h"
#include "generate_river_lines.h"
#include "generate_river_network.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "generate_watershed_mtn_lines.h"
#include "generator_constants.h"
#include "map_loader.h"
#include "map_terrain_validate.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path = "/home/w/Projects/simple-map-gen/watershed-mtn-lines.ppm";
static const i32 g_hills_perc = 90;
static const i32 g_tile_cap_perc = 30;
static const i32 g_min_dist_perc = 5;

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool is_plains (u8 cls) {
    return cls == TERR_PLAINS[0];
}

static bool is_hills (u8 cls) {
    return cls == TERR_HILLS[0];
}

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

static u32 seg_mtn_tile_n (
    const u8* terrain,
    u16 w,
    u16 h,
    const WatershedMtnLinesResult* mtns,
    u16 oid) 
{
    if (terrain == nullptr || mtns == nullptr || mtns->overlay == nullptr || w == 0 || h == 0) {
        return 0;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 cnt = 0;
    for (u32 i = 0; i < n; ++i) {
        if (mtns->overlay[i] != oid) {
            continue;
        }
        if (is_water(terrain[i])) {
            continue;
        }
        if (!is_plains(terrain[i]) && !is_hills(terrain[i])) {
            continue;
        }
        cnt++;
    }
    return cnt;
}

static void stamp_seg_mtns (
    u8* terrain,
    u16 w,
    u16 h,
    const WatershedMtnLinesResult* mtns,
    u16 oid) 
{
    if (terrain == nullptr || mtns == nullptr || mtns->overlay == nullptr || w == 0 || h == 0) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (mtns->overlay[i] != oid) {
            continue;
        }
        if (is_water(terrain[i])) {
            continue;
        }
        if (!is_plains(terrain[i]) && !is_hills(terrain[i])) {
            continue;
        }
        terrain[i] = TERR_MOUNTAINS[0];
    }
}

static u32 max_mouth_dist (const WatershedMtnLinesResult* mtns) {
    if (mtns == nullptr || mtns->segs == nullptr || mtns->seg_n == 0) {
        return 0;
    }
    return mtns->segs[mtns->seg_n - 1].mouth_dist;
}

static bool seg_dist_too_short (u32 mouth_dist, u32 max_dist, i32 min_perc) {
    if (max_dist == 0 || min_perc <= 0) {
        return false;
    }
    if (min_perc >= 100) {
        return false;
    }
    return mouth_dist * 100u < max_dist * static_cast<u32>(min_perc);
}

static u32 apply_mtns (
    u8* terrain,
    u16 w,
    u16 h,
    const WatershedMtnLinesResult* mtns,
    i32 hills_perc,
    i32 tile_cap_perc,
    i32 min_dist_perc,
    u8* picked) 
{
    if (terrain == nullptr || mtns == nullptr || mtns->overlay == nullptr || mtns->segs == nullptr) {
        return 0;
    }
    const u32 cap = tile_cap_n(mtns->border_tile_n, tile_cap_perc);
    const u32 max_dist = max_mouth_dist(mtns);
    u32 added = 0;
    u32 picked_n = 0;
    for (u16 si = mtns->seg_n; si > 0; --si) {
        if (added >= cap) {
            break;
        }
        const WatershedBorderSeg& s = mtns->segs[si - 1];
        if (seg_dist_too_short(s.mouth_dist, max_dist, min_dist_perc)) {
            break;
        }
        if (!seg_hills_ok(s, hills_perc)) {
            continue;
        }
        const u32 seg_n = seg_mtn_tile_n(terrain, w, h, mtns, s.ov_idx);
        if (seg_n == 0) {
            continue;
        }
        if (added + seg_n > cap) {
            break;
        }
        stamp_seg_mtns(terrain, w, h, mtns, s.ov_idx);
        added += seg_n;
        if (picked != nullptr) {
            picked[si - 1] = 1;
        }
        picked_n++;
    }
    return picked_n;
}

static bool save_viz (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverLinesResult* lines) 
{
    if (path == nullptr || terrain == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(terrain[i], &r, &g, &b);
        if (lines != nullptr && lines->overlay != nullptr && lines->overlay[i] != 0) {
            r = 0;
            g = 0;
            b = 255;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_generate_watershed_mtn_lines_basic (u32 seed) {
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
    const clock_t t_pts0 = clock();
    RiverPtsResult* pts = Generate_RiverPts::generate(terrain, w, h, seed);
    const clock_t t_pts1 = clock();
    if (pts == nullptr) {
        std::printf("Generate_RiverPts failed to generate\n");
        return -1;
    }
    const clock_t t_sec0 = clock();
    RiverSectorsResult* sectors = Generate_RiverSectors::generate(terrain, w, h, pts, seed);
    const clock_t t_sec1 = clock();
    Generate_RiverPts::free_result(pts);
    if (sectors == nullptr) {
        std::printf("Generate_RiverSectors failed to generate\n");
        return -1;
    }
    const clock_t t_net0 = clock();
    RiverNetworkResult* network = Generate_RiverNetwork::generate(terrain, w, h, sectors);
    const clock_t t_net1 = clock();
    if (network == nullptr) {
        Generate_RiverSectors::free_result(sectors);
        std::printf("Generate_RiverNetwork failed to generate\n");
        return -1;
    }
    const clock_t t_lin0 = clock();
    RiverLinesResult* lines = Generate_RiverLines::generate(terrain, w, h, sectors, network, seed);
    const clock_t t_lin1 = clock();
    if (lines == nullptr) {
        Generate_RiverNetwork::free_result(network);
        Generate_RiverSectors::free_result(sectors);
        std::printf("Generate_RiverLines failed to generate\n");
        return -1;
    }
    const clock_t t0 = clock();
    WatershedMtnLinesResult* mtns = Generate_WatershedMtnLines::generate(terrain, w, h, network);
    const clock_t t1 = clock();
    if (mtns == nullptr) {
        delete[] terrain;
        Generate_RiverLines::free_result(lines);
        Generate_RiverNetwork::free_result(network);
        Generate_RiverSectors::free_result(sectors);
        std::printf("Generate_WatershedMtnLines failed to generate\n");
        return -1;
    }
    const double pts_sec = static_cast<double>(t_pts1 - t_pts0) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec_sec = static_cast<double>(t_sec1 - t_sec0) / static_cast<double>(CLOCKS_PER_SEC);
    const double net_sec = static_cast<double>(t_net1 - t_net0) / static_cast<double>(CLOCKS_PER_SEC);
    const double lin_sec = static_cast<double>(t_lin1 - t_lin0) / static_cast<double>(CLOCKS_PER_SEC);
    const double mtn_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("Generate_RiverPts generate time: %.6f s\n", pts_sec);
    std::printf("Generate_RiverSectors generate time: %.6f s\n", sec_sec);
    std::printf("Generate_RiverNetwork generate time: %.6f s\n", net_sec);
    std::printf("Generate_RiverLines generate time: %.6f s\n", lin_sec);
    std::printf("Generate_WatershedMtnLines generate time: %.6f s (%u x %u)\n", mtn_sec, w, h);
    std::printf("border segments: %u  border tiles: %u\n", mtns->seg_n, mtns->border_tile_n);
    u8* picked = new u8[mtns->seg_n];
    if (picked == nullptr) {
        delete[] terrain;
        Generate_WatershedMtnLines::free_result(mtns);
        Generate_RiverLines::free_result(lines);
        Generate_RiverNetwork::free_result(network);
        Generate_RiverSectors::free_result(sectors);
        std::printf("out of memory\n");
        return -1;
    }
    for (u16 si = 0; si < mtns->seg_n; ++si) {
        picked[si] = 0;
    }
    const u32 mtn_cap = tile_cap_n(mtns->border_tile_n, g_tile_cap_perc);
    const u32 max_dist = max_mouth_dist(mtns);
    const u32 picked_n = apply_mtns(terrain, w, h, mtns, g_hills_perc, g_tile_cap_perc, g_min_dist_perc, picked);
    std::printf(
        "mountain segments: %u  hills>=%d%% on a side  tile cap %d%% (%u)"
        "  min dist %d%% of max (%u)\n",
        picked_n,
        g_hills_perc,
        g_tile_cap_perc,
        mtn_cap,
        g_min_dist_perc,
        max_dist);
    for (u16 si = 0; si < mtns->seg_n; ++si) {
        const WatershedBorderSeg& s = mtns->segs[si];
        std::printf(
            "  seg ov %u basins %u-%u mouths (%u,%u)-(%u,%u) dist %u tiles %u"
            " a pln/hil %u/%u b pln/hil %u/%u%s\n",
            s.ov_idx,
            s.basin_a,
            s.basin_b,
            s.mouth_ax,
            s.mouth_ay,
            s.mouth_bx,
            s.mouth_by,
            s.mouth_dist,
            s.tile_n,
            s.a_plains,
            s.a_hills,
            s.b_plains,
            s.b_hills,
            picked[si] ? " *" : "");
    }
    delete[] picked;
    const bool ok = save_viz(g_out_path, terrain, w, h, lines);
    Generate_WatershedMtnLines::free_result(mtns);
    Generate_RiverLines::free_result(lines);
    Generate_RiverNetwork::free_result(network);
    Generate_RiverSectors::free_result(sectors);
    delete[] terrain;
    if (!ok) {
        std::printf("failed to save: %s\n", g_out_path);
        return -1;
    }
    std::printf("saved: %s\n", g_out_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 0;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    return test_generate_watershed_mtn_lines_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
