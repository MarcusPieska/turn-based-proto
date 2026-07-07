//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_gen_cont_outlines.h"
#include "p1_tester_util.h"
#include "whiteboard.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool save_cont_merge (cstr path, const u8* comp, u16 w, u16 h) {
    if (path == nullptr || comp == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* tmp = new u8[n];
    if (tmp == nullptr) {
        return false;
    }
    p1_cont_ov_to_gray(comp, tmp, w, h); 
    MapArrayOverlay map;
    const bool ok = map.assign_copy(w, h, tmp) && map.save(path);
    delete[] tmp;
    return ok;
}

i32 test_p1_gen_cont_outlines_basic (const P1_RunPrm& prm) {
    P1_Gen_ContOutlines gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_ContOutlines failed to generate\n");
        return -1;
    }
    const P1_Gen_ContOutlinesRslt& r = gen.result();
    std::printf("P1_Gen_ContOutlines generate time: %.6f s (%u x %u) n=%u\n",
        sec,
        static_cast<u32>(r.m_w),
        static_cast<u32>(r.m_h),
        static_cast<u32>(r.m_n));
    char merge_path[320];
    if (!p1_tester_make_out(prm.m_seed, merge_path, sizeof(merge_path))) {
        std::printf("failed to ensure merge output path\n");
        return -1;
    }
    const u8* ov = r.m_ov.data();
    if (ov == nullptr || !save_cont_merge(merge_path, ov, r.m_w, r.m_h)) {
        std::printf("failed to save merge: %s\n", merge_path);
        return -1;
    }
    std::printf("saved: %s\n", merge_path);
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
    const i32 rc = test_p1_gen_cont_outlines_basic(prm);
    if (!p1_tester_whiteboard_chk()) {
        return -1;
    }
    return rc;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
