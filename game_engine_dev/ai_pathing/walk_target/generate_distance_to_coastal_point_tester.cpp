//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "generate_distance_to_coastal_point.h"
#include "generate_distance_to_ocean_coast.h"
#include "game_map_defs.h"
#include "map_loader.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr k_in_path = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr k_out_path = "/home/w/Projects/simple-map-gen/distance-to-coastal-point.ppm";
static const u16 k_sx = 499u;
static const u16 k_sy = 499u;
static const u16 k_dep_none = 0xFFFFu;
static const u8 k_wtr_b = 30;
static const u8 k_wtr_g = 110;
static const u8 k_wtr_r = 220;
static const u8 k_inland_r = 34;
static const u8 k_inland_g = 112;
static const u8 k_inland_b = 48;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static u8 norm_gray (u16 d, u16 max_d) {
    if (d == k_dep_none) {
        return 0;
    }
    if (max_d == 0) {
        return 255;
    }
    return static_cast<u8>((static_cast<u32>(d) * 255u) / static_cast<u32>(max_d));
}

static bool step_down (
    const u16* coast_dist,
    u16 w,
    u16 h,
    u16& x,
    u16& y) {
    const u32 ci = tidx(w, x, y);
    const u16 cur = coast_dist[ci];
    if (cur == k_dep_none || cur == 0u) {
        return false;
    }
    u16 best = cur;
    u16 bx = x;
    u16 by = y;
    bool found = false;
    static const i16 dx4[] = {0, 1, 0, -1};
    static const i16 dy4[] = {-1, 0, 1, 0};
    for (u32 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + dx4[k];
        const i32 ny = static_cast<i32>(y) + dy4[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (tx >= w || ty >= h) {
            continue;
        }
        const u16 d = coast_dist[tidx(w, tx, ty)];
        if (d == k_dep_none || d >= cur) {
            continue;
        }
        if (!found || d < best) {
            best = d;
            bx = tx;
            by = ty;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    x = bx;
    y = by;
    return true;
}

static bool find_coast_pt (
    const u16* coast_dist,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16& ox,
    u16& oy) {
    ox = sx;
    oy = sy;
    if (coast_dist[tidx(w, ox, oy)] == 0u) {
        return true;
    }
    for (u32 guard = 0; guard < 100000u; ++guard) {
        if (!step_down(coast_dist, w, h, ox, oy)) {
            break;
        }
        if (coast_dist[tidx(w, ox, oy)] == 0u) {
            return true;
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (coast_dist[tidx(w, x, y)] == 0u) {
                ox = x;
                oy = y;
                return true;
            }
        }
    }
    return false;
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
    const u16* coast_dist,
    const u16* ring,
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
        } else if (coast_dist[i] == k_dep_none
            || coast_dist[i] > Generate_DistanceToCoastalPoint::k_coast_band) {
            r = k_inland_r;
            g = k_inland_g;
            b = k_inland_b;
        } else if (ring[i] != k_dep_none) {
            const u8 gry = norm_gray(ring[i], max_d);
            r = gry;
            g = gry;
            b = gry;
        } else {
            r = k_inland_r;
            g = k_inland_g;
            b = k_inland_b;
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
    u16 coast_max = 0;
    const clock_t t0 = clock();
    u16* coast_dist = Generate_DistanceToOceanCoast::generate(terrain, w, h, &coast_max);
    const double coast_sec = static_cast<double>(clock() - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (coast_dist == nullptr) {
        std::printf("*** FAILED coast generate\n");
        return 1;
    }
    u16 px = k_sx;
    u16 py = k_sy;
    if (!find_coast_pt(coast_dist, w, h, px, py, px, py)) {
        delete[] coast_dist;
        std::printf("*** FAILED find coastal point\n");
        return 1;
    }
    u16 ring_max = 0;
    const clock_t t1 = clock();
    u16* ring = Generate_DistanceToCoastalPoint::generate(
        terrain, w, h, coast_dist, px, py, &ring_max);
    const double ring_sec = static_cast<double>(clock() - t1) / static_cast<double>(CLOCKS_PER_SEC);
    if (ring == nullptr) {
        delete[] coast_dist;
        std::printf("*** FAILED ring generate\n");
        return 1;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        delete[] coast_dist;
        delete[] ring;
        std::printf("*** FAILED alloc rgb\n");
        return 1;
    }
    build_img(terrain, coast_dist, ring, w, h, ring_max, rgb);
    delete[] coast_dist;
    delete[] ring;
    if (!save_rgb_ppm(k_out_path, rgb, w, h)) {
        delete[] rgb;
        std::printf("*** FAILED save %s\n", k_out_path);
        return 1;
    }
    delete[] rgb;
    std::printf("Generate_DistanceToCoastalPoint: %ux%u from (%u,%u) max_ring %u\n",
        static_cast<unsigned>(w),
        static_cast<unsigned>(h),
        static_cast<unsigned>(px),
        static_cast<unsigned>(py),
        static_cast<unsigned>(ring_max));
    std::printf("coast overlay time: %.6f s ring overlay time: %.6f s\n", coast_sec, ring_sec);
    std::printf("saved: %s\n", k_out_path);
    std::printf("*** PASSED generate_distance_to_coastal_point\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
