//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "resource_io.h"
#include "resource_parsing.h"
#include "resource_placement.h"
#include "p1_gen_climate.h"
#include "p1_tester_chain15.h"
#include "p1_tester_util.h"

static u32 RES_PLC_BASE_N = 100;
static u8 g_dot_small = 0;

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static ResPlcVizPrm viz_prm () {
    return g_dot_small != 0 ? res_plc_viz_prm_small() : res_plc_viz_prm_def();
}

static i32 run_placement_viz (const P1_RunPrm& prm, const P1_Adj_LandAltitudePrm& lap) {
    ResIoPath paths("../");
    ResIoFile file;
    ResParser parser;
    if (!file.load(paths.resources_path())) {
        std::printf("failed to load resources file\n");
        return -1;
    }
    if (!parser.parse_file(file)) {
        std::printf("failed to parse resources\n");
        return -1;
    }
    const ResCatalog& cat = parser.catalog();
    P1_TesterChain15Rslt chain = {};
    double sec_i = 0.0;
    if (!p1_build_ensure_input(prm, lap, 22u, &chain, &sec_i)) {
        std::printf("P1 steps 1-21 input failed\n");
        parser.release();
        return -1;
    }
    P1_Gen_Climate clim_gen(prm);
    if (!clim_gen.generate(chain.m_terrain, chain.m_w, chain.m_h, chain.m_river)
        || !clim_gen.is_valid()) {
        std::printf("P1_Gen_Climate failed\n");
        p1_free_chain15(&chain);
        parser.release();
        return -1;
    }
    const u8* climate = clim_gen.result().m_ov.data();
    u8* overlay = ResPlcOverlay::build_stub(chain.m_w, chain.m_h, chain.m_terrain, climate, chain.m_river);
    if (overlay == nullptr) {
        std::printf("overlay stub failed\n");
        p1_free_chain15(&chain);
        parser.release();
        return -1;
    }
    ResPlcMapCtx ctx = {};
    ctx.m_w = chain.m_w;
    ctx.m_h = chain.m_h;
    ctx.m_terrain = chain.m_terrain;
    ctx.m_climate = climate;
    ctx.m_river = chain.m_river;
    ctx.m_overlay = overlay;
    const u32 npx = (u32)chain.m_w * (u32)chain.m_h;
    u8* marks = new u8[npx];
    if (marks == nullptr) {
        delete[] overlay;
        p1_free_chain15(&chain);
        parser.release();
        return -1;
    }
    const ResPlcVizPrm vprm = viz_prm();
    u32 img_n = 0;
    double sel_sec_sum = 0.0;
    char fname[96];
    char out_path[384];
    for (u16 ri = 0; ri < cat.m_n; ++ri) {
        const ResEntry& entry = cat.m_items[ri];
        if (entry.m_has_plc == 0) {
            continue;
        }
        u32 placed = 0;
        double sel_sec = 0.0;
        if (!ResPlcSelect::run(ctx, entry, RES_PLC_BASE_N, prm.m_seed + (u32)ri,
                marks, npx, &placed, &sel_sec)) {
            std::printf("selection failed for %s\n", entry.m_name);
            delete[] marks;
            delete[] overlay;
            p1_free_chain15(&chain);
            parser.release();
            return -1;
        }
        sel_sec_sum += sel_sec;
        std::snprintf(fname, sizeof(fname), "Actual_%s.ppm", entry.m_name);
        if (!ResPlcViz::make_out_path(prm.m_seed, fname, out_path, sizeof(out_path))) {
            std::printf("failed to build out path\n");
            delete[] marks;
            delete[] overlay;
            p1_free_chain15(&chain);
            parser.release();
            return -1;
        }
        if (!ResPlcViz::save_pair_img(out_path, ctx, marks, npx, vprm)) {
            std::printf("failed to save: %s\n", out_path);
            delete[] marks;
            delete[] overlay;
            p1_free_chain15(&chain);
            parser.release();
            return -1;
        }
        std::printf("saved: %s (placed=%u, target=%u, select=%.6f s)\n",
            out_path,
            placed,
            RES_PLC_BASE_N * (u32)entry.m_plc.m_res_wt,
            sel_sec);
        ++img_n;
    }
    std::printf("resource_placement images: %u (base=%u, seed=%u, %ux%u pair)\n",
        img_n,
        RES_PLC_BASE_N,
        prm.m_seed,
        (unsigned)(chain.m_w * 2u),
        (unsigned)chain.m_h);
    std::printf("resource_placement select time total: %.6f s\n", sel_sec_sum);
    delete[] marks;
    delete[] overlay;
    p1_free_chain15(&chain);
    parser.release();
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
    std::printf("resource_placement_tester done: %d images in %.3f s (dot=%s)\n",
        img_n, sec, g_dot_small != 0 ? "1px" : "large");
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
