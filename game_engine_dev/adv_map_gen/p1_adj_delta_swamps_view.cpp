//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "p1_adj_delta_swamps_view.h"

#include "game_map_defs.h"
#include "map_terrain_validate.h"
#include "p1_gen_river_network.h"
#include "res_placement_defs.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

struct Rng32 {
    u32 m_s;
};

static void rng_seed (Rng32* g, u32 seed) {
    g->m_s = seed != 0u ? seed : 1u;
}

static u32 rng_next (Rng32* g) {
    g->m_s = g->m_s * 1664525u + 1013904223u;
    return g->m_s;
}

static bool is_water_cls (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool wr_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
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

static bool fill_pri_rgb (
    u8* rgb,
    const u8* terrain,
    const u8* climate,
    const u8* riv,
    const u8* res_ov,
    u32 n) 
{
    if (rgb == nullptr || terrain == nullptr || climate == nullptr || res_ov == nullptr) {
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
        if (res_ov[i] == static_cast<u8>(RES_OV_SWAMPS)) {
            r = 255;
            g = 0;
            b = 0;
        } else if (res_ov[i] == static_cast<u8>(RES_OV_FORESTS)) {
            r = 0;
            g = 100;
            b = 0;
        } else if (res_ov[i] == static_cast<u8>(RES_OV_JUNGLES)) {
            r = 0;
            g = 160;
            b = 40;
        }
        if (riv != nullptr && riv[i] != 0) {
            r = 0;
            g = 0;
            b = 255;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    return true;
}

static bool fill_sec_rgb (
    u8* rgb,
    const u8* terrain,
    const u8* climate,
    const u8* riv,
    u32 n) 
{
    if (rgb == nullptr || terrain == nullptr || climate == nullptr) {
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
        if (riv != nullptr && riv[i] != 0) {
            r = 0;
            g = 0;
            b = 255;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    return true;
}

//================================================================================================================================
//=> - P1_Adj_DeltaSwampsView -
//================================================================================================================================

bool P1_Adj_DeltaSwampsView::save_pri (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* riv,
    const u8* res_ov,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || climate == nullptr || res_ov == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    const bool fill_ok = fill_pri_rgb(rgb, terrain, climate, riv, res_ov, n);
    const bool ok = fill_ok && wr_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool P1_Adj_DeltaSwampsView::save_sec (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* riv,
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
    const bool fill_ok = fill_sec_rgb(rgb, terrain, climate, riv, n);
    const bool ok = fill_ok && wr_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool P1_Adj_DeltaSwampsView::save_wshed_extra (
    cstr path,
    u32 seed,
    const u8* terrain,
    const u16* wshed,
    const u8* riv,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || wshed == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16 max_b = 0;
    for (u32 i = 0; i < n; ++i) {
        if (wshed[i] > max_b) {
            max_b = wshed[i];
        }
    }
    const u16 pal_cap = static_cast<u16>(max_b + 1u);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    u8* pal = new u8[static_cast<size_t>(pal_cap) * 3u];
    bool* pal_set = new bool[pal_cap];
    if (rgb == nullptr || pal == nullptr || pal_set == nullptr) {
        delete[] pal_set;
        delete[] pal;
        delete[] rgb;
        return false;
    }
    for (u16 bi = 0; bi < pal_cap; ++bi) {
        pal_set[bi] = false;
    }
    Rng32 rng;
    rng_seed(&rng, seed ^ 0xA5A5A5A5u);
    for (u32 i = 0; i < n; ++i) {
        u8 r = 32;
        u8 g = 32;
        u8 b = 32;
        if (!is_water_cls(terrain[i])) {
            MapTerrainValidate::rgb_from_class(terrain[i], &r, &g, &b);
            r = static_cast<u8>(r / 3u);
            g = static_cast<u8>(g / 3u);
            b = static_cast<u8>(b / 3u);
        }
        const u16 bid = wshed[i];
        if (bid != static_cast<u16>(P1_RIVER_BASIN_NONE) && bid < pal_cap) {
            if (!pal_set[bid]) {
                pal[bid * 3u + 0] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
                pal[bid * 3u + 1] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
                pal[bid * 3u + 2] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
                pal_set[bid] = true;
            }
            r = pal[bid * 3u + 0];
            g = pal[bid * 3u + 1];
            b = pal[bid * 3u + 2];
        }
        if (riv != nullptr && riv[i] != 0) {
            r = 0;
            g = 0;
            b = 255;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = wr_rgb_ppm(path, rgb, w, h);
    delete[] pal_set;
    delete[] pal;
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
