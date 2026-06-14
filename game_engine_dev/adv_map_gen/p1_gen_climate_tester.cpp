//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_climate.h"
#include "game_primitives.h"
#include "generator_constants.h"
#include "map_terrain_validate.h"
#include "p1_tester_chain15.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr || wi == 0 || hi == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void climate_rgb (u8 cls, u8* r, u8* g, u8* b) {
    if (cls == P1_CLIMATE_GRASSLAND) {
        *r = 90;
        *g = 170;
        *b = 50;
        return;
    }
    if (cls == P1_CLIMATE_PLAINS) {
        *r = 210;
        *g = 200;
        *b = 80;
        return;
    }
    if (cls == P1_CLIMATE_DESERT) {
        *r = 210;
        *g = 160;
        *b = 70;
        return;
    }
    *r = 0;
    *g = 0;
    *b = 0;
}

static bool save_climate_viz (
    cstr path,
    const u8* terrain,
    const u8* river,
    const u8* climate,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || climate == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(terrain[i], &r, &g, &b);
        const u8 cl = climate[i];
        if (cl != P1_CLIMATE_NONE) {
            climate_rgb(cl, &r, &g, &b);
        }
        if (terrain[i] == TERR_MOUNTAINS[0]) {
            MapTerrainValidate::rgb_from_class(TERR_MOUNTAINS[0], &r, &g, &b);
        }
        if (river != nullptr && river[i] != 0) {
            r = 0;
            g = 0;
            b = 255;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_p1_gen_climate_basic (const P1_RunPrm& prm, const P1_Adj_LandAltitudePrm& lap) {
    char out_path[320];
    char terr_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    if (!p1_tester_make_step_out(prm.m_seed, p1_tester_step(), "climate_terrain", terr_path, sizeof(terr_path))) {
        std::printf("failed to ensure terrain output path\n");
        return -1;
    }
    P1_TesterChain15Rslt chain = {};
    double sec_i = 0.0;
    if (!p1_build_ensure_input(prm, lap, static_cast<u16>(p1_tester_step()), &chain, &sec_i)) {
        std::printf("P1 steps 1-21 input failed for step 22\n");
        return -1;
    }
    P1_Gen_Climate gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate(chain.m_terrain, chain.m_w, chain.m_h, chain.m_river);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_Climate failed to generate\n");
        p1_free_chain15(&chain);
        return -1;
    }
    const u8* climate = gen.result().m_ov.data();
    std::printf("P1 steps 1-21 input time: %.6f s\n", sec_i);
    std::printf("P1_Gen_Climate generate time: %.6f s (%u x %u)\n",
        sec,
        static_cast<u32>(chain.m_w),
        static_cast<u32>(chain.m_h));
    if (!save_climate_viz(out_path, chain.m_terrain, chain.m_river, climate, chain.m_w, chain.m_h)) {
        std::printf("failed to save map: %s\n", out_path);
        p1_free_chain15(&chain);
        return -1;
    }
    std::printf("saved: %s\n", out_path);
    if (!save_climate_viz(terr_path, chain.m_terrain, nullptr, climate, chain.m_w, chain.m_h)) {
        std::printf("failed to save terrain map: %s\n", terr_path);
        p1_free_chain15(&chain);
        return -1;
    }
    p1_free_chain15(&chain);
    std::printf("saved: %s\n", terr_path);
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
    P1_Adj_LandAltitudePrm lap;
    p1_resolve_run_prm(argc, argv, &prm);
    p1_resolve_land_altitude_prm(argc, argv, &lap);
    return test_p1_gen_climate_basic(prm, lap);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
