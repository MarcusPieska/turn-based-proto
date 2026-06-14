//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

#include "game_primitives.h"
#include "generate_generic_depth_overlay.h"
#include "generator_constants.h"
#include "map_loader.h"
#include "perlin_noise.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_dir =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500";
static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_dir = "/home/w/Projects/simple-map-gen";

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static const u8 k_terr[6] = {
    TERR_OCEAN[0],
    TERR_SEA[0],
    TERR_COASTAL[0],
    TERR_PLAINS[0],
    TERR_HILLS[0],
    TERR_MOUNTAINS[0]
};

static cstr terr_name (u8 terr_idx) {
    if (terr_idx == TERR_OCEAN[0]) {
        return "ocean";
    }
    if (terr_idx == TERR_SEA[0]) {
        return "sea";
    }
    if (terr_idx == TERR_COASTAL[0]) {
        return "coastal";
    }
    if (terr_idx == TERR_PLAINS[0]) {
        return "plains";
    }
    if (terr_idx == TERR_HILLS[0]) {
        return "hills";
    }
    if (terr_idx == TERR_MOUNTAINS[0]) {
        return "mountains";
    }
    return "unknown";
}

static bool has_terr (const u8* terrain, u32 n, u8 terr_idx) {
    for (u32 i = 0; i < n; ++i) {
        if (terrain[i] == terr_idx) {
            return true;
        }
    }
    return false;
}

static i32 run_case (
    cstr tag,
    cstr out_path,
    const u8* terrain,
    u16 w,
    u16 h,
    u8 terr_idx) 
{
    const clock_t t0 = clock();
    u8* pix = Generate_GenericDepthOverlay::generate(terrain, w, h, terr_idx);
    const clock_t t1 = clock();
    if (pix == nullptr) {
        std::printf("%s failed to generate\n", tag);
        return -1;
    }
    const double gen_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("%s generate time: %.6f s\n", tag, gen_sec);
    const clock_t t2 = clock();
    const bool ok = save_perlin_gray_pgm(out_path, pix, w, h);
    const clock_t t3 = clock();
    delete[] pix;
    if (!ok) {
        std::printf("%s failed to save: %s\n", tag, out_path);
        return -1;
    }
    const double save_sec = static_cast<double>(t3 - t2) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("%s save time: %.6f s  saved=%s\n", tag, save_sec, out_path);
    return 0;
}

static i32 test_generate_generic_depth_overlay_basic () {
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
    i32 rc = 0;
    for (u32 ti = 0; ti < 6u; ++ti) {
        const u8 terr_idx = k_terr[ti];
        char path[512];
        char tag[64];
        std::snprintf(path, sizeof(path), "%s/generic-depth-%s.ppm", g_out_dir, terr_name(terr_idx));
        std::snprintf(tag, sizeof(tag), "depth-%s", terr_name(terr_idx));
        rc |= run_case(tag, path, terrain, w, h, terr_idx);
    }
    std::printf("ok  map=%ux%u\n", w, h);
    return rc;
}

static i32 test_generate_generic_depth_overlay_bench () {
    std::vector<double> times_ms;
    times_ms.reserve(600);
    u32 missing_n = 0;
    u32 fail_n = 0;
    for (u32 frame = 0; frame < 100u; ++frame) {
        char path[512];
        std::snprintf(
            path,
            sizeof(path),
            "%s/frame_%03u.ppm",
            g_map_dir,
            frame);
        MapTerrainData map;
        if (!MapLoader::load_terrain_ppm(path, map)) {
            std::printf("failed to load map: %s\n", path);
            return -1;
        }
        const u16 w = map.width();
        const u16 h = map.height();
        const u8* terrain = map.data();
        if (terrain == nullptr || w == 0 || h == 0) {
            std::printf("invalid map data: %s\n", path);
            return -1;
        }
        const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
        for (u32 ti = 0; ti < 6u; ++ti) {
            const u8 terr_idx = k_terr[ti];
            if (!has_terr(terrain, n, terr_idx)) {
                missing_n++;
                std::printf(
                    "WARNING  terr absent  frame=%03u  terr=%u (%s)\n",
                    frame,
                    terr_idx,
                    terr_name(terr_idx));
            }
            const clock_t t0 = clock();
            u8* pix = Generate_GenericDepthOverlay::generate(terrain, w, h, terr_idx);
            const clock_t t1 = clock();
            const double ms = static_cast<double>(t1 - t0)
                * 1000.0
                / static_cast<double>(CLOCKS_PER_SEC);
            times_ms.push_back(ms);
            if (pix == nullptr) {
                fail_n++;
                std::printf(
                    "WARNING  generate failed  frame=%03u  terr=%u\n",
                    frame,
                    terr_idx);
                continue;
            }
            delete[] pix;
        }
    }
    double sum = 0.0;
    for (size_t i = 0; i < times_ms.size(); ++i) {
        sum += times_ms[i];
    }
    const double avg_ms = (times_ms.empty()) ? 0.0 : (sum / static_cast<double>(times_ms.size()));
    std::printf(
        "bench  runs=%zu  avg=%.3f ms  missing_terr=%u  fail=%u\n",
        times_ms.size(),
        avg_ms,
        missing_n,
        fail_n);
    return (fail_n > 0) ? -1 : 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    if (argc >= 2 && std::strcmp(argv[1], "bench") == 0) {
        return test_generate_generic_depth_overlay_bench();
    }
    return test_generate_generic_depth_overlay_basic();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
