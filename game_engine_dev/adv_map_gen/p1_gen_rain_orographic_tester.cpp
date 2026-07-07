//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>
#include <ctime>

#include "game_primitives.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "map_terrain_validate.h"
#include "p1_gen_rain_orographic.h"
#include "p1_make_map.h"
#include "p1_tester_chain_core.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_terr_path = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const u32 g_seed = 42u;

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
        const f32 a = 0.92f; // Transparency of the rain overlay.
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

static bool make_out (cstr fname, char* out, size_t cap) {
    if (fname == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    char dir[256];
    std::snprintf(dir, sizeof(dir), "%s/p1-seed-%03u", P1_OUT_ROOT, static_cast<unsigned>(g_seed));
    if (!p1_ensure_dir(P1_OUT_ROOT) || !p1_ensure_dir(dir)) {
        return false;
    }
    std::snprintf(out, cap, "%s/%s", dir, fname);
    return true;
}

i32 test_p1_gen_rain_orographic_basic (const P1_RunPrm& prm) {
    char flood_coast_path[320];
    char flood_mtn_path[320];
    char height_path[320];
    char slope_path[320];
    char rain_path[320];
    if (!p1_tester_make_step_out(prm.m_seed, k_p1_step_rain, "rain_oro_flood_coast", flood_coast_path, sizeof(flood_coast_path))
        || !p1_tester_make_step_out(prm.m_seed, k_p1_step_rain, "rain_oro_flood_mtn", flood_mtn_path, sizeof(flood_mtn_path))
        || !p1_tester_make_step_out(prm.m_seed, k_p1_step_rain, "rain_oro_height", height_path, sizeof(height_path))
        || !p1_tester_make_step_out(prm.m_seed, k_p1_step_rain, "rain_oro_slope", slope_path, sizeof(slope_path))
        || !p1_tester_make_step_out(prm.m_seed, k_p1_step_rain, "rain_oro_rain", rain_path, sizeof(rain_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    P1_MakeMapRslt chain = {};
    double sec_i = 0.0;
    if (!p1_build_chain_core(prm, k_p1_step_wind, &chain, &sec_i)) {
        std::printf("P1 steps 1-22 input failed for step 23\n");
        return -1;
    }
    const u16 w = chain.m_w;
    const u16 h = chain.m_h;
    if (chain.m_terrain == nullptr || chain.m_wind_dir == nullptr || chain.m_wind_str == nullptr || w == 0 || h == 0) {
        std::printf("invalid chain input for rain\n");
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    P1_Gen_RainOrographicPrm sp = p1_gen_rain_orographic_prm_def();
    sp.m_pack_dbg = 1;
    P1_Gen_RainOrographic rain(prm, 3, 100, sp);
    const clock_t t0 = clock();
    const bool ok = rain.generate(chain.m_terrain, chain.m_wind_dir, chain.m_wind_str, w, h);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !rain.is_valid()) {
        std::printf("P1_Gen_RainOrographic failed to generate\n");
        P1_MakeMap::free_rslt(&chain);
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
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    std::printf("P1 steps 1-22 input time: %.6f s\n", sec_i);
    std::printf("P1_Gen_RainOrographic generate time: %.6f s (%u x %u)\n",
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h));
    if (!save_gray_viz(flood_coast_path, chain.m_terrain, flood_coast, w, h)) {
        std::printf("failed to save flood coast: %s\n", flood_coast_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    if (!save_gray_viz(flood_mtn_path, chain.m_terrain, flood_mtn, w, h)) {
        std::printf("failed to save flood mtn: %s\n", flood_mtn_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    if (!save_gray_viz(height_path, chain.m_terrain, height, w, h)) {
        std::printf("failed to save height: %s\n", height_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    if (!save_gray_viz(slope_path, chain.m_terrain, slope, w, h)) {
        std::printf("failed to save slope: %s\n", slope_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    if (!save_rain_viz(rain_path, chain.m_terrain, rov, w, h)) {
        std::printf("failed to save rain: %s\n", rain_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    std::printf("saved: %s\n", flood_coast_path);
    std::printf("saved: %s\n", flood_mtn_path);
    std::printf("saved: %s\n", height_path);
    std::printf("saved: %s\n", slope_path);
    std::printf("saved: %s\n", rain_path);
    P1_MakeMap::free_rslt(&chain);
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
    const i32 rc = test_p1_gen_rain_orographic_basic(prm);
    if (!p1_tester_whiteboard_chk()) {
        return -1;
    }
    return rc;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
