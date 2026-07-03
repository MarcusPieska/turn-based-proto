//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "res_statics.h"
#include "resource_placement.h"
#include "p1_gen_climate.h"
#include "p1_tester_chain15.h"
#include "p1_tester_util.h"

static const char* g_lib_path = "../data_io/runtime_static_loader_lib.so";
static const char* g_data_path = "../";
static u32 RES_PLC_BASE_N = 100;
static u8 g_dot_small = 0;

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static ResPlcVizPrm viz_prm () {
    return g_dot_small != 0 ? res_plc_viz_prm_small() : res_plc_viz_prm_def();
}

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

static i32 run_placement_viz (const P1_RunPrm& prm, const P1_Adj_LandAltitudePrm& lap) {
    ResStatics rs;
    if (!rs.load(g_lib_path, g_data_path)) {
        std::printf("failed to load statics\n");
        return -1;
    }
    P1_TesterChain15Rslt chain = {};
    ResPlcMapCtx ctx = {};
    u8* overlay = nullptr;
    u8* climate = nullptr;
    if (!build_ctx(prm, lap, &ctx, &chain, &overlay, &climate)) {
        std::printf("map context build failed\n");
        rs.unload();
        return -1;
    }
    const u32 npx = (u32)chain.m_w * (u32)chain.m_h;
    u8* poss = new u8[npx];
    u8* act = new u8[npx];
    if (poss == nullptr || act == nullptr) {
        delete[] poss;
        delete[] act;
        delete[] overlay;
        delete[] climate;
        p1_free_chain15(&chain);
        rs.unload();
        return -1;
    }
    const ResPlcVizPrm vprm = viz_prm();
    u32 img_n = 0;
    double sel_sec_sum = 0.0;
    char fname[96];
    char out_path[384];
    for (u16 ri = 0; ri < rs.res_n(); ++ri) {
        if (!rs.res_has_plc(ri)) {
            continue;
        }
        cstr rnm = rs.res_name(ri);
        const u32 hit = ResPlcMatch::mark_all_rules(ctx, rs.s(), ri, poss, npx);
        u32 placed = 0;
        double sel_sec = 0.0;
        if (!ResPlcSelect::run(ctx, rs.s(), ri, RES_PLC_BASE_N, prm.m_seed + (u32)ri,
                act, npx, &placed, &sel_sec)) {
            std::printf("selection failed for %s\n", rnm);
            delete[] poss;
            delete[] act;
            delete[] overlay;
            delete[] climate;
            p1_free_chain15(&chain);
            rs.unload();
            return -1;
        }
        sel_sec_sum += sel_sec;
        std::snprintf(fname, sizeof(fname), "%s.ppm", rnm);
        if (!ResPlcViz::make_out_path(prm.m_seed, fname, out_path, sizeof(out_path))
            || !ResPlcViz::save_pair_ov_img(out_path, ctx, poss, act, npx, vprm)) {
            std::printf("overlay viz failed for %s\n", rnm);
            delete[] poss;
            delete[] act;
            delete[] overlay;
            delete[] climate;
            p1_free_chain15(&chain);
            rs.unload();
            return -1;
        }
        std::printf("saved: %s (possible=%u placed=%u target=%u select=%.6f s)\n",
            out_path, hit, placed, RES_PLC_BASE_N * (u32)rs.res_plc(ri).m_res_wt, sel_sec);
        ++img_n;
    }
    std::printf("resource_placement images: %u (base=%u, seed=%u, %ux%u quad)\n",
        img_n, RES_PLC_BASE_N, prm.m_seed,
        (unsigned)(chain.m_w * 2u), (unsigned)(chain.m_h * 2u));
    std::printf("resource_placement select time total: %.6f s\n", sel_sec_sum);
    delete[] poss;
    delete[] act;
    delete[] overlay;
    delete[] climate;
    p1_free_chain15(&chain);
    rs.unload();
    return (i32)img_n;
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
        g_dot_small = (u8)std::strtoul(argv[2], nullptr, 10);
    }
    if (argc >= 4) {
        RES_PLC_BASE_N = (u32)std::strtoul(argv[3], nullptr, 10);
    }
    const clock_t t0 = clock();
    const i32 img_n = run_placement_viz(prm, lap);
    const clock_t t1 = clock();
    if (img_n < 0) {
        return -1;
    }
    const double sec = (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
    std::printf("resource_placement_tester done: %d resources in %.3f s (dot=%s)\n",
        img_n, sec, g_dot_small != 0 ? "1px" : "large");
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
