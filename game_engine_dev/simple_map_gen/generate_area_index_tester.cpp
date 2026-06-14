//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

#include "game_primitives.h"
#include "generate_area_index.h"
#include "generator_constants.h"
#include "generator_whiteboard.h"
#include "map_loader.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_dir = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500";
static const char* g_map_path = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_eq = "/home/w/Projects/simple-map-gen/area-index-eq-plains.ppm";
static const char* g_out_ge = "/home/w/Projects/simple-map-gen/area-index-ge-plains.ppm";
static const char* g_out_le = "/home/w/Projects/simple-map-gen/area-index-le-plains.ppm";

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static void print_areas (cstr tag, const AreaIdxResult* res) {
    if (res == nullptr) {
        return;
    }
    std::printf("%s  area_n=%u\n", tag, res->area_n);
    const u32 show = (res->area_n < 8u) ? res->area_n : 8u;
    for (u32 k = 0; k < show; ++k) {
        const AreaIdxEntry* e = &res->areas[k];
        std::printf(
            "  %u  size=%u  (%u,%u)\n",
            k,
            e->size,
            e->x,
            e->y);
    }
}

static void area_fill_rgb (u32 rid, u8* rgb) {
    const u32 lo = 100u;
    const u32 span = 255u - lo;
    rgb[0] = static_cast<u8>(lo + ((rid * 47u + 13u) % (span + 1u)));
    rgb[1] = static_cast<u8>(lo + ((rid * 17u + 7u) % (span + 1u)));
    rgb[2] = static_cast<u8>(lo + ((rid * 31u + 23u) % (span + 1u)));
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool tile_ok_eq (u8 cls, u8 terr_idx) {
    return cls == terr_idx;
}

static bool tile_ok_ge (u8 cls, u8 terr_idx) {
    return cls >= terr_idx;
}

static bool tile_ok_le (u8 cls, u8 terr_idx) {
    return cls <= terr_idx;
}

static void flood_paint_area (
    u16 w,
    u16 h,
    const u8* terrain,
    u8 terr_idx,
    bool (*tile_ok) (u8, u8),
    u8* rgb,
    u8* vis,
    u16 sx,
    u16 sy,
    const u8 fill[3]) 
{
    const u32 wi = static_cast<u32>(w);
    const u32 n = wi * static_cast<u32>(h);
    std::vector<u32> st;
    st.reserve(256);
    const u32 seed = static_cast<u32>(sy) * wi + static_cast<u32>(sx);
    if (seed >= n || vis[seed] != 0 || !tile_ok(terrain[seed], terr_idx)) {
        return;
    }
    st.push_back(seed);
    while (!st.empty()) {
        const u32 i = st.back();
        st.pop_back();
        if (vis[i] != 0) {
            continue;
        }
        if (!tile_ok(terrain[i], terr_idx)) {
            continue;
        }
        vis[i] = 1;
        u8* op = rgb + i * 3u;
        op[0] = fill[0];
        op[1] = fill[1];
        op[2] = fill[2];
        const u16 py = static_cast<u16>(i / wi);
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * wi);
        if (px > 0) {
            const u32 j = i - 1u;
            if (vis[j] == 0) {
                st.push_back(j);
            }
        }
        if (static_cast<u32>(px) + 1u < wi) {
            const u32 j = i + 1u;
            if (vis[j] == 0) {
                st.push_back(j);
            }
        }
        if (py > 0) {
            const u32 j = i - wi;
            if (vis[j] == 0) {
                st.push_back(j);
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + wi;
            if (vis[j] == 0) {
                st.push_back(j);
            }
        }
    }
}

static bool save_area_viz (
    cstr path,
    u16 w,
    u16 h,
    const u8* terrain,
    u8 terr_idx,
    bool (*tile_ok) (u8, u8),
    const AreaIdxResult* res) 
{
    if (path == nullptr || terrain == nullptr || res == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    u8* vis = new u8[n];
    if (rgb == nullptr || vis == nullptr) {
        delete[] rgb;
        delete[] vis;
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        rgb[i * 3u + 0] = 0;
        rgb[i * 3u + 1] = 0;
        rgb[i * 3u + 2] = 0;
        vis[i] = 0;
    }
    for (u32 k = 0; k < res->area_n; ++k) {
        const AreaIdxEntry* e = &res->areas[k];
        u8 fill[3];
        area_fill_rgb(k, fill);
        flood_paint_area(w, h, terrain, terr_idx, tile_ok, rgb, vis, e->x, e->y, fill);
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] vis;
    delete[] rgb;
    return ok;
}

static i32 run_case (
    cstr tag,
    cstr out_path,
    const u8* terrain,
    u16 w,
    u16 h,
    u8 terr_idx,
    bool (*tile_ok) (u8, u8),
    AreaIdxResult* (*gen) (const u8*, u16, u16, u8)) 
{
    const clock_t t0 = clock();
    AreaIdxResult* res = gen(terrain, w, h, terr_idx);
    const clock_t t1 = clock();
    if (res == nullptr) {
        std::printf("%s failed to generate\n", tag);
        return -1;
    }
    const double gen_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    print_areas(tag, res);
    std::printf("%s generate time: %.6f s\n", tag, gen_sec);
    const clock_t t2 = clock();
    const bool ok = save_area_viz(out_path, w, h, terrain, terr_idx, tile_ok, res);
    const clock_t t3 = clock();
    Generate_AreaIndex::free_result(res);
    if (!ok) {
        std::printf("%s failed to save: %s\n", tag, out_path);
        return -1;
    }
    const double save_sec = static_cast<double>(t3 - t2) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("%s save time: %.6f s  saved=%s\n", tag, save_sec, out_path);
    return 0;
}

static cstr mode_tag (u32 mode) {
    if (mode == 0) {
        return "eq";
    }
    if (mode == 1) {
        return "ge";
    }
    return "le";
}

static AreaIdxResult* run_mode (
    u32 mode,
    const u8* terrain,
    u16 w,
    u16 h,
    u8 terr_idx) 
{
    if (mode == 0) {
        return Generate_AreaIndex::generate_eq(terrain, w, h, terr_idx);
    }
    if (mode == 1) {
        return Generate_AreaIndex::generate_ge(terrain, w, h, terr_idx);
    }
    return Generate_AreaIndex::generate_le(terrain, w, h, terr_idx);
}

static i32 test_generate_area_index_bench () {
    static const u8 k_terr[6] = {
        TERR_OCEAN[0],
        TERR_SEA[0],
        TERR_COASTAL[0],
        TERR_PLAINS[0],
        TERR_HILLS[0],
        TERR_MOUNTAINS[0]
    };
    std::vector<double> times_ms;
    times_ms.reserve(1800);
    u32 zero_n = 0;
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
        for (u32 ti = 0; ti < 6u; ++ti) {
            const u8 terr_idx = k_terr[ti];
            for (u32 mode = 0; mode < 3u; ++mode) {
                const clock_t t0 = clock();
                AreaIdxResult* res = run_mode(mode, terrain, w, h, terr_idx);
                const clock_t t1 = clock();
                const double ms = static_cast<double>(t1 - t0)
                    * 1000.0
                    / static_cast<double>(CLOCKS_PER_SEC);
                times_ms.push_back(ms);
                if (res == nullptr) {
                    fail_n++;
                    std::printf(
                        "WARNING  generate failed  frame=%03u  mode=%s  terr=%u\n",
                        frame,
                        mode_tag(mode),
                        terr_idx);
                    continue;
                }
                if (res->area_n < 1u) {
                    zero_n++;
                    std::printf(
                        "WARNING  area_n=0  frame=%03u  mode=%s  terr=%u\n",
                        frame,
                        mode_tag(mode),
                        terr_idx);
                }
                Generate_AreaIndex::free_result(res);
            }
        }
    }
    GeneratorWhiteboard::dealloc();
    if (GeneratorWhiteboard::chkout() != 0) {
        std::printf(
            "GeneratorWhiteboard checkout leak: %u\n",
            GeneratorWhiteboard::chkout());
        return -1;
    }
    double sum = 0.0;
    for (size_t i = 0; i < times_ms.size(); ++i) {
        sum += times_ms[i];
    }
    const double avg_ms = (times_ms.empty()) ? 0.0 : (sum / static_cast<double>(times_ms.size()));
    std::printf(
        "bench  runs=%zu  avg=%.3f ms  zero_area=%u  fail=%u\n",
        times_ms.size(),
        avg_ms,
        zero_n,
        fail_n);
    return (fail_n > 0) ? -1 : 0;
}

static i32 test_generate_area_index_basic () {
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
    const u8 terr_idx = TERR_PLAINS[0];
    i32 rc = 0;
    rc |= run_case(
        "eq-plains",
        g_out_eq,
        terrain,
        w,
        h,
        terr_idx,
        tile_ok_eq,
        Generate_AreaIndex::generate_eq);
    rc |= run_case(
        "ge-plains",
        g_out_ge,
        terrain,
        w,
        h,
        terr_idx,
        tile_ok_ge,
        Generate_AreaIndex::generate_ge);
    rc |= run_case(
        "le-plains",
        g_out_le,
        terrain,
        w,
        h,
        terr_idx,
        tile_ok_le,
        Generate_AreaIndex::generate_le);
    GeneratorWhiteboard::dealloc();
    if (GeneratorWhiteboard::chkout() != 0) {
        std::printf("GeneratorWhiteboard checkout leak: %u\n", GeneratorWhiteboard::chkout());
        rc |= -1;
    }
    std::printf("ok  map=%ux%u  terr_idx=%u\n", w, h, terr_idx);
    return rc;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    if (argc >= 2 && std::strcmp(argv[1], "bench") == 0) {
        return test_generate_area_index_bench();
    }
    return test_generate_area_index_basic();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
