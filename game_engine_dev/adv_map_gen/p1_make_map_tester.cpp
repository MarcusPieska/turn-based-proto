//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_primitives.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_p1_make_map_basic (P1_TesterHarness& h) {
    char terr_path[320];
    char clim_path[320];
    char riv_path[320];
    if (!h.path_pri(terr_path, sizeof(terr_path))
        || !h.path_sec(clim_path, sizeof(clim_path))
        || !h.path_extra("make_map_rivers", riv_path, sizeof(riv_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    P1_MakeMap mk(h.prm());
    const clock_t t0 = clock();
    const bool ok = mk.generate(k_p1_step_seed_export);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !mk.is_valid()) {
        std::printf("P1_MakeMap failed to generate\n");
        return -1;
    }
    const P1_MakeMapRslt& r = mk.result();
    std::printf("P1_MakeMap generate time: %.6f s (%u x %u)\n",
        sec,
        static_cast<u32>(r.m_w),
        static_cast<u32>(r.m_h));
    if (!mk.save_terrain_ppm(terr_path)) {
        std::printf("failed to save map: %s\n", terr_path);
        return -1;
    }
    std::printf("saved: %s\n", terr_path);
    if (!mk.save_climate_ppm(clim_path)) {
        std::printf("failed to save climate: %s\n", clim_path);
        return -1;
    }
    std::printf("saved: %s\n", clim_path);
    if (!mk.save_rivers_ppm(riv_path)) {
        std::printf("failed to save rivers: %s\n", riv_path);
        return -1;
    }
    std::printf("saved: %s\n", riv_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    P1_TesterHarness h;
    if (!h.begin(argc, argv)) {
        return -1;
    }
    const i32 rc = test_p1_make_map_basic(h);
    if (rc != 0) {
        return rc;
    }
    if (!h.finish()) {
        return -1;
    }
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
