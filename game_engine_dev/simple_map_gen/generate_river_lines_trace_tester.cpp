//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "game_primitives.h"
#include "generate_river_line_data.h"
#include "generate_river_lines.h"
#include "generate_river_network.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "map_loader.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path = "/home/w/Projects/simple-map-gen/river-lines-trace-miss.ppm";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

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

static bool save_miss_viz (cstr path, const u8* riv, const u16* rdep, u16 w, u16 h) {
    if (path == nullptr || riv == nullptr || rdep == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 v = 255;
        if (riv[i] != 0 && rdep[i] == 0) {
            v = 0;
        }
        rgb[i * 3u + 0] = v;
        rgb[i * 3u + 1] = v;
        rgb[i * 3u + 2] = v;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Test -
//================================================================================================================================

static i32 run_trace (u32 seed) {
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {
        std::printf("failed to load map: %s\n", g_map_path);
        return -1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain = map.data();
    if (terrain == nullptr || w == 0 || h == 0) {
        std::printf("invalid map data\n");
        return -1;
    }
    RiverPtsResult* pts = Generate_RiverPts::generate(terrain, w, h, seed);
    if (pts == nullptr) {
        std::printf("Generate_RiverPts failed to generate\n");
        return -1;
    }
    RiverSectorsResult* sectors = Generate_RiverSectors::generate(terrain, w, h, pts, seed);
    Generate_RiverPts::free_result(pts);
    if (sectors == nullptr) {
        std::printf("Generate_RiverSectors failed to generate\n");
        return -1;
    }
    RiverNetworkResult* network = Generate_RiverNetwork::generate(terrain, w, h, sectors);
    if (network == nullptr) {
        Generate_RiverSectors::free_result(sectors);
        std::printf("Generate_RiverNetwork failed to generate\n");
        return -1;
    }
    RiverLinesResult* lines = Generate_RiverLines::generate(terrain, w, h, sectors, network, seed);
    Generate_RiverSectors::free_result(sectors);
    if (lines == nullptr) {
        Generate_RiverNetwork::free_result(network);
        std::printf("Generate_RiverLines failed to generate\n");
        return -1;
    }
    RiverLineDataResult* ldata = Generate_RiverLineData::generate(terrain, lines->overlay, network->overlay, w, h);
    Generate_RiverNetwork::free_result(network);
    if (ldata == nullptr) {
        Generate_RiverLines::free_result(lines);
        std::printf("Generate_RiverLineData failed to generate\n");
        return -1;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 riv_n = 0;
    u32 miss_n = 0;
    for (u32 i = 0; i < n; ++i) {
        if (lines->overlay[i] == 0) {
            continue;
        }
        riv_n++;
        if (ldata->rdep[i] == 0) {
            miss_n++;
        }
    }
    std::printf("river systems: %u\n", static_cast<u32>(ldata->sys_n));
    std::printf("river tiles: %u\n", riv_n);
    const u32 show_n = ldata->sys_n < 8 ? static_cast<u32>(ldata->sys_n) : 8u;
    for (u32 si = 0; si < show_n; ++si) {
        const RiverSysEntry& s = ldata->sys[si];
        std::printf(
            "  sys %u: mouth (%u,%u) tiles %u max_depth %u\n",
            si,
            s.mx,
            s.my,
            s.tile_n,
            s.max_d);
    }
    const bool viz_ok = save_miss_viz(g_out_path, lines->overlay, ldata->rdep, w, h);
    if (!viz_ok) {
        std::printf("failed to save: %s\n", g_out_path);
    } else {
        std::printf("saved miss overlay: %s (%u tiles)\n", g_out_path, miss_n);
    }
    if (miss_n > 0) {
        std::printf("FAIL: %u river tiles with zero depth\n", miss_n);
        Generate_RiverLineData::free_result(ldata);
        Generate_RiverLines::free_result(lines);
        return -1;
    }
    std::printf("PASS: all river tiles traced\n");
    Generate_RiverLineData::free_result(ldata);
    Generate_RiverLines::free_result(lines);
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
    return run_trace(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
