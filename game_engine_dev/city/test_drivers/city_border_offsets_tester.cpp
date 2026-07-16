//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "circular_tile_areas.h"
#include "game_array_simple.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_img = 61;
static const u16 k_img_c = 30;
static const u16 k_map = 1000;
static const u16 k_map_c = 500;
static const u8 k_owner = 1;
static const u8 k_r_lo = 2;
static const u8 k_r_hi = 20;
static const char* k_out_dir = "/home/w/Projects/simple-map-gen/city-border-offsets";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool wr_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
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

static void stamp (GameTileSimple* tiles, u16 cx, u16 cy, u16 map_w, u8 r, u8 owner) {
    const CircArea area = CircularTileAreas::get(r);
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0 || x >= static_cast<i32>(map_w) || y >= static_cast<i32>(map_w)) {
            continue;
        }
        tiles[static_cast<u32>(y) * map_w + static_cast<u32>(x)].m_civ_owner = owner;
    }
}

static void wr_range_img (u8 r) {
    u8 rgb[k_img * k_img * 3];
    std::memset(rgb, 255, sizeof(rgb));
    const CircArea area = CircularTileAreas::get(r);
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(k_img_c) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(k_img_c) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0 || x >= static_cast<i32>(k_img) || y >= static_cast<i32>(k_img)) {
            continue;
        }
        u8* p = &rgb[(static_cast<u32>(y) * k_img + static_cast<u32>(x)) * 3u];
        p[0] = 255;
        p[1] = 0;
        p[2] = 0;
    }
    u8* c = &rgb[(static_cast<u32>(k_img_c) * k_img + static_cast<u32>(k_img_c)) * 3u];
    c[0] = 0;
    c[1] = 0;
    c[2] = 0;
    char path[512];
    std::snprintf(path, sizeof(path), "%s/city_brd_r%02u.ppm", k_out_dir, (unsigned)r);
    if (!wr_ppm(path, rgb, k_img, k_img)) {
        std::printf("fail write %s\n", path);
    } else {
        std::printf("wrote %s\n", path);
    }
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    const CircArea bad = CircularTileAreas::get(21);
    std::printf("invalid r=21 lim=%u\n", (unsigned)bad.m_lim);

    for (u8 r = k_r_lo; r <= k_r_hi; ++r) {
        wr_range_img(r);
    }

    GameTileSimple* tiles = new GameTileSimple[static_cast<u32>(k_map) * static_cast<u32>(k_map)];
    std::memset(tiles, 0, sizeof(GameTileSimple) * static_cast<u32>(k_map) * static_cast<u32>(k_map));
    stamp(tiles, k_map_c, k_map_c, k_map, k_r_hi, k_owner);

    for (u8 r = k_r_lo; r <= k_r_hi; ++r) {
        const auto t0 = std::chrono::high_resolution_clock::now();
        stamp(tiles, k_map_c, k_map_c, k_map, r, k_owner);
        const auto t1 = std::chrono::high_resolution_clock::now();
        const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
        std::printf("stamp r=%u: %.2f us\n", (unsigned)r, us);
    }

    delete[] tiles;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
