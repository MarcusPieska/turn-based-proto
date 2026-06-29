//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_gen_climate.h"
#include "p1_gen_desert_river_cull.h"
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

static bool save_climate_viz (
    cstr path,
    const u8* terrain,
    const u8* river,
    const u8* climate,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || river == nullptr || climate == nullptr || w == 0 || h == 0) {
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
        if (cl != CLIMATE_NONE) {
            climate_to_rgb(cl, &r, &g, &b);
        }
        if (terrain[i] == TERR_MOUNTAINS[0]) {
            MapTerrainValidate::rgb_from_class(TERR_MOUNTAINS[0], &r, &g, &b);
        }
        if (river[i] != 0) {
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

static bool run_cull_case (
    const P1_RunPrm& prm,
    const u8* terrain,
    const u8* climate,
    const u8* riv_src,
    u16 w,
    u16 h,
    bool from_upstream,
    cstr out_path,
    double* out_sec,
    u32* out_culled) 
{
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* riv = new u8[npx];
    if (riv == nullptr) {
        return false;
    }
    std::memcpy(riv, riv_src, static_cast<size_t>(npx));
    P1_Gen_DesertRiverCull gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate(riv, w, h, terrain, climate, from_upstream);
    const clock_t t1 = clock();
    *out_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        delete[] riv;
        return false;
    }
    *out_culled = gen.culled_n();
    const bool saved = save_climate_viz(out_path, terrain, riv, climate, w, h);
    delete[] riv;
    return saved;
}

i32 test_p1_gen_desert_river_cull_basic (const P1_RunPrm& prm, const P1_Adj_LandAltitudePrm& lap) {
    char up_path[320];
    char dn_path[320];
    if (!p1_tester_make_out(prm.m_seed, up_path, sizeof(up_path))) {
        std::printf("failed to ensure upstream output dir\n");
        return -1;
    }
    if (!p1_tester_make_step_out(prm.m_seed, p1_tester_step(), "desert_river_cull_downstream", dn_path, sizeof(dn_path))) {
        std::printf("failed to ensure downstream output dir\n");
        return -1;
    }
    P1_TesterChain15Rslt chain = {};
    double sec_i = 0.0;
    if (!p1_build_ensure_input(prm, lap, static_cast<u16>(p1_tester_step() - 1u), &chain, &sec_i)) {
        std::printf("P1 steps 1-22 input failed for step 23\n");
        return -1;
    }
    P1_Gen_Climate clim_gen(prm);
    if (!clim_gen.generate(chain.m_terrain, chain.m_w, chain.m_h, chain.m_river)
        || !clim_gen.is_valid()) {
        std::printf("P1_Gen_Climate failed for step 23 input\n");
        p1_free_chain15(&chain);
        return -1;
    }
    const u8* climate = clim_gen.result().m_ov.data();
    double sec_up = 0.0;
    double sec_dn = 0.0;
    u32 culled_up = 0;
    u32 culled_dn = 0;
    std::printf("P1 steps 1-22 input time: %.6f s\n", sec_i);
    if (!run_cull_case(
            prm,
            chain.m_terrain,
            climate,
            chain.m_river,
            chain.m_w,
            chain.m_h,
            true,
            up_path,
            &sec_up,
            &culled_up)) {
        std::printf("P1_Gen_DesertRiverCull upstream failed\n");
        p1_free_chain15(&chain);
        return -1;
    }
    std::printf(
        "P1_Gen_DesertRiverCull upstream time: %.6f s (culled %u, %u x %u)\n",
        sec_up,
        culled_up,
        static_cast<u32>(chain.m_w),
        static_cast<u32>(chain.m_h));
    std::printf("saved: %s\n", up_path);
    if (!run_cull_case(
            prm,
            chain.m_terrain,
            climate,
            chain.m_river,
            chain.m_w,
            chain.m_h,
            false,
            dn_path,
            &sec_dn,
            &culled_dn)) {
        std::printf("P1_Gen_DesertRiverCull downstream failed\n");
        p1_free_chain15(&chain);
        return -1;
    }
    std::printf(
        "P1_Gen_DesertRiverCull downstream time: %.6f s (culled %u, %u x %u)\n",
        sec_dn,
        culled_dn,
        static_cast<u32>(chain.m_w),
        static_cast<u32>(chain.m_h));
    std::printf("saved: %s\n", dn_path);
    p1_free_chain15(&chain);
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
    return test_p1_gen_desert_river_cull_basic(prm, lap);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
