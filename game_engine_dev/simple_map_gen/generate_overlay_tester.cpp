//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <sys/stat.h>
#include <sys/types.h>

#include "game_primitives.h"
#include "generate_overlay.h"
#include "generate_terrain_cont_outline.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool ensure_maps_dir () {
    if (mkdir("maps", 0755) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
}

static bool gen_outline_terrain (u32 seed, MapArrayTerrain& out) {
    TerrainContOutlineParams params;
    params.m_seed = seed;
    params.m_width = 1000;
    params.m_height = 1000;
    params.m_fill_mode = TERR_OUTLINE_FILL_MODE_PERLIN_NOISE;
    params.m_pn_params.m_inner_grad_limit = 0.50f;
    Generate_TerrainContOutline gen(params);
    if (!gen.generate() || !gen.is_valid()) {
        return false;
    }
    return out.assign_copy(gen.width(), gen.height(), gen.terrain().data());
}

i32 test_generate_overlay_basic (u32 seed) {
    if (!ensure_maps_dir()) {
        std::printf("could not create maps/\n");
        return -1;
    }
    MapArrayTerrain ter;
    if (!gen_outline_terrain(seed, ter)) {
        std::printf("outline terrain failed\n");
        return -1;
    }
    Generate_Overlay ov;
    const clock_t t0 = clock();
    if (!ov.generate_spec_terrain_overlay(ter, TERR_MOUNTAINS[0])) {
        std::printf("spec terrain overlay failed\n");
        return -1;
    }
    const clock_t t1 = clock();
    if (!ov.save_output("maps/out_map_overlay_spec_mountains.pgm")) {
        std::printf("save spec mountains failed\n");
        return -1;
    }
    if (!ov.generate_terrain_limit_overlay(ter, TERR_PLAINS[0])) {
        std::printf("terrain limit overlay failed\n");
        return -1;
    }
    if (!ov.save_output("maps/out_map_overlay_limit_plains.pgm")) {
        std::printf("save limit plains failed\n");
        return -1;
    }
    if (!ov.generate_spec_terrains_overlay(ter, TERR_HILLS[0], TERR_MOUNTAINS[0])) {
        std::printf("spec terrains overlay failed\n");
        return -1;
    }
    if (!ov.save_output("maps/out_map_overlay_spec_hills_mountains.pgm")) {
        std::printf("save spec hills+mountains failed\n");
        return -1;
    }
    if (!ov.generate_spec_terrain_overlay(ter, TERR_OCEAN[0], true)) {
        std::printf("spec terrain invert overlay failed\n");
        return -1;
    }
    if (!ov.save_output("maps/out_map_overlay_spec_ocean_invert.pgm")) {
        std::printf("save spec ocean invert failed\n");
        return -1;
    }
    const clock_t t2 = clock();
    const double sec = static_cast<double>(t2 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ov.is_valid()) {
        std::printf("Generate_Overlay not valid\n");
        return -1;
    }
    std::printf("Generate_Overlay generate time: %.6f s\n", sec);
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
    return test_generate_overlay_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
