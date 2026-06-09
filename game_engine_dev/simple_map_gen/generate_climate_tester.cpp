//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <time.h>

#include "game_primitives.h"
#include "generate_climate.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_generate_climate_basic (u32 seed) {
    // Setup goes here, if any

    // Generation goes here, and is timed
    // The output is saved to local map folder

    Generate_Climate gen(seed);

    const clock_t t0 = clock();
    gen.generate();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!gen.is_valid()) {
        std::printf("Generate_Climate failed to generate\n");
        return -1;
    }
    std::printf("Generate_Climate generate time: %.6f s\n", sec);
    gen.save_output("maps/out_map_climate.ppm");
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

    return test_generate_climate_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
