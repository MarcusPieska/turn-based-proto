//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_rich_coast_fertility.h"
#include "p1_gen_rich_coast_fertility_view.h"
#include "p1_make_map.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool save_canonical (P1_TesterHarness& h, const P1_MakeMapRslt& chain, const u16* fert, u16 fert_peak) {
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return false;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    if (!P1_Gen_RichCoastFertilityView::save_pri(
            pri_path, h.seed(), chain.m_terrain, chain.m_climate, fert, fert_peak, w, ht)) {
        P1_RPrint::rprint_info("Failed to save primary output");
        return false;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", pri_path);
    P1_RPrint::rprint_info(obuf);
    return true;
}

static bool save_full_extras (P1_TesterHarness& h, const P1_Gen_RichCoastFertility& gen) {
    char brush_path[320];
    if (!h.path_extra(P1_RICH_COAST_VIZ_EXTRA_BRUSH, brush_path, sizeof(brush_path))) {
        return false;
    }
    gen.save_brush(brush_path);
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", brush_path);
    P1_RPrint::rprint_info(obuf);
    return true;
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_rich_coast_fertility_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("P1_MakeMap steps 1-30 input failed for rich coast fertility");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        P1_RPrint::rprint_info("Invalid chain input for rich coast fertility");
        return -1;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    const P1_Gen_RichCoastFertilityPrm fp = p1_make_map_prm_def().m_rich_coast;
    P1_Gen_RichCoastFertility gen(h.prm(), fp);
    const clock_t t0 = clock();
    const bool ok = gen.generate(chain.m_terrain, w, ht);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        P1_RPrint::rprint_info("P1_Gen_RichCoastFertility failed to generate");
        return -1;
    }
    const P1_Gen_RichCoastFertilityRslt& r = gen.result();
    const u16* fert = r.m_ov;
    if (fert == nullptr) {
        P1_RPrint::rprint_info("Missing fertility overlay");
        return -1;
    }
    u32 land_nz = 0;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(ht);
    for (u32 i = 0; i < npx; ++i) {
        if (is_land(chain.m_terrain[i]) && fert[i] > 0) {
            land_nz++;
        }
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state("P1_Gen_RichCoastFertility", "Peak", r.m_peak);
    P1_RPrint::rprint_state_u32("P1_Gen_RichCoastFertility", "LandTiles", land_nz);
    P1_RPrint::rprint_state("P1_Gen_RichCoastFertility", "BrushW", gen.brush_w());
    P1_RPrint::rprint_state("P1_Gen_RichCoastFertility", "BrushH", gen.brush_h());
    P1_RPrint::rprint_state("P1_Gen_RichCoastFertility", "BrushRad", fp.m_brush_rad);
    P1_RPrint::rprint_state("P1_Gen_RichCoastFertility", "StampLim", fp.m_stamp_lim);
    P1_RPrint::rprint_state("Map", "W", w);
    P1_RPrint::rprint_state("Map", "H", ht);
    if (!save_canonical(h, chain, fert, r.m_peak)) {
        return -1;
    }
    if (h.full() && !save_full_extras(h, gen)) {
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
    const i32 rc = test_p1_gen_rich_coast_fertility_basic(h);
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
