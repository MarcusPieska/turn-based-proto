//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "game_primitives.h"
#include "generate_river_line_data.h"
#include "generate_river_lines.h"
#include "generate_river_network.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "map_loader.h"
#include "map_terrain_validate.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path = "/home/w/Projects/simple-map-gen/river-line-data.ppm";

static const u8 k_col_b[3] = {0, 0, 255};
static const u8 k_col_r[3] = {255, 0, 0};
static const u8 k_col_p[3] = {160, 32, 240};
static const u8 k_col_br[3] = {139, 69, 19};

//================================================================================================================================
//=> - Test helpers -
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

static bool save_top3_viz (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverLineDataResult* data) 
{
    if (path == nullptr || terrain == nullptr || data == nullptr || data->rsys == nullptr || w == 0 || h == 0) {
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
        const u16 si = data->rsys[i];
        const u8* col = nullptr;
        if (si == 0) {
            col = k_col_b;
        } else if (si == 1) {
            col = k_col_r;
        } else if (si == 2) {
            col = k_col_p;
        } else if (si == 3) {
            col = k_col_br;
        }
        if (col != nullptr) {
            r = col[0];
            g = col[1];
            b = col[2];
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_generate_river_line_data_basic (u32 seed) {
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
    const clock_t t0 = clock();
    RiverLineDataResult* data = Generate_RiverLineData::generate(terrain, lines->overlay, network->overlay, w, h);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    Generate_RiverNetwork::free_result(network);
    if (data == nullptr) {
        Generate_RiverLines::free_result(lines);
        std::printf("Generate_RiverLineData failed to generate\n");
        return -1;
    }
    std::printf("Generate_RiverLineData generate time: %.6f s (%u x %u)\n", sec, static_cast<u32>(w), static_cast<u32>(h));
    std::printf("river systems: %u\n", static_cast<u32>(data->sys_n));
    const u32 show_n = data->sys_n < 4 ? static_cast<u32>(data->sys_n) : 4u;
    for (u32 si = 0; si < show_n; ++si) {
        const RiverSysEntry& s = data->sys[si];
        std::printf(
            "  sys %u: mouth (%u,%u) tiles %u max_depth %u\n",
            si,
            s.mx,
            s.my,
            s.tile_n,
            s.max_d);
    }
    if (!save_top3_viz(g_out_path, terrain, w, h, data)) {
        Generate_RiverLineData::free_result(data);
        Generate_RiverLines::free_result(lines);
        std::printf("failed to save map: %s\n", g_out_path);
        return -1;
    }
    Generate_RiverLineData::free_result(data);
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
    return test_generate_river_line_data_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
