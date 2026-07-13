//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_gen_river_lines.h"
#include "p1_gen_river_lines_view.h"
#include "p1_gen_river_network_view.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_sect_adj.h"
#include "p1_gen_river_pts.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool copy_pts_rslt (const P1_EarlyChainRslt& ec, P1_Gen_RiverPtsRslt* out) {
    if (out == nullptr || ec.m_pts_que == nullptr || !ec.m_pts_que->ok() || ec.m_pts_n == 0u) {
        return false;
    }
    out->m_w = ec.m_w;
    out->m_h = ec.m_h;
    out->m_n = ec.m_pts_n;
    out->m_ocn_sec_n = ec.m_pts_ocn_sec_n;
    out->m_que.clear();
    for (u32 pi = 0; pi < ec.m_pts_n; ++pi) {
        if (!out->m_que.push(ec.m_pts_que->x_at(pi), ec.m_pts_que->y_at(pi))) {
            return false;
        }
    }
    return out->m_que.ok();
}

static bool stub_coast_lim (u16 w, u16 h, u16 sector_n, P1_Gen_CoastalMtnLimitsRslt* out) {
    if (out == nullptr || w == 0 || h == 0) {
        return false;
    }
    out->m_w = w;
    out->m_h = h;
    out->m_sector_n = sector_n;
    out->m_max_sec_dist = 0;
    out->m_limit_n = 0;
    out->m_sel_n = 0;
    if (!out->m_limit_ov.resize(w, h)) {
        return false;
    }
    u8* lim = out->m_limit_ov.data_w();
    if (lim == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(lim, 0, n);
    return true;
}

static u32 count_dov_land (const u8* dov, const u8* terrain, u32 n) {
    u32 c = 0u;
    for (u32 i = 0; i < n; ++i) {
        if (dov[i] != 0xFFu && terrain[i] != TERR_OCEAN[0] && terrain[i] != TERR_SEA[0]) {
            c++;
        }
    }
    return c;
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_river_lines_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step 11 input");
        return -1;
    }
    const P1_EarlyChainRslt& ec = h.early();
    if (ec.m_ter == nullptr || !p1_early_has_ocean(ec) || !p1_early_has_sectors(ec)) {
        P1_RPrint::rprint_info("Invalid early chain input for river lines");
        return -1;
    }
    P1_Gen_RiverPtsRslt pts_rslt;
    if (!copy_pts_rslt(ec, &pts_rslt)) {
        P1_RPrint::rprint_info("Failed to copy river pts from early chain");
        return -1;
    }
    const P1_Gen_RiverSectorsRslt sec_r = p1_early_sectors_rslt(ec);
    P1_Gen_CoastalMtnLimitsRslt lim_r;
    if (!stub_coast_lim(ec.m_w, ec.m_h, sec_r.m_sector_n, &lim_r)) {
        P1_RPrint::rprint_info("Failed to build stub coastal limits");
        return -1;
    }
    P1_Gen_RiverSectAdj adj_gen(h.prm());
    if (!adj_gen.generate(sec_r) || !adj_gen.is_valid()) {
        P1_RPrint::rprint_info("Failed to generate river sector adjacency");
        return -1;
    }
    P1_Gen_RiverNetwork net_gen(h.prm());
    if (!net_gen.generate(ec.m_ter, ec.m_w, ec.m_h, pts_rslt, sec_r, adj_gen.result(), lim_r, p1_early_ocean_ref(ec)) || !net_gen.is_valid()) {
        P1_RPrint::rprint_info("Failed to generate river network");
        return -1;
    }
    if (h.full()) {
        char net_path[320];
        if (!p1_make_out_path(h.seed(), "14_river_network.ppm", net_path, sizeof(net_path))) {
            P1_RPrint::rprint_info("Failed to ensure network output path");
            return -1;
        }
        if (!P1_Gen_RiverNetworkView::save_pri(net_path, ec.m_ter, ec.m_w, ec.m_h, h.seed(), net_gen.result(), pts_rslt)) {
            P1_RPrint::rprint_info("Failed to save network full prereq view");
            return -1;
        }
        char obuf[384];
        std::snprintf(obuf, sizeof(obuf), "Output: %s", net_path);
        P1_RPrint::rprint_info(obuf);
    }
    {
        char ibuf[64];
        std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
        P1_RPrint::rprint_info(ibuf);
    }
    P1_Gen_RiverLines lin_gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = lin_gen.generate(ec.m_ter, ec.m_w, ec.m_h, pts_rslt, sec_r, net_gen.result(), p1_early_ocean_ref(ec));
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const P1_Gen_RiverLinesRslt& r = lin_gen.result();
    if (!ok || !lin_gen.is_valid() || r.m_ov == nullptr || r.m_dov == nullptr) {
        P1_RPrint::rprint_info("P1_Gen_RiverLines generate failed");
        return -1;
    }
    const u32 npx = static_cast<u32>(ec.m_w) * static_cast<u32>(ec.m_h);
    const u32 dov_land = count_dov_land(r.m_dov, ec.m_ter, npx);
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state_u32("P1_Gen_RiverLines", "DistLand", dov_land);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    if (!P1_Gen_RiverLinesView::save_pri(pri_path, ec.m_ter, ec.m_w, ec.m_h, r, net_gen.result(), pts_rslt)) {
        P1_RPrint::rprint_info("Failed to save primary output");
        return -1;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", pri_path);
    P1_RPrint::rprint_info(obuf);
    if (h.has_sec()) {
        char sec_path[320];
        if (!h.path_sec(sec_path, sizeof(sec_path))) {
            P1_RPrint::rprint_info("Failed to ensure secondary output path");
            return -1;
        }
        if (!P1_Gen_RiverLinesView::save_dist(sec_path, ec.m_ter, ec.m_w, ec.m_h, r)) {
            P1_RPrint::rprint_info("Failed to save distance output");
            return -1;
        }
        std::snprintf(obuf, sizeof(obuf), "Output: %s", sec_path);
        P1_RPrint::rprint_info(obuf);
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
    const i32 rc = test_p1_gen_river_lines_basic(h);
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
