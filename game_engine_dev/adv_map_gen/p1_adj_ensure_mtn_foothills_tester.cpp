//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_adj_ensure_mtn_foothills.h"
#include "game_primitives.h"
#include "p1_tester_chain15.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_p1_adj_ensure_mtn_foothills_basic (const P1_RunPrm& prm, const P1_Adj_LandAltitudePrm& lap) {
    char out_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    P1_TesterChain15Rslt chain = {};
    double sec_i = 0.0;
    if (!p1_build_ensure_input(prm, lap, static_cast<u16>(p1_tester_step()), &chain, &sec_i)) {
        std::printf("P1 steps 1-20 input failed for step 21\n");
        return -1;
    }
    P1_Adj_EnsureMtnFoothills adj(prm);
    const clock_t t0 = clock();
    const bool ok = adj.adjust(chain.m_terrain, chain.m_w, chain.m_h);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_EnsureMtnFoothills failed to adjust\n");
        p1_free_chain15(&chain);
        return -1;
    }
    std::printf("P1 steps 1-20 input time: %.6f s\n", sec_i);
    std::printf("P1_Adj_EnsureMtnFoothills adjust time: %.6f s (%u x %u)\n",
        sec,
        static_cast<u32>(chain.m_w),
        static_cast<u32>(chain.m_h));
    if (!p1_save_terrain_rivers_ppm(out_path, chain.m_terrain, chain.m_river, chain.m_w, chain.m_h)) {
        std::printf("failed to save map: %s\n", out_path);
        p1_free_chain15(&chain);
        return -1;
    }
    p1_free_chain15(&chain);
    std::printf("saved: %s\n", out_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    if (!p1_tester_checkout(argc, argv)) {
        return -1;
    }
    P1_RunPrm prm;
    P1_Adj_LandAltitudePrm lap;
    p1_resolve_run_prm(argc, argv, &prm);
    p1_resolve_land_altitude_prm(argc, argv, &lap);
    return test_p1_adj_ensure_mtn_foothills_basic(prm, lap);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
