//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_primitives.h"
#include "p1_tester_cli.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_p1_make_map_basic (P1_TesterHarness& h) {
    P1_MakeMap mk(h.prm());
    const clock_t t0 = clock();
    const bool ok = mk.generate(k_p1_step_seed_export);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !mk.is_valid()) {
        std::fprintf(stderr, "P1_MakeMap failed to generate\n");
        return -1;
    }
    if (!p1_tester_batch_export()) {
        const P1_MakeMapRslt& r = mk.result();
        std::printf("P1_MakeMap generate time: %.6f s (%u x %u)\n",
            sec,
            static_cast<u32>(r.m_w),
            static_cast<u32>(r.m_h));
    }
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
