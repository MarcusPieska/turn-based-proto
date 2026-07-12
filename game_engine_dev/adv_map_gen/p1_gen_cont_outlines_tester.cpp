//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_cont_outlines.h"
#include "p1_gen_cont_outlines_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_cont_outlines_basic (P1_TesterHarness& h) {
    P1_Gen_ContOutlines gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = gen.generate();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        P1_RPrint::rprint_result_u16("P1_Gen_ContOutlines", "Continents", 0u, false);
        return -1;
    }
    const P1_Gen_ContOutlinesRslt& r = gen.result();
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state("P1_Gen_ContOutlines", "Continents", r.m_n);
    P1_RPrint::rprint_state("Map", "W", r.m_w);
    P1_RPrint::rprint_state("Map", "H", r.m_h);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    const u8* ov = r.m_ov.data();
    if (ov == nullptr || !P1_Gen_ContOutlinesView::save_pri(pri_path, ov, r.m_w, r.m_h)) {
        P1_RPrint::rprint_info("Failed to save primary output");
        return -1;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", pri_path);
    P1_RPrint::rprint_info(obuf);
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
    const i32 rc = test_p1_gen_cont_outlines_basic(h);
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
