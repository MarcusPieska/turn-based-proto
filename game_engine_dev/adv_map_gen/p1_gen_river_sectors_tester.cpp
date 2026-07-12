//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_river_sectors_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"

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
    out->m_que.clear();
    for (u32 pi = 0; pi < ec.m_pts_n; ++pi) {
        if (!out->m_que.push(ec.m_pts_que->x_at(pi), ec.m_pts_que->y_at(pi))) {
            return false;
        }
    }
    return out->m_que.ok();
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_river_sectors_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step 8 input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_EarlyChainRslt& ec = h.early();
    if (ec.m_ter == nullptr || ec.m_pts_que == nullptr || !ec.m_pts_que->ok() || ec.m_pts_n == 0u || ec.m_w == 0 || ec.m_h == 0) {
        P1_RPrint::rprint_info("Invalid early chain input for river sectors");
        return -1;
    }
    P1_Gen_RiverPtsRslt pts_rslt;
    if (!copy_pts_rslt(ec, &pts_rslt)) {
        P1_RPrint::rprint_info("Failed to copy river pts from early chain");
        return -1;
    }
    P1_Gen_RiverSectors gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = gen.generate(ec.m_ter, ec.m_w, ec.m_h, pts_rslt);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const P1_Gen_RiverSectorsRslt& r = gen.result();
    if (!ok || !gen.is_valid() || r.m_sector_n == 0u || r.m_ov == nullptr) {
        P1_RPrint::rprint_result_u16("P1_Gen_RiverSectors", "Sectors", r.m_sector_n, false);
        return -1;
    }
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state("P1_Gen_RiverSectors", "Sectors", r.m_sector_n);
    P1_RPrint::rprint_state_u32("P1_Gen_RiverPts", "Pts", pts_rslt.m_n);
    P1_RPrint::rprint_state("Map", "W", ec.m_w);
    P1_RPrint::rprint_state("Map", "H", ec.m_h);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    if (!P1_Gen_RiverSectorsView::save_pri(
            pri_path,
            ec.m_ter,
            ec.m_w,
            ec.m_h,
            h.seed(),
            r.m_ov,
            r.m_sector_n)) {
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
    const i32 rc = test_p1_gen_river_sectors_basic(h);
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
