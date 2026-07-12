//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_adj_land_altitude.h"
#include "game_primitives.h"
#include "generator_constants.h"
#include "map_terrain_validate.h"
#include "perlin_noise.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_p1_adj_land_altitude_basic (P1_TesterHarness& h, const P1_Adj_LandAltitudePrm& sp) {
    char out_path[320];
    char joint_path[320];
    if (!h.path_pri(out_path, sizeof(out_path)) || !h.path_sec(joint_path, sizeof(joint_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    if (!h.run_input()) {
        std::printf("P1 steps 1-16 input failed for step 17\n");
        return -1;
    }
    P1_TesterChain15Rslt& chain = h.c15_mut();
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(ht);
    u8* terrain_pre = new u8[npx];
    if (terrain_pre == nullptr) {
        return -1;
    }
    std::memcpy(terrain_pre, chain.m_terrain, static_cast<size_t>(npx));
    std::printf(
        "land altitude prm: w_noise %.3f w_near %.3f w_riv %.3f lim_hills %.3f lim_mtn %.3f\n",
        sp.m_w_noise,
        sp.m_w_near,
        sp.m_w_riv,
        sp.m_lim_hills,
        sp.m_lim_mtn);
    P1_Adj_LandAltitude adj(h.prm(), sp);
    const clock_t t0 = clock();
    const bool ok = adj.adjust(chain.m_terrain, w, ht, chain.m_noise, chain.m_dist_riv, chain.m_near_mtn);
    const clock_t t1 = clock();
    u8* joint = new u8[npx];
    if (joint == nullptr) {
        delete[] terrain_pre;
        return -1;
    }
    if (!adj.joint_ov(terrain_pre, w, ht, chain.m_noise, chain.m_dist_riv, chain.m_near_mtn, joint)) {
        std::printf("P1_Adj_LandAltitude joint_ov failed\n");
        delete[] joint;
        delete[] terrain_pre;
        return -1;
    }
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_LandAltitude failed to adjust\n");
        delete[] joint;
        delete[] terrain_pre;
        return -1;
    }
    u32 pln_n = 0;
    u32 hil_n = 0;
    u32 mtn_n = 0;
    for (u32 i = 0; i < npx; ++i) {
        const u8 t = chain.m_terrain[i];
        if (t == TERR_PLAINS[0]) {
            pln_n++;
        } else if (t == TERR_HILLS[0]) {
            hil_n++;
        } else if (t == TERR_MOUNTAINS[0]) {
            mtn_n++;
        }
    }
    std::printf("P1 steps 1-16 input time: %.6f s\n", h.input_sec());
    std::printf(
        "P1_Adj_LandAltitude adjust time: %.6f s (plains %u hills %u mountains %u, %u x %u)\n",
        sec,
        pln_n,
        hil_n,
        mtn_n,
        static_cast<u32>(w),
        static_cast<u32>(ht));
    if (!p1_save_terrain_rivers_ppm(out_path, chain.m_terrain, chain.m_river, w, ht)) {
        std::printf("failed to save map: %s\n", out_path);
        delete[] joint;
        delete[] terrain_pre;
        return -1;
    }
    std::printf("saved: %s\n", out_path);
    if (!save_perlin_gray_pgm(joint_path, joint, w, ht)) {
        std::printf("failed to save joint: %s\n", joint_path);
        delete[] joint;
        delete[] terrain_pre;
        return -1;
    }
    std::printf("saved: %s\n", joint_path);
    delete[] joint;
    delete[] terrain_pre;
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
    P1_Adj_LandAltitudePrm sp;
    p1_resolve_land_altitude_prm(argc, argv, &sp);
    const i32 rc = test_p1_adj_land_altitude_basic(h, sp);
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
