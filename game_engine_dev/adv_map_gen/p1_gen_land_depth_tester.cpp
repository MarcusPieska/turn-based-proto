//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_land_depth.h"
#include "p1_gen_land_depth_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;

static u32 count_coast (const u8* wl, const u16* dist, u32 n) {
    u32 c = 0u;
    if (wl == nullptr || dist == nullptr) {
        return 0u;
    }
    for (u32 i = 0; i < n; ++i) {
        if (wl[i] == WL_OVERLAY_LAND_GRAY && dist[i] == 0u) {
            ++c;
        }
    }
    return c;
}

static u16 max_land_depth (const u8* wl, const u16* dist, u32 n) {
    u16 mx = 0u;
    if (wl == nullptr || dist == nullptr) {
        return 0u;
    }
    for (u32 i = 0; i < n; ++i) {
        if (wl[i] != WL_OVERLAY_LAND_GRAY) {
            continue;
        }
        const u16 d = dist[i];
        if (d == k_inf) {
            continue;
        }
        if (d > mx) {
            mx = d;
        }
    }
    return mx;
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_land_depth_basic (P1_TesterHarness& h) {
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
    P1_Gen_LandDepth gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = gen.generate(ov, w, ht);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const P1_Gen_LandDepthRslt& r = gen.result();
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(ht);
    const u8* wl = r.m_wl.data();
    const u16* dist = r.m_dist.data();
    const u32 coast_n = count_coast(wl, dist, npx);
    const u16 depth_max = max_land_depth(wl, dist, npx);
    if (!ok || !gen.is_valid() || coast_n == 0u) {
        P1_RPrint::rprint_result_u32("P1_Gen_LandDepth", "Coast", coast_n, false);
        return -1;
    }
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state_u32("P1_Gen_LandDepth", "Coast", coast_n);
    P1_RPrint::rprint_state("P1_Gen_LandDepth", "MaxDepth", depth_max);
    P1_RPrint::rprint_state("Map", "W", w);
    P1_RPrint::rprint_state("Map", "H", ht);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    if (wl == nullptr || dist == nullptr || !P1_Gen_LandDepthView::save_pri(pri_path, wl, dist, w, ht)) {
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
    const i32 rc = test_p1_gen_land_depth_basic(h);
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
