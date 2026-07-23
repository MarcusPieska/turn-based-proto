//================================================================================================================================
//= - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "r1_adj_general_placement.h"
#include "r1_adj_gemstone_placements.h"
#include "r1_adj_livestock_placements.h"
#include "r1_adj_food_crop_placements.h"
#include "r1_adj_metal_placements.h"
#include "r1_adj_spice_placements.h"
#include "r1_adj_cash_crop_placements.h"
#include "r1_adj_produce_placements.h"
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

i32 test_r1_adj_general_placement (P1_TesterHarness& h) {
    char out_path[320];
    if (!h.path_pri(out_path, sizeof(out_path))) {
        std::printf("failed to ensure output path\n");
        return -1;
    }
    if (!h.run_input()) {
        std::printf("P1_MakeMap input failed for general placement\n");
        return -1;
    }
    if (!ResStatics::is_ready()) {
        std::printf("failed to load statics\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_overlay == nullptr
        || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for general placement\n");
        return -1;
    }
    R1_Gen_EmptyResourceOverlay empty;
    if (!empty.generate(chain.m_w, chain.m_h) || !empty.is_valid()) {
        std::printf("R1_Gen_EmptyResourceOverlay failed\n");
        return -1;
    }
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
    LandMassIndex mass;
    if (!mass.generate(chain.m_terrain, chain.m_w, chain.m_h) || !mass.is_valid()) {
        std::printf("LandMassIndex failed\n");
        return -1;
    }
    R1_Gen_ResSectors def_sec;
    if (!def_sec.generate(chain.m_terrain, chain.m_w, chain.m_h, mass.result(), h.prm().m_seed,
            static_cast<u16>(R1_RES_SECTOR_PCT_DEF)) || !def_sec.is_valid()) {
        std::printf("R1_Gen_ResSectors failed\n");
        return -1;
    }
    R1_Adj_GemstonePlacements gems;
    if (!gems.adjust(empty.overlay_mut(), ResStatics::shared(), st, ctx, def_sec.result().m_ov,
            def_sec.result().m_sec_n, mp.m_res_base_n, h.prm().m_seed) || !gems.is_valid()) {
        std::printf("R1_Adj_GemstonePlacements failed\n");
        return -1;
    }
    const u32 gem_plcs = gems.plc_n();
    R1_Adj_LivestockPlacements livs;
    if (!livs.adjust(empty.overlay_mut(), ResStatics::shared(), st, ctx, def_sec.result().m_ov,
            def_sec.result().m_sec_n, mp.m_res_base_n, h.prm().m_seed) || !livs.is_valid()) {
        std::printf("R1_Adj_LivestockPlacements failed\n");
        return -1;
    }
    const u32 liv_plcs = livs.plc_n();
    R1_Adj_FoodCropPlacements crops;
    if (!crops.adjust(empty.overlay_mut(), ResStatics::shared(), st, ctx, def_sec.result().m_ov,
            def_sec.result().m_sec_n, mp.m_res_base_n, h.prm().m_seed) || !crops.is_valid()) {
        std::printf("R1_Adj_FoodCropPlacements failed\n");
        return -1;
    }
    const u32 crp_plcs = crops.plc_n();
    R1_Gen_ResSectors met_sec;
    if (!met_sec.generate(chain.m_terrain, chain.m_w, chain.m_h, mass.result(), h.prm().m_seed,
            static_cast<u16>(R1_RES_SECTOR_PCT_MET)) || !met_sec.is_valid()) {
        std::printf("R1_Gen_ResSectors (metal) failed\n");
        return -1;
    }
    R1_Adj_MetalPlacements mets;
    if (!mets.adjust(empty.overlay_mut(), ResStatics::shared(), st, ctx, met_sec.result().m_ov,
            met_sec.result().m_sec_n, mp.m_res_base_n, h.prm().m_seed) || !mets.is_valid()) {
        std::printf("R1_Adj_MetalPlacements failed\n");
        return -1;
    }
    const u32 met_plcs = mets.plc_n();
    R1_Adj_SpicePlacements spices;
    if (!spices.adjust(empty.overlay_mut(), ResStatics::shared(), st, ctx, met_sec.result().m_ov,
            met_sec.result().m_sec_n, mp.m_res_base_n, h.prm().m_seed) || !spices.is_valid()) {
        std::printf("R1_Adj_SpicePlacements failed\n");
        return -1;
    }
    const u32 spc_plcs = spices.plc_n();
    R1_Adj_CashCropPlacements cash;
    if (!cash.adjust(empty.overlay_mut(), ResStatics::shared(), st, ctx, met_sec.result().m_ov,
            met_sec.result().m_sec_n, mp.m_res_base_n, h.prm().m_seed) || !cash.is_valid()) {
        std::printf("R1_Adj_CashCropPlacements failed\n");
        return -1;
    }
    const u32 csh_plcs = cash.plc_n();
    R1_Adj_ProducePlacements prod;
    if (!prod.adjust(empty.overlay_mut(), ResStatics::shared(), st, ctx, met_sec.result().m_ov,
            met_sec.result().m_sec_n, mp.m_res_base_n, h.prm().m_seed) || !prod.is_valid()) {
        std::printf("R1_Adj_ProducePlacements failed\n");
        return -1;
    }
    const u32 prd_plcs = prod.plc_n();
    R1_Adj_GeneralPlacement gen;
    const clock_t t0 = clock();
    const bool ok = gen.adjust(empty.overlay_mut(), chain.m_w, chain.m_h, ctx, ResStatics::shared(), st,
            mp.m_res_base_n, h.prm().m_seed) && gen.is_valid();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok) {
        std::printf("R1_Adj_GeneralPlacement failed\n");
        return -1;
    }
    u32 placed = 0;
    const u16* ov = empty.overlay();
    const u32 npx = static_cast<u32>(empty.width()) * static_cast<u32>(empty.height());
    for (u32 i = 0; i < npx; ++i) {
        if (ov[i] != U16_KEY_NULL) {
            placed = placed + 1u;
        }
    }
    std::printf("P1_MakeMap input time: %.6f s\n", h.input_sec());
    std::printf("gems: %u; livestock: %u; food_crop: %u; metals: %u; spice: %u; cash_crop: %u; produce: %u; general added: %u; overlay occupied: %u\n",
        gem_plcs, liv_plcs, crp_plcs, met_plcs, spc_plcs, csh_plcs, prd_plcs, gen.placed_n(), placed);
    std::printf("R1_Adj_GeneralPlacement time: %.6f s (%u x %u)\n",
        sec, (u32)chain.m_w, (u32)chain.m_h);
    for (u16 gi = 0; gi < gems.gem_n(); ++gi) {
        const u16 res_i = gems.gem_at(gi);
        u32 cnt = 0;
        for (u32 i = 0; i < npx; ++i) {
            if (ov[i] == res_i) {
                cnt = cnt + 1u;
            }
        }
        cstr nm = ResStatics::shared().resource().get_name(ResourceStaticDataKey::from_raw(res_i));
        if (nm == nullptr) {
            nm = "unknown";
        }
        std::printf("gem after general %s: ov=%u state=%u\n", nm, cnt, st.get(res_i));
        if (cnt != st.get(res_i)) {
            std::printf("gem state/overlay mismatch\n");
            return -1;
        }
    }
    if (!R1_Gen_EmptyResourceOverlay::save_ppm(out_path, ctx, ov, empty.width(), empty.height(), 255u)) {
        std::printf("failed to save map: %s\n", out_path);
        return -1;
    }
    std::printf("saved: %s\n", out_path);
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
    const i32 rc = test_r1_adj_general_placement(h);
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
