//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <time.h>

#include "game_primitives.h"
#include "generate_terrain_cont_pn.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_generate_terrain_cont_pn_basic (u32 seed) {
    TerrainContPnParams params;
    params.m_seed = seed;

    const clock_t t0 = clock();
    Generate_TerrainContPn gen(params);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!gen.is_valid()) {
        std::printf("Generate_TerrainContPn failed to generate\n");
        return -1;
    }
    std::printf("Generate_TerrainContPn generate time: %.6f s\n", sec);
    gen.save_output("maps/out_generate_terrain_cont_pn.ppm");
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

    return test_generate_terrain_cont_pn_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
