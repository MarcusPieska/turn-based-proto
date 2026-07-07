//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_shaped_outline.h"
#include "p1_adj_outline_fill.h"
#include "p1_adj_river_inlets.h"
#include "p1_adj_river_lakes.h"
#include "p1_gen_distance_to_river.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
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

static bool build_step11_input (
    const P1_RunPrm& prm,
    u8* terrain,
    u16 w,
    u16 h,
    P1_Gen_RiverLines* lin_gen) 
{
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(prm, &ov_map)) {
        return false;
    }
    if (ov_map.width() != w || ov_map.height() != h) {
        return false;
    }
    const u8* ov = ov_map.data();
    if (ov == nullptr) {
        return false;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        return false;
    }
    const u16* land_depth = depth_gen.result().m_dist.data();
    if (land_depth == nullptr) {
        return false;
    }
    if (!build_step5_terrain(prm, terrain, w, h, ov, land_depth)) {
        return false;
    }
    P1_Gen_RiverPts pts_gen(prm);
    if (!pts_gen.generate(terrain, w, h) || !pts_gen.is_valid()) {
        return false;
    }
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result()) || !sec_gen.is_valid()) {
        return false;
    }
    P1_Gen_RiverNetwork net_gen(prm);
    if (!net_gen.generate(terrain, w, h, sec_gen.result()) || !net_gen.is_valid()) {
        return false;
    }
    if (!lin_gen->generate(terrain, w, h, sec_gen.result(), net_gen.result())
        || !lin_gen->is_valid()) {
        return false;
    }
    P1_Adj_RiverLakes lakes(prm);
    if (!lakes.adjust(terrain, w, h, lin_gen->result().m_ov) || !lakes.is_valid()) {
        return false;
    }
    P1_Adj_RiverInlets inlets(prm);
    return inlets.adjust(terrain, w, h, lin_gen->result().m_ov, lin_gen->result())
        && inlets.is_valid();
}

i32 test_p1_gen_distance_to_river_basic (const P1_RunPrm& prm) {
    char out_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    const u16 w = prm.m_w;
    const u16 h = prm.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return -1;
    }
    const clock_t t0i = clock();
    P1_Gen_RiverLines lin_gen(prm);
    if (!build_step11_input(prm, terrain, w, h, &lin_gen)) {
        std::printf("P1 steps 1-13 input failed for step 15\n");
        delete[] terrain;
        return -1;
    }
    const clock_t t1i = clock();
    P1_Gen_DistanceToRiver gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate(terrain, w, h, lin_gen.result().m_ov);
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_DistanceToRiver failed to generate\n");
        delete[] terrain;
        return -1;
    }
    const P1_Gen_DistanceToRiverRslt& r = gen.result();
    u32 land_nz = 0;
    const u8* pix = r.m_ov.data();
    if (pix != nullptr) {
        for (u32 i = 0; i < npx; ++i) {
            if (pix[i] != 0) {
                land_nz++;
            }
        }
    }
    std::printf("P1 steps 1-13 input time: %.6f s\n", sec_i);
    std::printf(
        "P1_Gen_DistanceToRiver generate time: %.6f s (max dist %u, %u land tiles, %u x %u)\n",
        sec,
        static_cast<u32>(r.m_max_dist),
        land_nz,
        static_cast<u32>(w),
        static_cast<u32>(h));
    gen.save_output(out_path);
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
    return test_p1_gen_distance_to_river_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
