//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_adj_ensure_river_valleys.h"
#include "game_primitives.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_p1_adj_ensure_river_valleys_basic (P1_TesterHarness& h, const P1_Adj_LandAltitudePrm& lap) {
    char out_path[320];
    char terr_path[320];
    if (!h.path_pri(out_path, sizeof(out_path))
        || !h.path_extra("ensure_river_valleys_terrain", terr_path, sizeof(terr_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    if (!h.run_input(&lap)) {
        std::printf("P1 steps 1-19 input failed for step 20\n");
        return -1;
    }
    P1_TesterChain15Rslt& chain = h.c15_mut();
    P1_Adj_EnsureRiverValleys adj(h.prm());
    const clock_t t0 = clock();
    const bool ok = adj.adjust(chain.m_terrain, chain.m_w, chain.m_h, chain.m_river, chain.m_dist_dn);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_EnsureRiverValleys failed to adjust\n");
        return -1;
    }
    std::printf("P1 steps 1-19 input time: %.6f s\n", h.input_sec());
    std::printf("P1_Adj_EnsureRiverValleys adjust time: %.6f s (%u x %u)\n",
        sec,
        static_cast<u32>(chain.m_w),
        static_cast<u32>(chain.m_h));
    if (!p1_save_terrain_rivers_ppm(out_path, chain.m_terrain, chain.m_river, chain.m_w, chain.m_h)) {
        std::printf("failed to save map: %s\n", out_path);
        return -1;
    }
    std::printf("saved: %s\n", out_path);
    if (!p1_save_terrain_ppm(terr_path, chain.m_terrain, chain.m_w, chain.m_h)) {
        std::printf("failed to save terrain map: %s\n", terr_path);
        return -1;
    }
    std::printf("saved: %s\n", terr_path);
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
    P1_Adj_LandAltitudePrm lap;
    p1_resolve_land_altitude_prm(argc, argv, &lap);
    const i32 rc = test_p1_adj_ensure_river_valleys_basic(h, lap);
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
