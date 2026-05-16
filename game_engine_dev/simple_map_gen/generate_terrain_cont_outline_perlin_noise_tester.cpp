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
#include "generate_terrain_cont_outline.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static bool ensure_maps_dir () {
    if (mkdir("maps", 0755) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 0;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    if (!ensure_maps_dir()) {
        std::printf("could not create maps/\n");
        return -1;
    }
    TerrainContOutlineParams params;
    params.m_seed = seed;
    params.m_width = 1000;
    params.m_height = 1000;
    params.m_fill_mode = TERR_OUTLINE_FILL_MODE_PERLIN_NOISE;
    Generate_TerrainContOutline gen(params);
    const clock_t t0 = clock();
    const bool ok = gen.generate();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("Generate_TerrainContOutline perlin_noise failed\n");
        return -1;
    }
    std::printf("perlin_noise mode time: %.6f s (%u x %u)\n", sec, gen.width(), gen.height());
    if (!gen.save_output("maps/out_map_terrain_cont_outline_perlin_noise.ppm")) {
        std::printf("save terrain failed\n");
        return -1;
    }
    if (!gen.has_land_to_water_distance()) {
        std::printf("land_to_water distance not available\n");
        return -1;
    }
    if (!gen.save_land_to_water_distance("maps/out_map_terrain_cont_outline_perlin_noise_land_to_water.ppm")) {
        std::printf("save land_to_water distance failed\n");
        return -1;
    }
    if (!gen.save_interior_grad_mask("maps/out_map_terrain_cont_outline_perlin_noise_interior_mask.pgm")) {
        std::printf("save interior grad mask failed\n");
        return -1;
    }
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================