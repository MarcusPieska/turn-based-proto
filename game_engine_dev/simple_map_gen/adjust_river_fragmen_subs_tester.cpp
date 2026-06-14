//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "adjust_river_fragmen_subs.h"
#include "adjust_river_to_inlets.h"
#include "adjust_river_to_lake.h"
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
static const char* g_before_path = "/home/w/Projects/simple-map-gen/river-fragmen-subs-before.ppm";
static const char* g_after_path = "/home/w/Projects/simple-map-gen/river-fragmen-subs-after.ppm";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_coastal_cls (u8 cls) {
    return cls == TERR_COASTAL[0];
}

static u32 count_coastal (const u8* terrain, u16 w, u16 h) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 cnt = 0;
    for (u32 i = 0; i < n; ++i) {
        if (is_coastal_cls(terrain[i])) {
            cnt++;
        }
    }
    return cnt;
}

static u32 count_coastal_diff (const u8* inp, const u8* out, u16 w, u16 h) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 cnt = 0;
    for (u32 i = 0; i < n; ++i) {
        if (is_coastal_cls(out[i]) && !is_coastal_cls(inp[i])) {
            cnt++;
        }
    }
    return cnt;
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

i32 test_adjust_river_fragmen_subs_basic (u32 seed) {
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
    MapTerrainData inp;
    MapTerrainData out;
    if (!inp.assign_copy(w, h, terrain) || !out.assign_copy(w, h, terrain)) {
        Generate_RiverNetwork::free_result(network);
        Generate_RiverLines::free_result(lines);
        std::printf("failed to copy terrain\n");
        return -1;
    }
    const u8* td_inp = inp.data();
    u8* td = const_cast<u8*>(out.data());
    const u32 coastal_in = count_coastal(td_inp, w, h);
    Adjust_RiverToLake lake(seed);
    if (!lake.adjust(td, lines->overlay, w, h) || !lake.is_valid()) {
        Generate_RiverNetwork::free_result(network);
        Generate_RiverLines::free_result(lines);
        std::printf("Adjust_RiverToLake failed\n");
        return -1;
    }
    RiverLineDataResult* ldata = Generate_RiverLineData::generate(td, lines->overlay, network->overlay, w, h);
    Generate_RiverNetwork::free_result(network);
    if (ldata == nullptr) {
        Generate_RiverLines::free_result(lines);
        std::printf("Generate_RiverLineData failed to generate\n");
        return -1;
    }
    Adjust_RiverToInlets inlets(seed);
    if (!inlets.adjust(td, lines->overlay, ldata, static_cast<u8>(RIVER_INLET_PERC_DEF), static_cast<u8>(RIVER_INLET_MIN_DEF))
        || !inlets.is_valid()) {
        Generate_RiverLineData::free_result(ldata);
        Generate_RiverLines::free_result(lines);
        std::printf("Adjust_RiverToInlets failed\n");
        return -1;
    }
    Generate_RiverLineData::free_result(ldata);
    if (!save_viz(g_before_path, td, lines->overlay, w, h)) {
        Generate_RiverLines::free_result(lines);
        std::printf("failed to save before: %s\n", g_before_path);
        return -1;
    }
    MapTerrainData pre_frag;
    if (!pre_frag.assign_copy(w, h, td)) {
        Generate_RiverLines::free_result(lines);
        std::printf("failed to copy pre_frag terrain\n");
        return -1;
    }
    const u8* td_pre = pre_frag.data();
    const u32 coastal_pre_frag = count_coastal(td_pre, w, h);
    Adjust_RiverFragmenSubs frag(seed);
    const clock_t t0 = clock();
    if (!frag.adjust(td, lines->overlay, w, h) || !frag.is_valid()) {
        Generate_RiverLines::free_result(lines);
        std::printf("Adjust_RiverFragmenSubs failed\n");
        return -1;
    }
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const u32 coastal_out = count_coastal(td, w, h);
    const u32 coastal_new = count_coastal_diff(td_inp, td, w, h);
    const u32 coastal_frag = count_coastal_diff(td_pre, td, w, h);
    std::printf("coastal input: %u  pre_frag: %u  output: %u  diff total: %u  diff frag: %u\n",
        coastal_in, coastal_pre_frag, coastal_out, coastal_new, coastal_frag);
    std::printf("Adjust_RiverFragmenSubs adjust time: %.6f s (%u x %u)\n",
        sec, static_cast<u32>(w), static_cast<u32>(h));
    if (!save_viz(g_after_path, td, lines->overlay, w, h)) {
        Generate_RiverLines::free_result(lines);
        std::printf("failed to save after: %s\n", g_after_path);
        return -1;
    }
    Generate_RiverLines::free_result(lines);
    std::printf("saved before: %s\n", g_before_path);
    std::printf("saved after: %s\n", g_after_path);
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
    return test_adjust_river_fragmen_subs_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
