//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_noise_perlin.h"
#include "p1_gen_noise_perlin_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_noise_perlin_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step 3 input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_EarlyChainRslt& ec = h.early();
    const u16 w = ec.m_w;
    const u16 ht = ec.m_h;
    if (w == 0 || ht == 0) {
        P1_RPrint::rprint_info("Invalid map size from early chain");
        return -1;
    }
    P1_Gen_NoisePerlin gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = gen.generate();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const P1_Gen_NoisePerlinRslt& r = gen.result();
    const u16 layers = static_cast<u16>(r.m_prm.m_layer_count);
    if (!ok || !gen.is_valid()) {
        P1_RPrint::rprint_result_u16("P1_Gen_NoisePerlin", "Layers", layers, false);
        return -1;
    }
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state("P1_Gen_NoisePerlin", "Layers", layers);
    P1_RPrint::rprint_state("Map", "W", w);
    P1_RPrint::rprint_state("Map", "H", ht);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    const u8* gray = r.m_ov.data();
    if (gray == nullptr || !P1_Gen_NoisePerlinView::save_pri(pri_path, gray, w, ht)) {
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
    const i32 rc = test_p1_gen_noise_perlin_basic(h);
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
