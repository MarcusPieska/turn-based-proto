//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "game_primitives.h"
#include "generate_distance_to_river.h"
#include "generate_river_lines.h"
#include "generate_river_network.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "map_loader.h"
#include "perlin_noise.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path =
    "/home/w/Projects/simple-map-gen/distance-to-river.ppm";

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_generate_distance_to_river_basic (u32 seed) {
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
    Generate_RiverNetwork::free_result(network);
    if (lines == nullptr) {
        Generate_RiverSectors::free_result(sectors);
        std::printf("Generate_RiverLines failed to generate\n");
        return -1;
    }
    const clock_t t0 = clock();
    u8* pix = Generate_DistanceToRiver::generate(terrain, w, h, lines->overlay);
    const clock_t t1 = clock();
    Generate_RiverLines::free_result(lines);
    Generate_RiverSectors::free_result(sectors);
    if (pix == nullptr) {
        std::printf("Generate_DistanceToRiver failed to generate\n");
        return -1;
    }
    const double pts_sec = static_cast<double>(t_pts1 - t_pts0) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec_sec = static_cast<double>(t_sec1 - t_sec0) / static_cast<double>(CLOCKS_PER_SEC);
    const double net_sec = static_cast<double>(t_net1 - t_net0) / static_cast<double>(CLOCKS_PER_SEC);
    const double lin_sec = static_cast<double>(t_lin1 - t_lin0) / static_cast<double>(CLOCKS_PER_SEC);
    const double dist_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("Generate_RiverPts generate time: %.6f s\n", pts_sec);
    std::printf("Generate_RiverSectors generate time: %.6f s\n", sec_sec);
    std::printf("Generate_RiverNetwork generate time: %.6f s\n", net_sec);
    std::printf("Generate_RiverLines generate time: %.6f s\n", lin_sec);
    std::printf("Generate_DistanceToRiver generate time: %.6f s (%u x %u)\n", dist_sec, w, h);
    const bool ok = save_perlin_gray_pgm(g_out_path, pix, w, h);
    delete[] pix;
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
    return test_generate_distance_to_river_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
