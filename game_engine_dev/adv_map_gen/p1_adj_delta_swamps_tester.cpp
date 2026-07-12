//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_adj_delta_swamps.h"
#include "p1_adj_delta_swamps_view.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

static bool save_canonical (P1_TesterHarness& h, const P1_MakeMapRslt& chain, const u8* clim, const u8* res_ov) {
    char pri_path[320];
    char sec_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        std::printf("failed to ensure primary output path\n");
        return false;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    if (!P1_Adj_DeltaSwampsView::save_pri(pri_path, chain.m_terrain, clim, chain.m_rivers, res_ov, w, ht)) {
        std::printf("failed to save map: %s\n", pri_path);
        return false;
    }
    std::printf("saved: %s\n", pri_path);
    if (h.has_sec() && h.path_sec(sec_path, sizeof(sec_path))) {
        if (!P1_Adj_DeltaSwampsView::save_sec(sec_path, chain.m_terrain, clim, chain.m_rivers, w, ht)) {
            std::printf("failed to save map: %s\n", sec_path);
            return false;
        }
        std::printf("saved: %s\n", sec_path);
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
        std::printf("failed to save map: %s\n", wshed_path);
        return false;
    }
    std::printf("saved: %s\n", wshed_path);
    return true;
}

i32 test_p1_adj_delta_swamps_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        std::printf("P1 steps 1-33 chain failed for delta swamps\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_rivers == nullptr
        || chain.m_wshed == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for delta swamps\n");
        return -1;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(ht);
    u8* clim = new u8[n];
    u8* res_ov = new u8[n];
    if (clim == nullptr || res_ov == nullptr) {
        delete[] res_ov;
        delete[] clim;
        return -1;
    }
    std::memcpy(clim, chain.m_climate, static_cast<size_t>(n));
    std::memset(res_ov, 0, static_cast<size_t>(n));
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
        std::printf("P1_Adj_DeltaSwamps failed to adjust\n");
        delete[] res_ov;
        delete[] clim;
        return -1;
    }
    std::printf("P1 steps 1-33 input time: %.6f s\n", h.input_sec());
    std::printf(
        "P1_Adj_DeltaSwamps adjust time: %.6f s (deltas %u swamps %u, %u x %u)\n",
        sec,
        adj.delta_n(),
        adj.swamp_n(),
        static_cast<u32>(w),
        static_cast<u32>(ht));
    if (!save_canonical(h, chain, clim, res_ov)) {
        delete[] res_ov;
        delete[] clim;
        return -1;
    }
    if (h.full() && !save_full_extras(h, chain)) {
        delete[] res_ov;
        delete[] clim;
        return -1;
    }
    delete[] res_ov;
    delete[] clim;
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
