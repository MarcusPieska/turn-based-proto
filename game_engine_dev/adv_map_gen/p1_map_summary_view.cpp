//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_map_summary_view.h"

#include "game_map_defs.h"
#include "map_terrain_validate.h"

#include <cstdio>

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static bool is_water_terr (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]
        || cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

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

static void summary_rgb (
    const u8* terrain,
    const u8* climate,
    const u8* rivers,
    u32 i,
    u8* rr,
    u8* gg,
    u8* bb) 
{
    MapTerrainValidate::rgb_from_class(terrain[i], rr, gg, bb);
    const u8 cl = climate[i];
    if (cl != CLIMATE_NONE) {
        climate_to_rgb(cl, rr, gg, bb);
    }
    if (terrain[i] == TERR_MOUNTAINS[0]) {
        *rr = TERR_MOUNTAINS[1];
        *gg = TERR_MOUNTAINS[2];
        *bb = TERR_MOUNTAINS[3];
    }
    if (rivers != nullptr && rivers[i] != 0 && !is_water_terr(terrain[i])) {
        *rr = 0;
        *gg = 0;
        *bb = 255;
    }
}

static void bleach_desat (u8* rr, u8* gg, u8* bb) {
    const u32 g = (static_cast<u32>(*rr) + static_cast<u32>(*gg) + static_cast<u32>(*bb)) / 3u;
    *rr = static_cast<u8>((g * 2u + 255u * 3u) / 5u);
    *gg = static_cast<u8>((g * 2u + 255u * 3u) / 5u);
    *bb = static_cast<u8>((g * 2u + 255u * 3u) / 5u);
}

//================================================================================================================================
//=> - P1_MapSummaryView -
//================================================================================================================================

bool P1_MapSummaryView::save (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* rivers,
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
        u8 rr = 0;
        u8 gg = 0;
        u8 bb = 0;
        summary_rgb(terrain, climate, rivers, i, &rr, &gg, &bb);
        rgb[i * 3u + 0] = rr;
        rgb[i * 3u + 1] = gg;
        rgb[i * 3u + 2] = bb;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool P1_MapSummaryView::save_bleached_mark (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* rivers,
    const u8* mark,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || climate == nullptr || mark == nullptr
        || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 rr = 0;
        u8 gg = 0;
        u8 bb = 0;
        summary_rgb(terrain, climate, rivers, i, &rr, &gg, &bb);
        bleach_desat(&rr, &gg, &bb);
        if (mark[i] != 0) {
            rr = 255;
            gg = 0;
            bb = 0;
        }
        rgb[i * 3u + 0] = rr;
        rgb[i * 3u + 1] = gg;
        rgb[i * 3u + 2] = bb;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
