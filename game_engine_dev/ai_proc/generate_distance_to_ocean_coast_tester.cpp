//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "generate_distance_to_ocean_coast.h"
#include "game_map_defs.h"
#include "map_loader.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr k_in_path = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr k_out_path = "/home/w/Projects/simple-map-gen/distance-to-ocean-coast.ppm";
static const u8 k_wtr_b = 30;
static const u8 k_wtr_g = 110;
static const u8 k_wtr_r = 220;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static u8 norm_gray (u16 d, u16 max_d) {
    if (d == 0xFFFFu) {
        return 0;
    }
    if (max_d == 0) {
        return 255;
    }
    return static_cast<u8>((static_cast<u32>(d) * 255u) / static_cast<u32>(max_d));
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
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

static bool build_img (
    const u8* terrain,
    const u16* dist,
    u16 w,
    u16 h,
    u16 max_d,
    u8* rgb) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u8 t = terrain[i];
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (is_water(t)) {
            r = k_wtr_r;
            g = k_wtr_g;
            b = k_wtr_b;
        } else {
            const u8 gry = norm_gray(dist[i], max_d);
            r = gry;
            g = gry;
            b = gry;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(k_in_path, map)) {
        std::printf("*** FAILED load %s\n", k_in_path);
        return 1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain = map.data();
    u16 max_d = 0;
    const clock_t t0 = clock();
    u16* dist = Generate_DistanceToOceanCoast::generate(terrain, w, h, &max_d);
    const double gen_sec = static_cast<double>(clock() - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (dist == nullptr) {
        std::printf("*** FAILED generate\n");
        return 1;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        delete[] dist;
        std::printf("*** FAILED alloc rgb\n");
        return 1;
    }
    build_img(terrain, dist, w, h, max_d, rgb);
    delete[] dist;
    if (!save_rgb_ppm(k_out_path, rgb, w, h)) {
        delete[] rgb;
        std::printf("*** FAILED save %s\n", k_out_path);
        return 1;
    }
    delete[] rgb;
    std::printf("Generate_DistanceToOceanCoast: %ux%u max_depth %u\n",
        static_cast<unsigned>(w),
        static_cast<unsigned>(h),
        static_cast<unsigned>(max_d));
    std::printf("overlay time: %.6f s\n", gen_sec);
    std::printf("saved: %s\n", k_out_path);
    std::printf("*** PASSED generate_distance_to_ocean_coast\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
