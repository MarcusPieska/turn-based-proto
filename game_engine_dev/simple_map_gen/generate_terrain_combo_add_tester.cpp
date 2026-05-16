//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <time.h>

#include "game_primitives.h"
#include "generate_terrain_combo_add.h"
#include "generate_terrain_cont_pn.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_generate_terrain_combo_add_basic (u32 seed) {
    TerrainContPnParams p0;
    p0.m_seed = seed ^ 0x9e3779b9u;
    p0.m_width = 1000;
    p0.m_height = 1000;
    Generate_TerrainContPn a(p0);
    if (!a.is_valid()) {
        std::printf("terrain A failed\n");
        return -1;
    }
    if (!a.save_terrain_rgb("maps/out_map_terrain_combo_add_component_a.ppm")) {
        std::printf("save component A failed\n");
        return -1;
    }

    TerrainContPnParams p1;
    p1.m_seed = seed;
    p1.m_width = 1000;
    p1.m_height = 1000;
    p1.m_inner_grad_limit = 0.00f;
    Generate_TerrainContPn b(p1);
    if (!b.is_valid()) {
        std::printf("terrain B failed\n");
        return -1;
    }
    if (!b.save_terrain_rgb("maps/out_map_terrain_combo_add_component_b.ppm")) {
        std::printf("save component B failed\n");
        return -1;
    }

    Generate_TerrainComboAdd gen;
    const clock_t t0 = clock();
    gen.generate(a.terrain(), b.terrain(), 0, 0);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!gen.is_valid()) {
        std::printf("Generate_TerrainComboAdd failed to generate\n");
        return -1;
    }
    std::printf("Generate_TerrainComboAdd generate time: %.6f s\n", sec);
    gen.save_output("maps/out_map_terrain_combo_add.ppm");
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

    return test_generate_terrain_combo_add_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
