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
#include "generate_distance_land_to_water.h"

static bool ensure_maps_dir () {
    if (mkdir("maps", 0755) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_generate_distance_land_to_water_basic (u32 seed) {
    if (!ensure_maps_dir()) {
        std::printf("could not create maps/\n");
        return -1;
    }
    Generate_DistanceLandToWater gen(seed);
    const clock_t t0 = clock();
    const bool ok = gen.generate();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("Generate_DistanceLandToWater failed to generate\n");
        return -1;
    }
    std::printf("Generate_DistanceLandToWater generate time: %.6f s (%u x %u)\n", sec, gen.width(), gen.height());
    if (!gen.save_output("maps/out_map_distance_land_to_water.ppm")) {
        std::printf("save failed\n");
        return -1;
    }
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

    return test_generate_distance_land_to_water_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
