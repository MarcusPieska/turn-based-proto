//================================================================================================================================
//= - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "r1_adj_food_crop_placements.h"
#include "r1_gen_empty_resource_overlay.h"
#include "r1_gen_res_sectors.h"
#include "land_mass_index.h"
#include "res_dist_state.h"
#include "res_statics.h"
#include "resource_static_key.h"
#include "p1_make_map.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//= - Test -
//================================================================================================================================

i32 test_r1_adj_food_crop_placements (P1_TesterHarness& h) {
    if (!h.run_input()) {
        std::printf("P1_MakeMap input failed for food_crop placements\n");
        return -1;
    }
    if (!ResStatics::is_ready()) {
        std::printf("failed to load statics\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for food_crop placements\n");
        return -1;
    }
    R1_Gen_EmptyResourceOverlay empty;
    if (!empty.generate(chain.m_w, chain.m_h) || !empty.is_valid()) {
        std::printf("R1_Gen_EmptyResourceOverlay failed\n");
        return -1;
    }
    LandMassIndex mass;
    if (!mass.generate(chain.m_terrain, chain.m_w, chain.m_h) || !mass.is_valid()) {
        std::printf("LandMassIndex failed\n");
        return -1;
    }
    R1_Gen_ResSectors sectors;
    if (!sectors.generate(chain.m_terrain, chain.m_w, chain.m_h, mass.result(), h.prm().m_seed,
            static_cast<u16>(R1_RES_SECTOR_PCT_DEF)) || !sectors.is_valid()) {
        std::printf("R1_Gen_ResSectors failed\n");
        return -1;
    }
    const R1_Gen_ResSectorsRslt& sr = sectors.result();
    ResPlcMapCtx ctx = {};
    ctx.m_w = chain.m_w;
    ctx.m_h = chain.m_h;
    ctx.m_terrain = chain.m_terrain;
    ctx.m_climate = chain.m_climate;
    ctx.m_river = chain.m_rivers;
    ctx.m_overlay = chain.m_overlay;
    P1_MakeMapPrm mp = p1_make_map_prm_def();
    ResDistState st;
    if (!st.reset(ResStatics::shared().resource().get_item_count())) {
        std::printf("ResDistState reset failed\n");
        return -1;
    }
    R1_Adj_FoodCropPlacements adj;
    const clock_t t0 = clock();
    const bool ok = adj.adjust(empty.overlay_mut(), ResStatics::shared(), st, ctx, sr.m_ov, sr.m_sec_n,
            mp.m_res_base_n, h.prm().m_seed) && adj.is_valid();
    const clock_t t1 = clock();
    const double ms = (static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC)) * 1000.0;
    if (!ok) {
        std::printf("R1_Adj_FoodCropPlacements failed\n");
        return -1;
    }
    std::printf("P1_MakeMap input time: %.6f s\n", h.input_sec());
    std::printf("R1_Adj_FoodCropPlacements time: %.2f ms (crps=%u secs=%u asg=%u plcs=%u)\n",
        ms, (unsigned)adj.crp_n(), (unsigned)adj.sec_n(), (unsigned)adj.asg_n(), adj.plc_n());
    const RuntimeStatics& rt = ResStatics::shared();
    const u16* ov = empty.overlay();
    const u32 npx = static_cast<u32>(chain.m_w) * static_cast<u32>(chain.m_h);
    for (u16 gi = 0; gi < adj.crp_n(); ++gi) {
        const u16 res_i = adj.crp_at(gi);
        u32 cnt = 0;
        for (u32 i = 0; i < npx; ++i) {
            if (ov[i] == res_i) {
                cnt = cnt + 1u;
            }
        }
        cstr nm = rt.resource().get_name(ResourceStaticDataKey::from_raw(res_i));
        if (nm == nullptr) {
            nm = "unknown";
        }
        std::printf("placed %s (idx=%u): %u (state=%u)\n", nm, (unsigned)res_i, cnt, st.get(res_i));
        char fname[160];
        std::snprintf(fname, sizeof(fname), "47_crp_%s.ppm", nm);
        char out_path[320];
        if (!p1_make_out_path(h.prm().m_seed, fname, out_path, sizeof(out_path))) {
            std::printf("failed to ensure path for %s\n", nm);
            return -1;
        }
        if (!adj.save_res_ppm(out_path, chain.m_terrain, ov, res_i)) {
            std::printf("failed to save: %s\n", out_path);
            return -1;
        }
        std::printf("saved: %s\n", out_path);
    }
    return 0;
}

//================================================================================================================================
//= - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    P1_TesterHarness h;
    if (!h.begin(argc, argv)) {
        return -1;
    }
    const i32 rc = test_r1_adj_food_crop_placements(h);
    if (rc != 0) {
        return rc;
    }
    if (!h.finish()) {
        return -1;
    }
    return 0;
}

//================================================================================================================================
//= - End of file -
//================================================================================================================================
