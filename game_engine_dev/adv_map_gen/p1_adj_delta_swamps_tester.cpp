//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_adj_delta_swamps.h"
#include "p1_adj_delta_swamps_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
#include "p1_wb_util.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool save_canonical (P1_TesterHarness& h, const P1_MakeMapRslt& chain, const u8* clim, const u8* res_ov) {
    char pri_path[320];
    char sec_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return false;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    if (!P1_Adj_DeltaSwampsView::save_pri(pri_path, chain.m_terrain, clim, chain.m_rivers, res_ov, w, ht)) {
        P1_RPrint::rprint_info("Failed to save primary output");
        return false;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", pri_path);
    P1_RPrint::rprint_info(obuf);
    if (h.has_sec() && h.path_sec(sec_path, sizeof(sec_path))) {
        if (!P1_Adj_DeltaSwampsView::save_sec(sec_path, chain.m_terrain, clim, chain.m_rivers, w, ht)) {
            P1_RPrint::rprint_info("Failed to save secondary output");
            return false;
        }
        char sbuf[384];
        std::snprintf(sbuf, sizeof(sbuf), "Output: %s", sec_path);
        P1_RPrint::rprint_info(sbuf);
    }
    return true;
}

static bool save_full_extras (P1_TesterHarness& h, const P1_MakeMapRslt& chain) {
    char wshed_path[320];
    if (!h.path_extra(P1_DELTA_SWAMPS_VIZ_EXTRA_WSHED, wshed_path, sizeof(wshed_path))) {
        return false;
    }
    if (!P1_Adj_DeltaSwampsView::save_wshed_extra(
            wshed_path, h.seed(), chain.m_terrain, chain.m_wshed, chain.m_rivers, chain.m_w, chain.m_h)) {
        P1_RPrint::rprint_info("Failed to save watershed extra output");
        return false;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", wshed_path);
    P1_RPrint::rprint_info(obuf);
    return true;
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_adj_delta_swamps_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("P1_MakeMap steps 1-39 input failed for delta swamps");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_rivers == nullptr
        || chain.m_wshed == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        P1_RPrint::rprint_info("Invalid chain input for delta swamps");
        return -1;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(ht);
    Whiteboard_1B wb_clim("P1_Adj_DeltaSwampsTester", "clim", h.seed());
    Whiteboard_1B wb_ov("P1_Adj_DeltaSwampsTester", "res_ov", h.seed());
    P1_WB_CHK(wb_clim);
    P1_WB_CHK(wb_ov);
    u8* clim = wb_clim.raw();
    u8* res_ov = wb_ov.raw();
    if (clim == nullptr || res_ov == nullptr) {
        return -1;
    }
    std::memcpy(clim, chain.m_climate, static_cast<size_t>(n));
    if (chain.m_overlay != nullptr) {
        std::memcpy(res_ov, chain.m_overlay, static_cast<size_t>(n));
    } else {
        std::memset(res_ov, 0, static_cast<size_t>(n));
    }
    P1_Adj_DeltaSwampsIn in = {};
    in.m_wshed = chain.m_wshed;
    in.m_terrain = chain.m_terrain;
    in.m_climate = clim;
    in.m_res_ov = res_ov;
    in.m_riv = chain.m_rivers;
    const P1_Adj_DeltaSwampsPrm sp = p1_adj_delta_swamps_prm_from_cfg(h.cfg());
    P1_Adj_DeltaSwamps adj(h.prm(), sp);
    const clock_t t0 = clock();
    const bool ok = adj.adjust(in, w, ht);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        P1_RPrint::rprint_info("P1_Adj_DeltaSwamps failed to adjust");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state_u32("P1_Adj_DeltaSwamps", "Deltas", adj.delta_n());
    P1_RPrint::rprint_state_u32("P1_Adj_DeltaSwamps", "Swamps", adj.swamp_n());
    P1_RPrint::rprint_state("Map", "W", w);
    P1_RPrint::rprint_state("Map", "H", ht);
    if (!save_canonical(h, chain, clim, res_ov)) {
        return -1;
    }
    if (h.full() && !save_full_extras(h, chain)) {
        return -1;
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
    const i32 rc = test_p1_adj_delta_swamps_basic(h);
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
