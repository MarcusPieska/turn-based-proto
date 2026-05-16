//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <time.h>

#include "game_primitives.h"
#include "generate_overlay_water_land.h"
#include "generate_overlay_wl_clip.h"
#include "generate_terrain_cont_pn.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_generate_overlay_wl_clip_basic (u32 seed) {
    TerrainContPnParams p0;
    p0.m_seed = seed ^ 0x9e3779b9u;
    p0.m_width = 1000;
    p0.m_height = 1000;
    p0.m_inner_grad_limit = 0.00f;
    Generate_TerrainContPn ta(p0);
    if (!ta.is_valid()) {
        std::printf("terrain A failed\n");
        return -1;
    }

    TerrainContPnParams p1;
    p1.m_seed = seed;
    p1.m_width = 1000;
    p1.m_height = 1000;
    p1.m_inner_grad_limit = 0.80f;
    Generate_TerrainContPn tb(p1);
    if (!tb.is_valid()) {
        std::printf("terrain B failed\n");
        return -1;
    }

    Generate_OverlayWaterLand wa;
    Generate_OverlayWaterLand wb;
    if (!wa.generate(ta.terrain()) || !wb.generate(tb.terrain())) {
        std::printf("overlay from terrain failed\n");
        return -1;
    }
    if (!wa.save_water_land_gray("maps/out_map_overlay_wl_clip_water_land_a.pgm")) {
        std::printf("save water/land overlay A failed\n");
        return -1;
    }
    if (!wb.save_water_land_gray("maps/out_map_overlay_wl_clip_water_land_b.pgm")) {
        std::printf("save water/land overlay B failed\n");
        return -1;
    }
    const MapArrayOverlay* oa = wa.overlay_ptr();
    const MapArrayOverlay* ob = wb.overlay_ptr();
    if (oa == nullptr || ob == nullptr) {
        std::printf("overlay ptr null\n");
        return -1;
    }

    Generate_OverlayWlClip clip;
    const clock_t t0 = clock();
    clip.generate(*oa, *ob, 0, 0);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!clip.is_valid()) {
        std::printf("Generate_OverlayWlClip failed to generate\n");
        return -1;
    }
    std::printf("Generate_OverlayWlClip generate time: %.6f s\n", sec);
    clip.save_output("maps/out_map_overlay_wl_clip.pgm");
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

    return test_generate_overlay_wl_clip_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
