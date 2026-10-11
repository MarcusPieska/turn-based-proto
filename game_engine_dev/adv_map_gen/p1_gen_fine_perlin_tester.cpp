//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_fine_perlin.h"
#include "p1_gen_noise_perlin_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
#include "p1_wb_util.h"

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_fine_perlin_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_EarlyChainRslt& ec = h.early();
    if (ec.m_perlin_f32 == nullptr || ec.m_w == 0 || ec.m_h == 0) {
        P1_RPrint::rprint_info("Invalid perlin from early chain");
        return -1;
    }
    Whiteboard_4B wb_fine("P1_Gen_FinePerlin", "fine", h.seed());
    P1_WB_CHK(wb_fine);
    f32* fine = reinterpret_cast<f32*>(wb_fine.get_iter_ptr());
    P1_Gen_FinePerlin gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = gen.generate(ec.m_perlin_f32, ec.m_w, ec.m_h, fine);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        P1_RPrint::rprint_info("P1_Gen_FinePerlin generate failed");
        return -1;
    }
    const P1_Gen_FinePerlinRslt& r = gen.result();
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state("Map", "W", r.m_w);
    P1_RPrint::rprint_state("Map", "H", r.m_h);
    P1_RPrint::rprint_state("P1_Gen_FinePerlin", "TW", r.m_tw);
    P1_RPrint::rprint_state("P1_Gen_FinePerlin", "TH", r.m_th);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    const u8* gray = r.m_ov.data();
    if (gray == nullptr || !P1_Gen_NoisePerlinView::save_pri(pri_path, gray, r.m_w, r.m_h)) {
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
    const i32 rc = test_p1_gen_fine_perlin_basic(h);
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
