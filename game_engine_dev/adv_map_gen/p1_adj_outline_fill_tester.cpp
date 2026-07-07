//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_adj_outline_fill.h"
#include "p1_gen_cont_outlines.h"
#include "game_primitives.h"
#include "map_terrain_data.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_p1_adj_outline_fill_basic (const P1_RunPrm& prm) {
    char out_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    MapArrayOverlay ov_map;
    const clock_t t0o = clock();
    if (!p1_gen_step01_ov(prm, &ov_map)) {
        std::printf("P1_Gen_ContOutlines failed for step 2 input\n");
        return -1;
    }
    const clock_t t1o = clock();
    const u16 w = ov_map.width();
    const u16 h = ov_map.height();
    const u8* ov = ov_map.data();
    if (ov == nullptr || w == 0 || h == 0) {
        std::printf("invalid outline overlay\n");
        return -1;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return -1;
    }
    P1_Adj_OutlineFill adj(prm);
    const clock_t t0 = clock();
    const bool ok = adj.adjust(terrain, w, h, ov);
    const clock_t t1 = clock();
    const double sec_o = static_cast<double>(t1o - t0o) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_OutlineFill failed to adjust\n");
        delete[] terrain;
        return -1;
    }
    std::printf("P1_Gen_ContOutlines input time: %.6f s\n", sec_o);
    std::printf("P1_Adj_OutlineFill adjust time: %.6f s (%u x %u)\n",
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h));
    MapTerrainData map;
    if (!map.assign_raw(w, h, terrain)) {
        std::printf("failed to assign terrain\n");
        delete[] terrain;
        return -1;
    }
    delete[] terrain;
    if (!map.save_terrain_ppm(out_path)) {
        std::printf("failed to save map: %s\n", out_path);
        return -1;
    }
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
    return test_p1_adj_outline_fill_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
