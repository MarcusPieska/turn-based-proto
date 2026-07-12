//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>
#include <ctime>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_terrain_validate.h"
#include "p1_gen_rain_orographic.h"
#include "p1_tester_harness.h"

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

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]
        || cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static bool is_mtn (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool save_gray_viz (cstr path, const u8* terrain, const u8* layer, u16 w, u16 h) {
    if (path == nullptr || terrain == nullptr || layer == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    const u8 br = 100;
    const u8 bg = 50;
    const u8 bb = 25;
    const u8 wr = 30;
    const u8 wg = 110;
    const u8 wb = 220;
    for (u32 i = 0; i < n; ++i) {
        if (is_water(terrain[i])) {
            rgb[i * 3u + 0] = wr;
            rgb[i * 3u + 1] = wg;
            rgb[i * 3u + 2] = wb;
        } else if (is_mtn(terrain[i])) {
            rgb[i * 3u + 0] = br;
            rgb[i * 3u + 1] = bg;
            rgb[i * 3u + 2] = bb;
        } else {
            const u8 v = layer[i];
            rgb[i * 3u + 0] = v;
            rgb[i * 3u + 1] = v;
            rgb[i * 3u + 2] = v;
        }
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_rain_viz (cstr path, const u8* terrain, const u8* rain, u16 w, u16 h) {
    if (path == nullptr || terrain == nullptr || rain == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        if (is_water(terrain[i])) {
            rgb[i * 3u + 0] = 0;
            rgb[i * 3u + 1] = 0;
            rgb[i * 3u + 2] = 0;
            continue;
        }
        u8 tr = 0;
        u8 tg = 0;
        u8 tb = 0;
        MapTerrainValidate::rgb_from_class(terrain[i], &tr, &tg, &tb);
        const f32 t = static_cast<f32>(rain[i]) / 255.f;
        if (t <= 0.f) {
            rgb[i * 3u + 0] = tr;
            rgb[i * 3u + 1] = tg;
            rgb[i * 3u + 2] = tb;
            continue;
        }
        const f32 a = 0.92f;
        const f32 u = std::pow(t, 0.55f);
        const f32 cr = 52.f + (218.f - 52.f) * u;
        const f32 cg = 108.f + (56.f - 108.f) * u;
        const f32 cb = 214.f + (52.f - 214.f) * u;
        rgb[i * 3u + 0] = static_cast<u8>(std::lrint(static_cast<f64>(tr) * (1.0 - a) + cr * a));
        rgb[i * 3u + 1] = static_cast<u8>(std::lrint(static_cast<f64>(tg) * (1.0 - a) + cg * a));
        rgb[i * 3u + 2] = static_cast<u8>(std::lrint(static_cast<f64>(tb) * (1.0 - a) + cb * a));
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_p1_gen_rain_orographic_basic (P1_TesterHarness& h) {
    char flood_coast_path[320];
    char flood_mtn_path[320];
    char height_path[320];
    char slope_path[320];
    char rain_path[320];
    if (!h.path_extra("rain_oro_flood_coast", flood_coast_path, sizeof(flood_coast_path))
        || !h.path_extra("rain_oro_flood_mtn", flood_mtn_path, sizeof(flood_mtn_path))
        || !h.path_extra("rain_oro_height", height_path, sizeof(height_path))
        || !h.path_extra("rain_oro_slope", slope_path, sizeof(slope_path))
        || !h.path_pri(rain_path, sizeof(rain_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    if (!h.run_input()) {
        std::printf("P1 steps 1-22 input failed for step 23\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    if (chain.m_terrain == nullptr || chain.m_wind_dir == nullptr || chain.m_wind_str == nullptr || w == 0 || ht == 0) {
        std::printf("invalid chain input for rain\n");
        return -1;
    }
    P1_Gen_RainOrographicPrm sp = p1_gen_rain_orographic_prm_def();
    sp.m_pack_dbg = 1;
    P1_Gen_RainOrographic rain(h.prm(), 3, 100, sp);
    const clock_t t0 = clock();
    const bool ok = rain.generate(chain.m_terrain, chain.m_wind_dir, chain.m_wind_str, w, ht);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !rain.is_valid()) {
        std::printf("P1_Gen_RainOrographic failed to generate\n");
        return -1;
    }
    const u8* flood_coast = rain.result().m_flood_coast.data();
    const u8* flood_mtn = rain.result().m_flood_mtn.data();
    const u8* height = rain.result().m_height.data();
    const u8* slope = rain.result().m_slope.data();
    const u8* rov = rain.result().m_rain.data();
    if (flood_coast == nullptr || flood_mtn == nullptr || height == nullptr || slope == nullptr
        || rov == nullptr) {
        std::printf("P1_Gen_RainOrographic missing overlays\n");
        return -1;
    }
    std::printf("P1 steps 1-22 input time: %.6f s\n", h.input_sec());
    std::printf("P1_Gen_RainOrographic generate time: %.6f s (%u x %u)\n",
        sec,
        static_cast<u32>(w),
        static_cast<u32>(ht));
    if (!save_gray_viz(flood_coast_path, chain.m_terrain, flood_coast, w, ht)) {
        std::printf("failed to save flood coast: %s\n", flood_coast_path);
        return -1;
    }
    if (!save_gray_viz(flood_mtn_path, chain.m_terrain, flood_mtn, w, ht)) {
        std::printf("failed to save flood mtn: %s\n", flood_mtn_path);
        return -1;
    }
    if (!save_gray_viz(height_path, chain.m_terrain, height, w, ht)) {
        std::printf("failed to save height: %s\n", height_path);
        return -1;
    }
    if (!save_gray_viz(slope_path, chain.m_terrain, slope, w, ht)) {
        std::printf("failed to save slope: %s\n", slope_path);
        return -1;
    }
    if (!save_rain_viz(rain_path, chain.m_terrain, rov, w, ht)) {
        std::printf("failed to save rain: %s\n", rain_path);
        return -1;
    }
    std::printf("saved: %s\n", flood_coast_path);
    std::printf("saved: %s\n", flood_mtn_path);
    std::printf("saved: %s\n", height_path);
    std::printf("saved: %s\n", slope_path);
    std::printf("saved: %s\n", rain_path);
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
    const i32 rc = test_p1_gen_rain_orographic_basic(h);
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
