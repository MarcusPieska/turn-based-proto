//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <ctime>

#include "game_map_defs.h"
#include "generator_constants.h"
#include "p1_gen_shaped_outline.h"
#include "p1_gen_shaped_outline_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static inline bool ov_is_shelf (const u8* ov, u32 i) {
    return ov[i] == WL_OVERLAY_LAND_GRAY;
}

static u32 count_shelf_plains (const u8* terrain, const u8* ov, u32 n) {
    u32 c = 0u;
    if (terrain == nullptr || ov == nullptr) {
        return 0u;
    }
    for (u32 i = 0; i < n; ++i) {
        if (ov_is_shelf(ov, i) && terrain[i] == TERR_PLAINS[0]) {
            ++c;
        }
    }
    return c;
}

static u32 count_shelf (const u8* ov, u32 n) {
    u32 c = 0u;
    if (ov == nullptr) {
        return 0u;
    }
    for (u32 i = 0; i < n; ++i) {
        if (ov_is_shelf(ov, i)) {
            ++c;
        }
    }
    return c;
}

static bool save_near_far (u32 seed, const u8* near_ter, const u8* far_ter, u16 w, u16 h) {
    char path[320];
    char obuf[384];
    if (near_ter == nullptr || far_ter == nullptr) {
        return false;
    }
    if (!p1_make_out_path(seed, "05_shaped_outline_near.ppm", path, sizeof(path))) {
        return false;
    }
    if (!P1_Gen_ShapedOutlineView::save_near(path, near_ter, w, h)) {
        return false;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", path);
    P1_RPrint::rprint_info(obuf);
    if (!p1_make_out_path(seed, "06_shaped_outline_far.ppm", path, sizeof(path))) {
        return false;
    }
    if (!P1_Gen_ShapedOutlineView::save_far(path, far_ter, w, h)) {
        return false;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", path);
    P1_RPrint::rprint_info(obuf);
    return true;
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_shaped_outline_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step 4 input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_EarlyChainRslt& ec = h.early();
    const u16 w = ec.m_w;
    const u16 ht = ec.m_h;
    const u8* ov = ec.m_ov.data();
    const u16* land_depth = ec.m_ld_dist.data();
    if (ov == nullptr || land_depth == nullptr || ec.m_fill_ter == nullptr
        || ec.m_perlin_f32 == nullptr || w == 0 || ht == 0) {
        P1_RPrint::rprint_info("Invalid early chain input for shaped outline");
        return -1;
    }
    const P1_Gen_ShapedOutlinePrm sp = p1_gen_shaped_outline_prm_from_cfg(h.cfg());
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(ht);
    u8* near_ter = new u8[npx];
    u8* far_ter = new u8[npx];
    u8* merged = new u8[npx];
    if (near_ter == nullptr || far_ter == nullptr || merged == nullptr) {
        delete[] merged;
        delete[] far_ter;
        delete[] near_ter;
        return -1;
    }
    std::memcpy(near_ter, ec.m_fill_ter, static_cast<size_t>(npx));
    std::memcpy(far_ter, ec.m_fill_ter, static_cast<size_t>(npx));
    std::memcpy(merged, ec.m_fill_ter, static_cast<size_t>(npx));
    P1_Gen_ShapedOutline gen(h.prm());
    gen.bind_perlin_field(ec.m_perlin_f32, w, ht);
    const clock_t t0 = clock();
    const bool ok_near = gen.generate_layer(near_ter, w, ht, ov, land_depth, sp.m_radial_near, sp.m_shelf_near);
    const bool ok_far = gen.generate_layer(far_ter, w, ht, ov, land_depth, sp.m_radial_far, sp.m_shelf_far);
    const bool ok_merge = ok_near && ok_far
        && gen.merge_layers(merged, w, ht, ov, land_depth, near_ter, far_ter);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const u32 shelf_n = count_shelf(ov, npx);
    const u32 plains_n = count_shelf_plains(merged, ov, npx);
    if (!ok_merge || !gen.is_valid() || shelf_n == 0u) {
        P1_RPrint::rprint_result_u32("P1_Gen_ShapedOutline", "Shelf", shelf_n, false);
        delete[] merged;
        delete[] far_ter;
        delete[] near_ter;
        return -1;
    }
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state_u32("P1_Gen_ShapedOutline", "Shelf", shelf_n);
    P1_RPrint::rprint_state_u32("P1_Gen_ShapedOutline", "Plains", plains_n);
    P1_RPrint::rprint_state("Map", "W", w);
    P1_RPrint::rprint_state("Map", "H", ht);
    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        delete[] merged;
        delete[] far_ter;
        delete[] near_ter;
        return -1;
    }
    if (!P1_Gen_ShapedOutlineView::save_pri(pri_path, merged, w, ht)) {
        P1_RPrint::rprint_info("Failed to save primary output");
        delete[] merged;
        delete[] far_ter;
        delete[] near_ter;
        return -1;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", pri_path);
    P1_RPrint::rprint_info(obuf);
    if (h.full() && !save_near_far(h.seed(), near_ter, far_ter, w, ht)) {
        P1_RPrint::rprint_info("Failed to save near/far outputs");
        delete[] merged;
        delete[] far_ter;
        delete[] near_ter;
        return -1;
    }
    delete[] merged;
    delete[] far_ter;
    delete[] near_ter;
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
    const i32 rc = test_p1_gen_shaped_outline_basic(h);
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
