//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_river_prob.h"
#include "p1_gen_river_prob_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_river_prob_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step 7 input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_EarlyChainRslt& ec = h.early();
    if (ec.m_ter == nullptr || ec.m_w == 0 || ec.m_h == 0) {
        P1_RPrint::rprint_info("Invalid shaped terrain from early chain");
        return -1;
    }
    P1_Gen_RiverProb gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = gen.generate(ec.m_ter, ec.m_w, ec.m_h);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const P1_Gen_RiverProbRslt& r = gen.result();
    if (!ok || !gen.is_valid() || r.m_prob == nullptr || r.m_wat_dist == nullptr || r.m_eq_near == nullptr || r.m_wgt_sum == nullptr) {
        P1_RPrint::rprint_info("P1_Gen_RiverProb generate failed");
        return -1;
    }
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state("Map", "W", ec.m_w);
    P1_RPrint::rprint_state("Map", "H", ec.m_h);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    if (!P1_Gen_RiverProbView::save_pri(pri_path, ec.m_ter, ec.m_w, ec.m_h, r.m_prob)) {
        P1_RPrint::rprint_info("Failed to save primary output");
        return -1;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", pri_path);
    P1_RPrint::rprint_info(obuf);
    char sec_path[320];
    if (!h.path_sec(sec_path, sizeof(sec_path))) {
        P1_RPrint::rprint_info("Failed to ensure secondary output path");
        return -1;
    }
    if (!P1_Gen_RiverProbView::save_wat(sec_path, ec.m_w, ec.m_h, r.m_wat_dist)) {
        P1_RPrint::rprint_info("Failed to save water distance output");
        return -1;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", sec_path);
    P1_RPrint::rprint_info(obuf);
    char eq_path[320];
    if (!p1_make_out_path(h.seed(), "08_river_prob_eq.ppm", eq_path, sizeof(eq_path))) {
        P1_RPrint::rprint_info("Failed to ensure equator output path");
        return -1;
    }
    if (!P1_Gen_RiverProbView::save_eq(eq_path, ec.m_w, ec.m_h, r.m_eq_near)) {
        P1_RPrint::rprint_info("Failed to save equator nearness output");
        return -1;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", eq_path);
    P1_RPrint::rprint_info(obuf);
    char wgt_path[320];
    if (!p1_make_out_path(h.seed(), "08_river_prob_wgt.ppm", wgt_path, sizeof(wgt_path))) {
        P1_RPrint::rprint_info("Failed to ensure weighted output path");
        return -1;
    }
    if (!P1_Gen_RiverProbView::save_wgt(wgt_path, ec.m_w, ec.m_h, r.m_wgt_sum)) {
        P1_RPrint::rprint_info("Failed to save weighted output");
        return -1;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", wgt_path);
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
    const i32 rc = test_p1_gen_river_prob_basic(h);
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
