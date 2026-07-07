//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <ctime>

#include "p1_adj_outline_fill.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_shaped_outline.h"
#include "game_primitives.h"
#include "map_terrain_data.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool save_terrain (cstr path, u8* terrain, u16 w, u16 h) {
    MapTerrainData map;
    if (!map.assign_raw(w, h, terrain)) {
        return false;
    }
    return map.save_terrain_ppm(path);
}

static bool run_shaped_layer (
    const P1_RunPrm& prm,
    f32 radial,
    u32 step_n,
    cstr suffix,
    u8* base_ter,
    u16 w,
    u16 h,
    const u8* ov,
    const u16* land_depth) 
{
    char out_path[320];
    if (!p1_tester_make_step_out(prm.m_seed, step_n, suffix, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return false;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return false;
    }
    std::memcpy(terrain, base_ter, static_cast<size_t>(npx));
    P1_Gen_ShapedOutline gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate_layer(terrain, w, h, ov, land_depth, radial);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_ShapedOutline failed radial=%.2f\n", static_cast<f64>(radial));
        delete[] terrain;
        return false;
    }
    std::printf("P1_Gen_ShapedOutline radial=%.2f time: %.6f s (%u x %u)\n",
        static_cast<f64>(radial),
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h));
    if (!save_terrain(out_path, terrain, w, h)) {
        std::printf("failed to save map: %s\n", out_path);
        delete[] terrain;
        return false;
    }
    delete[] terrain;
    std::printf("saved: %s\n", out_path);
    return true;
}

i32 test_p1_gen_shaped_outline_basic (const P1_RunPrm& prm) {
    P1_Gen_ShapedOutlinePrm sp = p1_gen_shaped_outline_prm_def();
    const clock_t t0o = clock();
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(prm, &ov_map)) {
        std::printf("P1_Gen_ContOutlines failed for shaped outline input\n");
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
        std::printf("P1_Adj_OutlineFill failed for shaped outline input\n");
        delete[] base_ter;
        return -1;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        std::printf("P1_Gen_LandDepth failed for shaped outline input\n");
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
    if (!run_shaped_layer(prm, sp.m_radial_near, p1_tester_step(), "shaped_outline_near", base_ter, w, h, ov, land_depth)) {
        delete[] base_ter;
        return -1;
    }
    if (!run_shaped_layer(prm, sp.m_radial_far, p1_tester_step() + 1u, "shaped_outline_far", base_ter, w, h, ov, land_depth)) {
        delete[] base_ter;
        return -1;
    }
    char merge_path[320];
    if (!p1_tester_make_out(prm.m_seed, merge_path, sizeof(merge_path))) {
        delete[] base_ter;
        return -1;
    }
    u8* near_ter = new u8[npx];
    u8* far_ter = new u8[npx];
    u8* merged = new u8[npx];
    if (near_ter == nullptr || far_ter == nullptr || merged == nullptr) {
        delete[] merged;
        delete[] far_ter;
        delete[] near_ter;
        delete[] base_ter;
        return -1;
    }
    std::memcpy(near_ter, base_ter, static_cast<size_t>(npx));
    std::memcpy(far_ter, base_ter, static_cast<size_t>(npx));
    std::memcpy(merged, base_ter, static_cast<size_t>(npx));
    P1_Gen_ShapedOutline gen(prm);
    const clock_t t0m = clock();
    const bool ok_near = gen.generate_layer(near_ter, w, h, ov, land_depth, sp.m_radial_near);
    const bool ok_far = gen.generate_layer(far_ter, w, h, ov, land_depth, sp.m_radial_far);
    const bool ok_merge = ok_near && ok_far
        && gen.merge_layers(merged, w, h, ov, land_depth, near_ter, far_ter);
    const clock_t t1m = clock();
    const double sec_m = static_cast<double>(t1m - t0m) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok_merge || !gen.is_valid()) {
        std::printf("P1_Gen_ShapedOutline merge failed\n");
        delete[] merged;
        delete[] far_ter;
        delete[] near_ter;
        delete[] base_ter;
        return -1;
    }
    std::printf("P1_Gen_ShapedOutline merge time: %.6f s (%u x %u)\n",
        sec_m,
        static_cast<u32>(w),
        static_cast<u32>(h));
    if (!save_terrain(merge_path, merged, w, h)) {
        std::printf("failed to save map: %s\n", merge_path);
        delete[] merged;
        delete[] far_ter;
        delete[] near_ter;
        delete[] base_ter;
        return -1;
    }
    std::printf("saved: %s\n", merge_path);
    delete[] merged;
    delete[] far_ter;
    delete[] near_ter;
    delete[] base_ter;
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
    return test_p1_gen_shaped_outline_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
