//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <sys/stat.h>

#include "factory_game_array_simple.h"
#include "generator_constants.h"
#include "map_terrain_data.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* GAS_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const char* GAS_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const char* GAS_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const char* GAS_IN_RES = "/home/w/Projects/simple-map-gen/p1-seed-042/25_res_overlay.ppm";
static const char* GAS_OUT_ROOT = "/home/w/Projects/simple-map-gen/game-array-simple-test";

static const u8 k_terr_max = 7;
static const u8 k_clim_max = 4;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool ensure_dir (cstr path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

static bool wr_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void print_pct (cstr lbl, u32 cnt, u32 tot) {
    const f64 pct = (tot > 0) ? (100.0 * static_cast<f64>(cnt) / static_cast<f64>(tot)) : 0.0;
    std::printf("  %-12s %8u  %6.2f%%\n", lbl, cnt, pct);
}

static void print_stats (const GameArraySimple& arr) {
    const u16 w = arr.width();
    const u16 h = arr.height();
    const u32 tot = arr.tile_n();
    u32 terr_cnt[k_terr_max] = {};
    u32 clim_cnt[k_clim_max] = {};
    u32 riv_cnt = 0;
    u32 res_cnt = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 t = arr.get_terrain(x, y);
            if (t < k_terr_max) {
                terr_cnt[t]++;
            }
            const u8 c = arr.get_climate(x, y);
            if (c < k_clim_max) {
                clim_cnt[c]++;
            }
            if (arr.get_river(x, y) != 0) {
                riv_cnt++;
            }
            if (arr.get_res(x, y) != U16_KEY_NULL) {
                res_cnt++;
            }
        }
    }
    std::printf("map stats (%u x %u, %u tiles)\n", static_cast<u32>(w), static_cast<u32>(h), tot);
    std::printf("terrain\n");
    print_pct("none", terr_cnt[TERR_NONE[0]], tot);
    print_pct("ocean", terr_cnt[TERR_OCEAN[0]], tot);
    print_pct("sea", terr_cnt[TERR_SEA[0]], tot);
    print_pct("coastal", terr_cnt[TERR_COASTAL[0]], tot);
    print_pct("plains", terr_cnt[TERR_PLAINS[0]], tot);
    print_pct("hills", terr_cnt[TERR_HILLS[0]], tot);
    print_pct("mountains", terr_cnt[TERR_MOUNTAINS[0]], tot);
    std::printf("climate\n");
    print_pct("none", clim_cnt[CLIMATE_NONE], tot);
    print_pct("grassland", clim_cnt[CLIMATE_GRASSLAND], tot);
    print_pct("plains", clim_cnt[CLIMATE_PLAINS], tot);
    print_pct("desert", clim_cnt[CLIMATE_DESERT], tot);
    std::printf("rivers\n");
    print_pct("river", riv_cnt, tot);
    std::printf("resources\n");
    print_pct("placed", res_cnt, tot);
}

static bool save_terrain (const GameArraySimple& arr, cstr path) {
    const u16 w = arr.width();
    const u16 h = arr.height();
    const u32 n = arr.tile_n();
    u8* terr = new u8[n];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            terr[i] = arr.get_terrain(x, y);
        }
    }
    MapTerrainData map;
    const bool ok = map.assign_copy(w, h, terr) && map.save_terrain_ppm(path);
    delete[] terr;
    return ok;
}

static bool save_climate (const GameArraySimple& arr, cstr path) {
    const u16 w = arr.width();
    const u16 h = arr.height();
    const u32 n = arr.tile_n();
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(arr.get_climate(x, y), &r, &g, &b);
            rgb[i * 3u + 0] = r;
            rgb[i * 3u + 1] = g;
            rgb[i * 3u + 2] = b;
        }
    }
    const bool ok = wr_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_rivers (const GameArraySimple& arr, cstr path) {
    const u16 w = arr.width();
    const u16 h = arr.height();
    const u32 n = arr.tile_n();
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            const u8 v = arr.get_river(x, y) != 0 ? 255 : 0;
            rgb[i * 3u + 0] = v;
            rgb[i * 3u + 1] = v;
            rgb[i * 3u + 2] = v;
        }
    }
    const bool ok = wr_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!ensure_dir(GAS_OUT_ROOT)) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    GameArraySimple arr;
    if (!Factory_GameArraySimple::load_map_gen_data(&arr, GAS_IN_TERR, GAS_IN_CLIM, GAS_IN_RIV)) {
        std::printf("failed to load map gen data\n");
        return -1;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&arr, GAS_IN_RES)) {
        std::printf("failed to load res dist data\n");
        return -1;
    }
    std::printf("loaded %u x %u (%u tiles)\n",
        static_cast<u32>(arr.width()),
        static_cast<u32>(arr.height()),
        arr.tile_n());
    print_stats(arr);
    char terr_path[320];
    char clim_path[320];
    char riv_path[320];
    std::snprintf(terr_path, sizeof(terr_path), "%s/01_terrain.ppm", GAS_OUT_ROOT);
    std::snprintf(clim_path, sizeof(clim_path), "%s/02_climate.ppm", GAS_OUT_ROOT);
    std::snprintf(riv_path, sizeof(riv_path), "%s/03_rivers.ppm", GAS_OUT_ROOT);
    if (!save_terrain(arr, terr_path)) {
        std::printf("failed to save terrain: %s\n", terr_path);
        return -1;
    }
    std::printf("saved: %s\n", terr_path);
    if (!save_climate(arr, clim_path)) {
        std::printf("failed to save climate: %s\n", clim_path);
        return -1;
    }
    std::printf("saved: %s\n", clim_path);
    if (!save_rivers(arr, riv_path)) {
        std::printf("failed to save rivers: %s\n", riv_path);
        return -1;
    }
    std::printf("saved: %s\n", riv_path);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
