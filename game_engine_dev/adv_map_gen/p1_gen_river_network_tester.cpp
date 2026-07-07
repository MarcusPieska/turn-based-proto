//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_shaped_outline.h"
#include "p1_adj_outline_fill.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_cont_outlines.h"
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

i32 test_p1_gen_river_network_basic (const P1_RunPrm& prm) {
    char out_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    const clock_t t0i = clock();
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(prm, &ov_map)) {
        std::printf("P1_Gen_ContOutlines failed for step 10 input\n");
        return -1;
    }
    const u16 w = ov_map.width();
    const u16 h = ov_map.height();
    const u8* ov = ov_map.data();
    if (ov == nullptr || w == 0 || h == 0) {
        std::printf("invalid outline overlay\n");
        return -1;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        std::printf("P1_Gen_LandDepth failed for step 10 input\n");
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
        std::printf("P1_Gen_ShapedOutline failed for step 10 input\n");
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverPts pts_gen(prm);
    if (!pts_gen.generate(terrain, w, h) || !pts_gen.is_valid()) {
        std::printf("P1_Gen_RiverPts failed for step 10 input\n");
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result()) || !sec_gen.is_valid()) {
        std::printf("P1_Gen_RiverSectors failed for step 10 input\n");
        delete[] terrain;
        return -1;
    }
    const clock_t t1i = clock();
    P1_Gen_RiverNetwork net_gen(prm);
    const clock_t t0 = clock();
    const bool ok = net_gen.generate(terrain, w, h, sec_gen.result());
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !net_gen.is_valid()) {
        std::printf("P1_Gen_RiverNetwork failed to generate\n");
        delete[] terrain;
        return -1;
    }
    const P1_Gen_RiverNetworkRslt& r = net_gen.result();
    std::printf("P1 steps 1-9 input time: %.6f s\n", sec_i);
    std::printf("P1_Gen_RiverNetwork generate time: %.6f s (%u watersheds, %u x %u)\n",
        sec,
        static_cast<u32>(r.m_basin_n),
        static_cast<u32>(w),
        static_cast<u32>(h));
    for (u16 bi = 0; bi < r.m_basin_n; ++bi) {
        const P1_RiverBasinEntry& b = r.m_basins[bi];
        std::printf("  basin %u: idx %u mouth (%u,%u) tiles %u\n",
            bi,
            b.m_idx,
            b.m_mouth_x,
            b.m_mouth_y,
            b.m_tile_n);
    }
    net_gen.save_output(out_path, terrain, sec_gen.result());
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
    return test_p1_gen_river_network_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
