//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_shaped_outline.h"
#include "p1_adj_outline_fill.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_outline.h"
#include "p1_gen_river_pts.h"
#include "game_primitives.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool build_step5_terrain (
    const P1_RunPrm& prm,
    u8* terrain,
    u16 w,
    u16 h,
    const u8* ov,
    const u16* land_depth) {
    P1_Adj_OutlineFill fill(prm);
    if (!fill.adjust(terrain, w, h, ov) || !fill.is_valid()) {
        return false;
    }
    const P1_Gen_ShapedOutlinePrm sp = p1_gen_shaped_outline_prm_def();
    return p1_apply_shaped_outline(prm, sp, terrain, w, h, ov, land_depth);
}

i32 test_p1_gen_river_pts_basic (const P1_RunPrm& prm) {
    char out_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    const clock_t t0i = clock();
    P1_Gen_Outline ol_gen(prm);
    if (!ol_gen.generate() || !ol_gen.is_valid()) {
        std::printf("P1_Gen_Outline failed for step 8 input\n");
        return -1;
    }
    const P1_Gen_OutlineRslt& ol = ol_gen.result();
    const u16 w = ol.m_w;
    const u16 h = ol.m_h;
    const u8* ov = ol.m_ov.data();
    if (ov == nullptr || w == 0 || h == 0) {
        std::printf("invalid outline overlay\n");
        return -1;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        std::printf("P1_Gen_LandDepth failed for step 8 input\n");
        return -1;
    }
    const u16* land_depth = depth_gen.result().m_dist.data();
    if (land_depth == nullptr) {
        std::printf("invalid land depth data\n");
        return -1;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return -1;
    }
    if (!build_step5_terrain(prm, terrain, w, h, ov, land_depth)) {
        std::printf("P1_Gen_ShapedOutline failed for step 8 input\n");
        delete[] terrain;
        return -1;
    }
    const clock_t t1i = clock();
    P1_Gen_RiverPts gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate(terrain, w, h);
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_RiverPts failed to generate\n");
        delete[] terrain;
        return -1;
    }
    const P1_Gen_RiverPtsRslt& r = gen.result();
    std::printf("P1 steps 1-7 input time: %.6f s\n", sec_i);
    std::printf("P1_Gen_RiverPts generate time: %.6f s (%u pts, %u x %u)\n",
        sec,
        r.m_n,
        static_cast<u32>(w),
        static_cast<u32>(h));
    gen.save_output(out_path, terrain);
    delete[] terrain;
    std::printf("saved: %s\n", out_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    if (!p1_tester_checkout(argc, argv)) {
        return -1;
    }
    P1_RunPrm prm;
    p1_resolve_run_prm(argc, argv, &prm);
    return test_p1_gen_river_pts_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
