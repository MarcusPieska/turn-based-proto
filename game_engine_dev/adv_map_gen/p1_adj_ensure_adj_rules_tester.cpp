//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_terrain_validate.h"
#include "p1_adj_ensure_adj_rules.h"
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

static bool save_terrain_viz (
    cstr path,
    const u8* terrain,
    const u8* river,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || w == 0 || h == 0) {
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

static bool save_terrain_climate_viz (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* river,
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

i32 test_p1_adj_ensure_adj_rules_basic (P1_TesterHarness& h) {
    char out_path[320];
    char terr_path[320];
    if (!h.path_pri(out_path, sizeof(out_path)) || !h.path_sec(terr_path, sizeof(terr_path))) {
        std::printf("failed to ensure output path\n");
        return -1;
    }
    if (!h.run_input()) {
        std::printf("P1 steps 1-27 input failed for ensure adj rules\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for ensure adj rules\n");
        return -1;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(ht);
    u8* terrain = new u8[npx];
    u8* climate = new u8[npx];
    if (terrain == nullptr || climate == nullptr) {
        delete[] terrain;
        delete[] climate;
        return -1;
    }
    std::memcpy(terrain, chain.m_terrain, static_cast<size_t>(npx));
    std::memcpy(climate, chain.m_climate, static_cast<size_t>(npx));
    P1_Adj_EnsureAdjRules adj(h.prm());
    const clock_t t0 = clock();
    const bool ok = adj.adjust(terrain, climate, chain.m_rivers, w, ht);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_EnsureAdjRules failed to adjust\n");
        delete[] terrain;
        delete[] climate;
        return -1;
    }
    std::printf("P1 steps 1-27 input time: %.6f s\n", h.input_sec());
    std::printf(
        "P1_Adj_EnsureAdjRules adjust time: %.6f s (terr chg %u, clim chg %u, que peak %u, %u x %u)\n",
        sec,
        adj.terr_chg_n(),
        adj.clim_chg_n(),
        adj.que_peak_n(),
        static_cast<u32>(w),
        static_cast<u32>(ht));
    if (!save_terrain_climate_viz(out_path, terrain, climate, chain.m_rivers, w, ht)) {
        std::printf("failed to save map: %s\n", out_path);
        delete[] terrain;
        delete[] climate;
        return -1;
    }
    std::printf("saved: %s\n", out_path);
    if (!save_terrain_viz(terr_path, terrain, chain.m_rivers, w, ht)) {
        std::printf("failed to save terrain map: %s\n", terr_path);
        delete[] terrain;
        delete[] climate;
        return -1;
    }
    std::printf("saved: %s\n", terr_path);
    delete[] terrain;
    delete[] climate;
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
    const i32 rc = test_p1_adj_ensure_adj_rules_basic(h);
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
