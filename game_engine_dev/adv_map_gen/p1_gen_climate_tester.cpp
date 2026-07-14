//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_climate.h"
#include "game_map_defs.h"
#include "game_primitives.h"
#include "generator_constants.h"
#include "map_terrain_validate.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"
#include "profile_time.h"

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
        if (cl != CLIMATE_NONE) {
            climate_to_rgb(cl, &r, &g, &b);
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

static bool run_rain_wt (
    P1_TesterHarness& h,
    const P1_MakeMapRslt& chain,
    u8 rain_wt) 
{
    PTIME_START(__func__);
    char out_path[320];
    char terr_path[320];
    char suffix[64];
    std::snprintf(suffix, sizeof(suffix), "climate_rain_wt_%02u", static_cast<unsigned>(rain_wt));
    if (!h.path_extra(suffix, out_path, sizeof(out_path)) || !h.path_sec(terr_path, sizeof(terr_path))) {
        std::printf("failed to ensure output path rain_wt=%u\n", static_cast<unsigned>(rain_wt));
        PTIME_STOP(__func__);
        return false;
    }
    P1_Gen_ClimatePrm sp = p1_gen_climate_prm_def();
    sp.m_wts.m_w_rain = rain_wt;
    P1_Gen_Climate gen(h.prm(), sp);
    const clock_t t0 = clock();
    const bool ok = gen.generate(chain.m_terrain, chain.m_w, chain.m_h, chain.m_rivers, chain.m_rain);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_Climate failed rain_wt=%u\n", static_cast<unsigned>(rain_wt));
        PTIME_STOP(__func__);
        return false;
    }
    const u8* climate = gen.result().m_ov.data();
    std::printf("P1_Gen_Climate rain_wt=%3u generate time: %.6f s (%u x %u)\n",
        static_cast<unsigned>(rain_wt),
        sec,
        static_cast<u32>(chain.m_w),
        static_cast<u32>(chain.m_h));
    if (rain_wt == p1_gen_climate_prm_def().m_wts.m_w_rain) {
        char pri_path[320];
        if (!h.path_pri(pri_path, sizeof(pri_path))) {
            std::printf("failed to ensure primary output path\n");
            PTIME_STOP(__func__);
            return false;
        }
        if (!save_climate_viz(pri_path, chain.m_terrain, chain.m_rivers, climate, chain.m_w, chain.m_h)) {
            std::printf("failed to save map: %s\n", pri_path);
            PTIME_STOP(__func__);
            return false;
        }
        std::printf("saved: %s\n", pri_path);
    } else if (!save_climate_viz(out_path, chain.m_terrain, chain.m_rivers, climate, chain.m_w, chain.m_h)) {
        std::printf("failed to save map: %s\n", out_path);
        PTIME_STOP(__func__);
        return false;
    } else {
        std::printf("saved: %s\n", out_path);
    }
    if (rain_wt == p1_gen_climate_prm_def().m_wts.m_w_rain && !save_climate_viz(terr_path, chain.m_terrain, nullptr, climate, chain.m_w, chain.m_h)) {
        std::printf("failed to save terrain map: %s\n", terr_path);
        PTIME_STOP(__func__);
        return false;
    }
    if (rain_wt == p1_gen_climate_prm_def().m_wts.m_w_rain) {
        std::printf("saved: %s\n", terr_path);
    }
    PTIME_STOP(__func__);
    return true;
}

i32 test_p1_gen_climate_basic (P1_TesterHarness& h, u8 rain_wt_arg, bool rain_wt_set) {
    if (!h.run_input()) {
        std::printf("P1 steps 1-23 input failed for step 24\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_rivers == nullptr || chain.m_rain == nullptr
        || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for climate\n");
        return -1;
    }
    std::printf("P1 steps 1-23 input time: %.6f s\n", h.input_sec());
    PTIME_START(__func__);
    if (rain_wt_set) {
        if (!run_rain_wt(h, chain, rain_wt_arg)) {
            PTIME_STOP(__func__);
            return -1;
        }
    } else {
        for (u32 wi = 0; wi <= 10u; ++wi) {
            u8 wt = static_cast<u8>(wi * 10u);
            if (wt > static_cast<u8>(CLIMATE_WT_MAX)) {
                wt = static_cast<u8>(CLIMATE_WT_MAX);
            }
            if (!run_rain_wt(h, chain, wt)) {
                PTIME_STOP(__func__);
                return -1;
            }
        }
    }
    PTIME_STOP(__func__);
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
    u8 rain_wt = 0;
    bool rain_wt_set = false;
    p1_resolve_climate_rain_wt(argc, argv, &rain_wt, &rain_wt_set);
    const i32 rc = test_p1_gen_climate_basic(h, rain_wt, rain_wt_set);
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
