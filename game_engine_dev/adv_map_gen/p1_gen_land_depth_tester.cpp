//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_primitives.h"
#include "p1_tester_util.h"
#include "p1_gen_land_depth.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_p1_gen_land_depth_basic (const P1_RunPrm& prm) {
    char out_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    P1_Gen_LandDepth gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_LandDepth failed to generate\n");
        return -1;
    }
    const P1_Gen_LandDepthRslt& r = gen.result();
    std::printf("P1_Gen_LandDepth generate time: %.6f s (%u x %u)\n",
        sec,
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
    if (!p1_tester_checkout(argc, argv)) {
        return -1;
    }
    P1_RunPrm prm;
    p1_resolve_run_prm(argc, argv, &prm);
    return test_p1_gen_land_depth_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
