//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "game_primitives.h"
#include "generate_distance_to_river.h"
#include "generate_latitude_overlay.h"
#include "generate_map_climate.h"
#include "generate_open_distance_to_water.h"
#include "generate_plain_distance_to_water.h"
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
static const char* g_out_path = "/home/w/Projects/simple-map-gen/map-climate.ppm";

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

static bool save_climate_viz (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const MapClimateResult* res) 
{
    if (path == nullptr || terrain == nullptr || res == nullptr || res->climate == nullptr || w == 0 || h == 0) {
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
        const u8 cl = res->climate[i];
        if (cl != CLIMATE_NONE) {
            climate_to_rgb(cl, &r, &g, &b);
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_generate_map_climate_basic (u32 seed) {
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
    const clock_t t_riv0 = clock();
    u8* dist_river = Generate_DistanceToRiver::generate(terrain, w, h, lines->overlay);
    const clock_t t_riv1 = clock();
    Generate_RiverLines::free_result(lines);
    Generate_RiverSectors::free_result(sectors);
    if (dist_river == nullptr) {
        std::printf("Generate_DistanceToRiver failed to generate\n");
        return -1;
    }
    const clock_t t_open0 = clock();
    u8* open_dist_water = Generate_OpenDistanceToWater::generate(terrain, w, h);
    const clock_t t_open1 = clock();
    if (open_dist_water == nullptr) {
        delete[] dist_river;
        std::printf("Generate_OpenDistanceToWater failed to generate\n");
        return -1;
    }
    const clock_t t_lat0 = clock();
    u8* latitude = Generate_LatitudeOverlay::generate(w, h);
    const clock_t t_lat1 = clock();
    if (latitude == nullptr) {
        delete[] open_dist_water;
        delete[] dist_river;
        std::printf("Generate_LatitudeOverlay failed to generate\n");
        return -1;
    }
    const clock_t t_plain0 = clock();
    u8* plain_dist_water = Generate_PlainDistanceToWater::generate(terrain, w, h);
    const clock_t t_plain1 = clock();
    if (plain_dist_water == nullptr) {
        delete[] latitude;
        delete[] open_dist_water;
        delete[] dist_river;
        std::printf("Generate_PlainDistanceToWater failed to generate\n");
        return -1;
    }
    MapClimateOverlays ovs = {};
    ovs.dist_river = dist_river;
    ovs.open_dist_water = open_dist_water;
    ovs.latitude = latitude;
    ovs.plain_dist_water = plain_dist_water;
    MapClimateParams prm = {};
    prm.wts.w_dist_river = 50;
    prm.wts.w_open_dist_water = 10;
    prm.wts.w_latitude = 90;
    prm.wts.w_plain_dist_water = 10;
    prm.pct.pct_grassland = 40;
    prm.pct.pct_plains = 35;
    prm.pct.pct_desert = 25;
    const clock_t t0 = clock();
    MapClimateResult* climate = Generate_MapClimate::generate(terrain, w, h, &ovs, &prm);
    const clock_t t1 = clock();
    delete[] plain_dist_water;
    delete[] latitude;
    delete[] open_dist_water;
    delete[] dist_river;
    if (climate == nullptr) {
        std::printf("Generate_MapClimate failed to generate\n");
        return -1;
    }
    const double pts_sec = static_cast<double>(t_pts1 - t_pts0) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec_sec = static_cast<double>(t_sec1 - t_sec0) / static_cast<double>(CLOCKS_PER_SEC);
    const double net_sec = static_cast<double>(t_net1 - t_net0) / static_cast<double>(CLOCKS_PER_SEC);
    const double lin_sec = static_cast<double>(t_lin1 - t_lin0) / static_cast<double>(CLOCKS_PER_SEC);
    const double riv_sec = static_cast<double>(t_riv1 - t_riv0) / static_cast<double>(CLOCKS_PER_SEC);
    const double open_sec = static_cast<double>(t_open1 - t_open0) / static_cast<double>(CLOCKS_PER_SEC);
    const double lat_sec = static_cast<double>(t_lat1 - t_lat0) / static_cast<double>(CLOCKS_PER_SEC);
    const double plain_sec = static_cast<double>(t_plain1 - t_plain0) / static_cast<double>(CLOCKS_PER_SEC);
    const double clim_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("Generate_RiverPts generate time: %.6f s\n", pts_sec);
    std::printf("Generate_RiverSectors generate time: %.6f s\n", sec_sec);
    std::printf("Generate_RiverNetwork generate time: %.6f s\n", net_sec);
    std::printf("Generate_RiverLines generate time: %.6f s\n", lin_sec);
    std::printf("Generate_DistanceToRiver generate time: %.6f s\n", riv_sec);
    std::printf("Generate_OpenDistanceToWater generate time: %.6f s\n", open_sec);
    std::printf("Generate_LatitudeOverlay generate time: %.6f s\n", lat_sec);
    std::printf("Generate_PlainDistanceToWater generate time: %.6f s\n", plain_sec);
    std::printf("Generate_MapClimate generate time: %.6f s (%u x %u)\n", clim_sec, w, h);
    const bool ok = save_climate_viz(g_out_path, terrain, w, h, climate);
    Generate_MapClimate::free_result(climate);
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
    return test_generate_map_climate_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
