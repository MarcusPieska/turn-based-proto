//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_adj_outline_fill.h"
#include "p1_adj_outline_fill_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
#include "p1_wb_util.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static u32 count_land_ov (const u8* ov, u32 n) {
    u32 c = 0u;
    if (ov == nullptr) {
        return 0u;
    }
    for (u32 i = 0; i < n; ++i) {
        if (ov[i] == WL_OVERLAY_LAND_GRAY) {
            ++c;
        }
    }
    return c;
}

static u32 count_plains (const u8* ter, u32 n) {
    u32 c = 0u;
    if (ter == nullptr) {
        return 0u;
    }
    const u8 plains = TERR_PLAINS[0];
    for (u32 i = 0; i < n; ++i) {
        if (ter[i] == plains) {
            ++c;
        }
    }
    return c;
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_adj_outline_fill_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step 1 input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_EarlyChainRslt& ec = h.early();
    const u16 w = ec.m_w;
    const u16 ht = ec.m_h;
    const u8* ov = ec.m_ov.data();
    if (ov == nullptr || w == 0 || ht == 0) {
        P1_RPrint::rprint_info("Invalid outline overlay from early chain");
        return -1;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(ht);
    const u32 land_n = count_land_ov(ov, npx);
    Whiteboard_1B wb_ter("P1_Adj_OutlineFillTester", "Terrain", h.prm().m_seed);
    P1_WB_CHK(wb_ter);
    u8* terrain = wb_ter.get_iter_ptr();
    P1_Adj_OutlineFill adj(h.prm());
    const clock_t t0 = clock();
    const bool ok = adj.adjust(terrain, w, ht, ov);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const u32 plains_n = count_plains(terrain, npx);
    const bool match = ok && adj.is_valid() && plains_n == land_n && land_n > 0u;
    if (!match) {
        P1_RPrint::rprint_result_u32("P1_Adj_OutlineFill", "Plains", plains_n, false);
        return -1;
    }
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state_u32("P1_Adj_OutlineFill", "Plains", plains_n);
    P1_RPrint::rprint_state("Map", "W", w);
    P1_RPrint::rprint_state("Map", "H", ht);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    if (!P1_Adj_OutlineFillView::save_pri(pri_path, terrain, w, ht)) {
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
    const i32 rc = test_p1_adj_outline_fill_basic(h);
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
