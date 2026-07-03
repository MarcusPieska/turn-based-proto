//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "res_statics.h"
#include "resource_placement.h"
#include "r1_gen_res_overlay.h"
#include "p1_gen_climate.h"
#include "p1_tester_chain15.h"
#include "p1_tester_util.h"

static const char* g_lib_path = "../data_io/runtime_static_loader_lib.so";
static const char* g_data_path = "../";
static u32 g_base_n = 100;

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool build_ctx (
    const P1_RunPrm& prm,
    const P1_Adj_LandAltitudePrm& lap,
    ResPlcMapCtx* ctx,
    P1_TesterChain15Rslt* chain,
    u8** overlay,
    u8** climate) 
{
    if (ctx == nullptr || chain == nullptr || overlay == nullptr || climate == nullptr) {
        return false;
    }
    double sec_i = 0.0;
    if (!p1_build_ensure_input(prm, lap, 22u, chain, &sec_i)) {
        return false;
    }
    P1_Gen_Climate clim_gen(prm);
    if (!clim_gen.generate(chain->m_terrain, chain->m_w, chain->m_h, chain->m_river)
        || !clim_gen.is_valid()) {
        return false;
    }
    const u32 npx = (u32)chain->m_w * (u32)chain->m_h;
    u8* clim_copy = new u8[npx];
    if (clim_copy == nullptr) {
        return false;
    }
    std::memcpy(clim_copy, clim_gen.result().m_ov.data(), npx);
    *overlay = ResPlcOverlay::build_stub(chain->m_w, chain->m_h, chain->m_terrain,
        clim_copy, chain->m_river);
    if (*overlay == nullptr) {
        delete[] clim_copy;
        return false;
    }
    ctx->m_w = chain->m_w;
    ctx->m_h = chain->m_h;
    ctx->m_terrain = chain->m_terrain;
    ctx->m_climate = clim_copy;
    ctx->m_river = chain->m_river;
    ctx->m_overlay = *overlay;
    *climate = clim_copy;
    return true;
}

static i32 run_res_overlay (const P1_RunPrm& prm, const P1_Adj_LandAltitudePrm& lap) {
    ResStatics rs;
    if (!rs.load(g_lib_path, g_data_path)) {
        std::printf("failed to load statics\n");
        return -1;
    }
    P1_TesterChain15Rslt chain = {};
    ResPlcMapCtx ctx = {};
    u8* overlay = nullptr;
    u8* climate = nullptr;
    const clock_t t_map0 = clock();
    if (!build_ctx(prm, lap, &ctx, &chain, &overlay, &climate)) {
        std::printf("map context build failed\n");
        rs.unload();
        return -1;
    }
    const clock_t t_map1 = clock();
    R1_Gen_ResOverlay gen;
    const clock_t t_plc0 = clock();
    if (!gen.generate(ctx, rs.s(), g_base_n, prm.m_seed) || !gen.is_valid()) {
        std::printf("generate failed\n");
        delete[] overlay;
        delete[] climate;
        p1_free_chain15(&chain);
        rs.unload();
        return -1;
    }
    const clock_t t_plc1 = clock();
    char out_path[384];
    const clock_t t_save0 = clock();
    if (!R1_Gen_ResOverlay::make_out_path(prm.m_seed, "25_res_overlay.ppm",
            out_path, sizeof(out_path))
        || !R1_Gen_ResOverlay::save_ppm(out_path, ctx, gen.overlay(),
            gen.width(), gen.height(), rs.res_n() > 0 ? (u16)(rs.res_n() - 1u) : 0u)) {
        std::printf("save failed\n");
        delete[] overlay;
        delete[] climate;
        p1_free_chain15(&chain);
        rs.unload();
        return -1;
    }
    const clock_t t_save1 = clock();
    u32 placed = 0;
    u32 res_plc_n = 0;
    const u32 npx = (u32)gen.width() * (u32)gen.height();
    for (u32 i = 0; i < npx; ++i) {
        if (gen.overlay()[i] != U16_KEY_NULL) {
            ++placed;
        }
    }
    for (u16 ri = 0; ri < rs.res_n(); ++ri) {
        if (rs.res_has_plc(ri)) {
            ++res_plc_n;
        }
    }
    const double map_sec = (double)(t_map1 - t_map0) / (double)CLOCKS_PER_SEC;
    const double plc_sec = (double)(t_plc1 - t_plc0) / (double)CLOCKS_PER_SEC;
    const double save_sec = (double)(t_save1 - t_save0) / (double)CLOCKS_PER_SEC;
    std::printf("saved: %s (placed=%u resources=%u base=%u seed=%u)\n",
        out_path, placed, res_plc_n, g_base_n, prm.m_seed);
    std::printf("r1_gen_res_overlay map build: %.6f s\n", map_sec);
    std::printf("r1_gen_res_overlay place all resources: %.6f s\n", plc_sec);
    std::printf("r1_gen_res_overlay save image: %.6f s\n", save_sec);
    delete[] overlay;
    delete[] climate;
    p1_free_chain15(&chain);
    rs.unload();
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    P1_RunPrm prm = p1_run_prm_def();
    P1_Adj_LandAltitudePrm lap = p1_tester_land_altitude_prm();
    if (argc >= 2) {
        prm.m_seed = (u32)std::strtoul(argv[1], nullptr, 10);
    } else {
        prm.m_seed = p1_read_seed_file();
    }
    if (argc >= 3) {
        g_base_n = (u32)std::strtoul(argv[2], nullptr, 10);
    }
    const clock_t t0 = clock();
    const i32 rc = run_res_overlay(prm, lap);
    const clock_t t1 = clock();
    if (rc < 0) {
        return -1;
    }
    const double sec = (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
    std::printf("r1_gen_res_overlay_tester total: %.3f s\n", sec);
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
