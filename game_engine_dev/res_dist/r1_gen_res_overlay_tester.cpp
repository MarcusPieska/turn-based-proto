//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "r1_gen_res_overlay.h"
#include "res_statics.h"
#include "p1_make_map.h"
#include "p1_tester_harness.h"

i32 test_r1_gen_res_overlay_basic (P1_TesterHarness& h) {
    char out_path[320];
    if (!h.path_pri(out_path, sizeof(out_path))) {
        std::printf("failed to ensure output path\n");
        return -1;
    }
    if (!h.run_input()) {
        std::printf("P1_MakeMap input failed for resources\n");
        return -1;
    }
    if (!ResStatics::is_ready()) {
        std::printf("failed to load statics\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_overlay == nullptr
        || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for resources\n");
        return -1;
    }
    ResPlcMapCtx ctx = {};
    ctx.m_w = chain.m_w;
    ctx.m_h = chain.m_h;
    ctx.m_terrain = chain.m_terrain;
    ctx.m_climate = chain.m_climate;
    ctx.m_river = chain.m_rivers;
    ctx.m_overlay = chain.m_overlay;
    R1_Gen_ResOverlay gen;
    const clock_t t0 = clock();
    P1_MakeMapPrm mp = p1_make_map_prm_def();
    const bool ok = gen.generate(ctx, ResStatics::shared(), mp.m_res_base_n, h.prm().m_seed) && gen.is_valid();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok) {
        std::printf("R1_Gen_ResOverlay failed to generate\n");
        return -1;
    }
    u32 placed = 0;
    const u32 npx = static_cast<u32>(gen.width()) * static_cast<u32>(gen.height());
    for (u32 i = 0; i < npx; ++i) {
        if (gen.overlay()[i] != U16_KEY_NULL) {
            ++placed;
        }
    }
    std::printf("P1_MakeMap input time: %.6f s\n", h.input_sec());
    std::printf("R1_Gen_ResOverlay gen time: %.6f s (placed=%u, %u x %u)\n",
        sec, placed, static_cast<u32>(chain.m_w), static_cast<u32>(chain.m_h));
    if (!R1_Gen_ResOverlay::save_ppm(out_path, ctx, gen.overlay(), gen.width(), gen.height(), 255u)) {
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
    P1_TesterHarness h;
    if (!h.begin(argc, argv)) {
        return -1;
    }
    const i32 rc = test_r1_gen_res_overlay_basic(h);
    if (rc != 0) {
        return rc;
    }
    if (!h.finish()) {
        return -1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
