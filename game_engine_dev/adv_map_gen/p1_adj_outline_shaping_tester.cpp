//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <ctime>

#include "p1_adj_outline_shaping.h"
#include "p1_adj_outline_fill.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_cont_outlines.h"
#include "game_primitives.h"
#include "map_terrain_data.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool run_outline_shaping (
    const P1_RunPrm& prm,
    f32 grad_lim,
    cstr out_fname,
    u8* base_ter,
    u16 w,
    u16 h,
    const u8* ov,
    const u16* land_depth) {
    char out_path[320];
    if (!p1_make_out_path(prm.m_seed, out_fname, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return false;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return false;
    }
    std::memcpy(terrain, base_ter, npx);
    P1_Adj_OutlineShapingPrm sp;
    sp.m_inner_grad_limit = grad_lim;
    P1_Adj_OutlineShaping adj(prm, sp);
    const clock_t t0 = clock();
    const bool ok = adj.adjust(terrain, w, h, ov, land_depth);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_OutlineShaping failed grad_lim=%.2f\n", static_cast<f64>(grad_lim));
        delete[] terrain;
        return false;
    }
    std::printf("P1_Adj_OutlineShaping grad_lim=%.2f time: %.6f s (%u x %u)\n",
        static_cast<f64>(grad_lim),
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h));
    MapTerrainData map;
    if (!map.assign_raw(w, h, terrain)) {
        std::printf("failed to assign terrain\n");
        delete[] terrain;
        return false;
    }
    delete[] terrain;
    if (!map.save_terrain_ppm(out_path)) {
        std::printf("failed to save map: %s\n", out_path);
        return false;
    }
    std::printf("saved: %s\n", out_path);
    return true;
}

i32 test_p1_adj_outline_shaping_basic (const P1_RunPrm& prm) {
    const clock_t t0o = clock();
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(prm, &ov_map)) {
        std::printf("P1_Gen_ContOutlines failed for step 5 input\n");
        return -1;
    }
    const u16 w = ov_map.width();
    const u16 h = ov_map.height();
    const u8* ov = ov_map.data();
    if (ov == nullptr || w == 0 || h == 0) {
        std::printf("invalid outline overlay\n");
        return -1;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* base_ter = new u8[npx];
    if (base_ter == nullptr) {
        return -1;
    }
    P1_Adj_OutlineFill fill(prm);
    if (!fill.adjust(base_ter, w, h, ov) || !fill.is_valid()) {
        std::printf("P1_Adj_OutlineFill failed for step 5 input\n");
        delete[] base_ter;
        return -1;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        std::printf("P1_Gen_LandDepth failed for step 5 input\n");
        delete[] base_ter;
        return -1;
    }
    const u16* land_depth = depth_gen.result().m_dist.data();
    if (land_depth == nullptr) {
        std::printf("invalid land depth data\n");
        delete[] base_ter;
        return -1;
    }
    const clock_t t1o = clock();
    const double sec_o = static_cast<double>(t1o - t0o) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("P1 steps 1-4 input time: %.6f s\n", sec_o);
    if (!run_outline_shaping(prm, 0.85f, "05_outline_shaping_lim085.ppm", base_ter, w, h, ov, land_depth)) {
        delete[] base_ter;
        return -1;
    }
    if (!run_outline_shaping(prm, 0.45f, "05_outline_shaping_lim045.ppm", base_ter, w, h, ov, land_depth)) {
        delete[] base_ter;
        return -1;
    }
    if (!run_outline_shaping(prm, 0.25f, "05_outline_shaping_lim025.ppm", base_ter, w, h, ov, land_depth)) {
        delete[] base_ter;
        return -1;
    }
    delete[] base_ter;
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    P1_RunPrm prm;
    p1_resolve_run_prm(argc, argv, &prm);
    return test_p1_adj_outline_shaping_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
