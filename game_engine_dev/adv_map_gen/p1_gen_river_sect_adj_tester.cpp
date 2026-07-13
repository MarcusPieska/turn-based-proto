//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_river_sect_adj.h"
#include "p1_gen_river_sect_adj_view.h"
#include "p1_gen_river_sectors.h"
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
    out->m_ocn_sec_n = ec.m_pts_ocn_sec_n;
    out->m_que.clear();
    for (u32 pi = 0; pi < ec.m_pts_n; ++pi) {
        if (!out->m_que.push(ec.m_pts_que->x_at(pi), ec.m_pts_que->y_at(pi))) {
            return false;
        }
    }
    return out->m_que.ok();
}

static void log_adj_stats (const P1_Gen_RiverSectAdjRslt& adj) {
    u32 edge_n = 0;
    u8 max_nb = 0;
    u32 cap_n = 0;
    for (u16 si = 0; si < adj.m_sector_n; ++si) {
        const u8 nn = adj.m_nb_n[si];
        if (nn > max_nb) {
            max_nb = nn;
        }
        if (nn >= static_cast<u8>(P1_RIVER_SECT_ADJ_MAX)) {
            cap_n++;
        }
        edge_n += static_cast<u32>(nn);
    }
    edge_n /= 2u;
    char buf[96];
    std::snprintf(buf, sizeof(buf), "Edges: %u (max deg %u, at cap %u)", edge_n, static_cast<u32>(max_nb), cap_n);
    P1_RPrint::rprint_info(buf);
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_river_sect_adj_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step 11 input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_EarlyChainRslt& ec = h.early();
    if (ec.m_ter == nullptr || ec.m_pts_que == nullptr || !ec.m_pts_que->ok() || ec.m_pts_n == 0u || ec.m_w == 0 || ec.m_h == 0 || !p1_early_has_ocean(ec)) {
        P1_RPrint::rprint_info("Invalid early chain input for river sector adjacency");
        return -1;
    }
    P1_Gen_RiverPtsRslt pts_rslt;
    if (!copy_pts_rslt(ec, &pts_rslt)) {
        P1_RPrint::rprint_info("Failed to copy river pts from early chain");
        return -1;
    }
    const P1_OceanIndexRef ocn_ref = p1_early_ocean_ref(ec);
    P1_Gen_RiverSectors sec_gen(h.prm());
    if (!sec_gen.generate(ec.m_ter, ec.m_w, ec.m_h, pts_rslt, ocn_ref) || !sec_gen.is_valid()) {
        P1_RPrint::rprint_info("Failed to generate river sectors");
        return -1;
    }
    const P1_Gen_RiverSectorsRslt& sec_r = sec_gen.result();
    P1_Gen_RiverSectAdj gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = gen.generate(sec_r);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const P1_Gen_RiverSectAdjRslt& r = gen.result();
    if (!ok || !gen.is_valid() || r.m_sector_n == 0u || r.m_nb == nullptr || r.m_nb_n == nullptr) {
        P1_RPrint::rprint_result_u16("P1_Gen_RiverSectAdj", "Sectors", r.m_sector_n, false);
        return -1;
    }
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state("P1_Gen_RiverSectAdj", "Sectors", r.m_sector_n);
    log_adj_stats(r);
    P1_RPrint::rprint_state("Map", "W", ec.m_w);
    P1_RPrint::rprint_state("Map", "H", ec.m_h);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    if (!P1_Gen_RiverSectAdjView::save_pri(
            pri_path,
            ec.m_ter,
            ec.m_w,
            ec.m_h,
            h.seed(),
            sec_r.m_ov,
            sec_r.m_sector_n,
            r,
            pts_rslt)) {
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
    const i32 rc = test_p1_gen_river_sect_adj_basic(h);
    if (rc != 0) {
        return rc;
    }
    if (!h.finish()) {
        return -1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================