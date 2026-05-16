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
#include "generate_terrain_combo_min.h"
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

static bool gen_outline (u32 seed, f32 lim, MapArrayTerrain& out) {
    TerrainContOutlineParams params;
    params.m_seed = seed;
    params.m_width = 1000;
    params.m_height = 1000;
    params.m_fill_mode = TERR_OUTLINE_FILL_MODE_PERLIN_NOISE;
    params.m_pn_params.m_inner_grad_limit = lim;
    Generate_TerrainContOutline gen(params);
    if (!gen.generate() || !gen.is_valid()) {
        return false;
    }
    return out.assign_copy(gen.width(), gen.height(), gen.terrain().data());
}

i32 test_generate_terrain_combo_min_basic (u32 seed) {
    if (!ensure_maps_dir()) {
        std::printf("could not create maps/\n");
        return -1;
    }
    MapArrayTerrain ta;
    MapArrayTerrain tb;
    if (!gen_outline(seed, 0.00f, ta)) {
        std::printf("outline radial 0.00 failed\n");
        return -1;
    }
    if (!gen_outline(seed, 0.85f, tb)) {
        std::printf("outline radial 0.85 failed\n");
        return -1;
    }
    if (!ta.save("maps/out_map_terrain_combo_min_component_a.ppm")) {
        std::printf("save component A failed\n");
        return -1;
    }
    if (!tb.save("maps/out_map_terrain_combo_min_component_b.ppm")) {
        std::printf("save component B failed\n");
        return -1;
    }
    Generate_TerrainComboMin gen;
    const clock_t t0 = clock();
    const bool ok = gen.generate(ta, tb);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("Generate_TerrainComboMin failed to generate\n");
        return -1;
    }
    std::printf("Generate_TerrainComboMin generate time: %.6f s\n", sec);
    gen.save_output("maps/out_map_terrain_combo_min.ppm");
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
    return test_generate_terrain_combo_min_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
