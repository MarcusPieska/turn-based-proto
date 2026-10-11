//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_nearness_to_mtn.h"
#include "game_primitives.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"
#include "profile_time.h"

//================================================================================================================================
//=> - Test -
//================================================================================================================================

static i32 test_p1_gen_nearness_to_mtn_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        std::printf("P1 MakeMap input through foothills failed for nearness_to_mtn\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for nearness_to_mtn\n");
        return -1;
    }
    std::printf("P1 MakeMap input time: %.6f s\n", h.input_sec());

    char out_path[320];
    if (!h.path_pri(out_path, sizeof(out_path))) {
        std::printf("failed to ensure primary output path\n");
        return -1;
    }

    P1_Gen_NearnessToMtnPrm sp = p1_gen_nearness_to_mtn_prm_def();
    sp.m_stop = p1_gen_nearness_to_mtn_stop_from_w(chain.m_w);
    P1_Gen_NearnessToMtn gen(h.prm(), sp);
    PTIME_START("P1_Gen_NearnessToMtn");
    const clock_t t0 = clock();
    const bool ok = gen.generate(chain.m_terrain, chain.m_w, chain.m_h);
    const clock_t t1 = clock();
    PTIME_STOP("P1_Gen_NearnessToMtn");
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_NearnessToMtn failed\n");
        return -1;
    }
    const P1_Gen_NearnessToMtnRslt& r = gen.result();
    std::printf(
        "P1_Gen_NearnessToMtn generate time: %.6f s (stop %u, seeds %u, filled %u, %u x %u)\n",
        sec,
        static_cast<u32>(r.m_stop),
        r.m_seed_n,
        r.m_fill_n,
        static_cast<u32>(r.m_w),
        static_cast<u32>(r.m_h));
    gen.save_output(out_path);
    std::printf("saved: %s\n", out_path);
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
    const i32 rc = test_p1_gen_nearness_to_mtn_basic(h);
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
