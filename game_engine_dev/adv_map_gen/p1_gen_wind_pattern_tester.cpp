//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_primitives.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "p1_gen_wind_pattern.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
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

static void dir_to_rgb (u8 dir, u8 str, u8* r, u8* g, u8* b) {
    const f32 ang = (static_cast<f32>(dir) / 255.f) * 6.28318530f;
    const f32 hx = ang / 6.28318530f;
    i32 sector = static_cast<i32>(hx * 6.f);
    if (sector < 0) {
        sector = 0;
    }
    if (sector > 5) {
        sector = 5;
    }
    const f32 f = hx * 6.f - static_cast<f32>(sector);
    f32 p = 0.f;
    f32 q = 1.f - f;
    f32 t = f;
    f32 rf = 0.f;
    f32 gf = 0.f;
    f32 bf = 0.f;
    switch (sector) {
        case 0: rf = 1.f; gf = t; bf = p; break;
        case 1: rf = q; gf = 1.f; bf = p; break;
        case 2: rf = p; gf = 1.f; bf = t; break;
        case 3: rf = p; gf = q; bf = 1.f; break;
        case 4: rf = t; gf = p; bf = 1.f; break;
        default: rf = 1.f; gf = p; bf = q; break;
    }
    const f32 dim = 0.25f + 0.75f * (static_cast<f32>(str) / 255.f);
    if (r != nullptr) {
        *r = static_cast<u8>(rf * dim * 255.f);
    }
    if (g != nullptr) {
        *g = static_cast<u8>(gf * dim * 255.f);
    }
    if (b != nullptr) {
        *b = static_cast<u8>(bf * dim * 255.f);
    }
}

static bool save_dir_viz (cstr path, const u8* dir, const u8* str, u16 w, u16 h) {
    if (path == nullptr || dir == nullptr || str == nullptr || w == 0 || h == 0) {
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
        dir_to_rgb(dir[i], str[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_str_viz (cstr path, const u8* str, u16 w, u16 h) {
    if (path == nullptr || str == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 v = str[i];
        rgb[i * 3u + 0] = v;
        rgb[i * 3u + 1] = v;
        rgb[i * 3u + 2] = v;
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

i32 test_p1_gen_wind_pattern_basic () {
    char dir_path[320];
    char str_path[320];
    if (!make_out("25_wind_pattern_dir.ppm", dir_path, sizeof(dir_path))
        || !make_out("25_wind_pattern_str.ppm", str_path, sizeof(str_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {
        std::printf("failed to load map: %s\n", g_map_path);
        return -1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain = map.data();
    if (terrain == nullptr || w == 0 || h == 0) {
        std::printf("invalid map data\n");
        return -1;
    }
    P1_RunPrm prm;
    prm.m_seed = g_seed;
    prm.m_w = w;
    prm.m_h = h;
    P1_Gen_WindPattern gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate(terrain, w, h);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_WindPattern failed to generate\n");
        return -1;
    }
    const P1_Gen_WindPatternRslt& r = gen.result();
    const u8* dir = r.m_dir.data();
    const u8* str = r.m_str.data();
    if (dir == nullptr || str == nullptr) {
        std::printf("P1_Gen_WindPattern missing overlays\n");
        return -1;
    }
    std::printf("P1_Gen_WindPattern generate time: %.6f s (%u x %u)\n",
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h));
    if (!save_dir_viz(dir_path, dir, str, w, h)) {
        std::printf("failed to save dir viz: %s\n", dir_path);
        return -1;
    }
    if (!save_str_viz(str_path, str, w, h)) {
        std::printf("failed to save str viz: %s\n", str_path);
        return -1;
    }
    std::printf("saved: %s\n", dir_path);
    std::printf("saved: %s\n", str_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    (void)argc;
    (void)argv;
    const i32 rc = test_p1_gen_wind_pattern_basic();
    if (!p1_tester_whiteboard_chk()) {
        return -1;
    }
    return rc;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
