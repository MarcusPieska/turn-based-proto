//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "adjust_river_to_inlets.h"
#include "game_primitives.h"
#include "generate_river_line_data.h"
#include "generate_river_lines.h"
#include "generate_river_network.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "generator_constants.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "map_terrain_validate.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path = "/home/w/Projects/simple-map-gen/river-to-inlets.ppm";

//================================================================================================================================
//=> - Viz helpers -
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

static bool save_viz (
    cstr path,
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || riv == nullptr || w == 0 || h == 0) {
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
        if (riv[i] != 0 && terrain[i] != TERR_COASTAL[0]) {
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

//================================================================================================================================
//=> - Test -
//================================================================================================================================

i32 test_adjust_river_to_inlets_basic (u32 seed) {
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
    std::printf("river systems: %u\n", static_cast<u32>(ldata->sys_n));
    MapTerrainData out;
    if (!out.assign_copy(w, h, terrain)) {
        Generate_RiverLineData::free_result(ldata);
        Generate_RiverLines::free_result(lines);
        std::printf("failed to copy terrain\n");
        return -1;
    }
    u8* td = const_cast<u8*>(out.data());
    Adjust_RiverToInlets adj(seed);
    const clock_t t0 = clock();
    adj.adjust(td, lines->overlay, ldata, static_cast<u8>(RIVER_INLET_PERC_DEF), static_cast<u8>(RIVER_INLET_MIN_DEF));
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!adj.is_valid()) {
        Generate_RiverLineData::free_result(ldata);
        Generate_RiverLines::free_result(lines);
        std::printf("Adjust_RiverToInlets failed to adjust\n");
        return -1;
    }
    std::printf("Adjust_RiverToInlets adjust time: %.6f s (%u x %u) perc=%u min=%u\n",
        sec, static_cast<u32>(w), static_cast<u32>(h),
        static_cast<u32>(RIVER_INLET_PERC_DEF), static_cast<u32>(RIVER_INLET_MIN_DEF));
    if (!save_viz(g_out_path, td, lines->overlay, w, h)) {
        Generate_RiverLineData::free_result(ldata);
        Generate_RiverLines::free_result(lines);
        std::printf("failed to save map: %s\n", g_out_path);
        return -1;
    }
    Generate_RiverLineData::free_result(ldata);
    Generate_RiverLines::free_result(lines);
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
    return test_adjust_river_to_inlets_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
