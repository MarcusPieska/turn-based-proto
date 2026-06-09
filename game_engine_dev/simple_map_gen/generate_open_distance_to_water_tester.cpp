//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_primitives.h"
#include "generate_open_distance_to_water.h"
#include "map_loader.h"
#include "perlin_noise.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path =
    "/home/w/Projects/simple-map-gen/open-distance-water.ppm";

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_generate_open_distance_to_water_basic () {
    const clock_t t_load0 = clock();
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {
        std::printf("failed to load map: %s\n", g_map_path);
        return -1;
    }
    const clock_t t_load1 = clock();
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain = map.data();
    if (terrain == nullptr || w == 0 || h == 0) {
        std::printf("invalid map data\n");
        return -1;
    }
    const clock_t t_gen0 = clock();
    u8* pix = Generate_OpenDistanceToWater::generate(terrain, w, h);
    const clock_t t_gen1 = clock();
    if (pix == nullptr) {
        std::printf("Generate_OpenDistanceToWater failed to generate\n");
        return -1;
    }
    const clock_t t_save0 = clock();
    const bool ok = save_perlin_gray_pgm(g_out_path, pix, w, h);
    const clock_t t_save1 = clock();
    delete[] pix;
    const double load_sec = static_cast<double>(t_load1 - t_load0) / static_cast<double>(CLOCKS_PER_SEC);
    const double gen_sec = static_cast<double>(t_gen1 - t_gen0) / static_cast<double>(CLOCKS_PER_SEC);
    const double save_sec = static_cast<double>(t_save1 - t_save0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("Generate_OpenDistanceToWater load time: %.6f s\n", load_sec);
    std::printf("Generate_OpenDistanceToWater generate time: %.6f s (%u x %u)\n", gen_sec, w, h);
    std::printf("Generate_OpenDistanceToWater save time: %.6f s\n", save_sec);
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

i32 main () {
    return test_generate_open_distance_to_water_basic();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
