//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_map_climate.h"

#include "generator_constants.h"

#include <algorithm>
#include <cstring>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static bool is_open_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

static u8 clamp_wt (u8 w) {
    return (w > CLIMATE_WT_MAX) ? CLIMATE_WT_MAX : w;
}

static u32 tile_score (
    u8 river,
    u8 open_w,
    u8 lat,
    u8 plain_w,
    const MapClimateOverlayWts* wts) 
{
    const u32 wr = static_cast<u32>(clamp_wt(wts->w_dist_river));
    const u32 wo = static_cast<u32>(clamp_wt(wts->w_open_dist_water));
    const u32 wl = static_cast<u32>(clamp_wt(wts->w_latitude));
    const u32 wp = static_cast<u32>(clamp_wt(wts->w_plain_dist_water));
    const u32 sr = static_cast<u32>(255u - static_cast<u32>(river));
    const u32 so = static_cast<u32>(255u - static_cast<u32>(open_w));
    const u32 sl = static_cast<u32>(255u - static_cast<u32>(lat));
    const u32 sp = static_cast<u32>(255u - static_cast<u32>(plain_w));
    return sr * wr + so * wo + sl * wl + sp * wp;
}

static u32 pct_count (u32 n, u8 pct) {
    return (n * static_cast<u32>(pct) + 50u) / 100u;
}

//================================================================================================================================
//=> - Generate_MapClimate -
//================================================================================================================================

MapClimateResult* Generate_MapClimate::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const MapClimateOverlays* overlays,
    const MapClimateParams* params) 
{
    if (terrain == nullptr || overlays == nullptr || params == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    if (overlays->dist_river == nullptr || overlays->open_dist_water == nullptr
        || overlays->latitude == nullptr || overlays->plain_dist_water == nullptr) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 tile_n = 0;
    for (u32 i = 0; i < n; ++i) {
        if (is_open_land(terrain[i])) {
            tile_n++;
        }
    }
    MapClimateResult* out = new MapClimateResult();
    if (out == nullptr) {
        return nullptr;
    }
    out->w = w;
    out->h = h;
    out->tile_n = tile_n;
    out->climate = new u8[n];
    out->tiles = (tile_n > 0) ? new MapClimateTileVal[tile_n] : nullptr;
    if (out->climate == nullptr || (tile_n > 0 && out->tiles == nullptr)) {
        delete[] out->climate;
        delete[] out->tiles;
        delete out;
        return nullptr;
    }
    std::memset(out->climate, CLIMATE_NONE, n * sizeof(u8));
    u32 ti = 0;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_open_land(terrain[i])) {
                continue;
            }
            MapClimateTileVal* e = &out->tiles[ti];
            e->x = px;
            e->y = py;
            e->val = tile_score(
                overlays->dist_river[i],
                overlays->open_dist_water[i],
                overlays->latitude[i],
                overlays->plain_dist_water[i],
                &params->wts);
            ti++;
        }
    }
    if (tile_n == 0) {
        return out;
    }
    std::sort(out->tiles, out->tiles + tile_n, [](const MapClimateTileVal& a, const MapClimateTileVal& b) {
        if (a.val != b.val) {
            return a.val > b.val;
        }
        if (a.y != b.y) {
            return a.y < b.y;
        }
        return a.x < b.x;
    });
    const u32 grass_n = pct_count(tile_n, params->pct.pct_grassland);
    const u32 plains_n = pct_count(tile_n, params->pct.pct_plains);
    for (u32 k = 0; k < tile_n; ++k) {
        const MapClimateTileVal* e = &out->tiles[k];
        const u32 idx = static_cast<u32>(e->y) * static_cast<u32>(w) + static_cast<u32>(e->x);
        u8 cls = CLIMATE_DESERT;
        if (k < grass_n) {
            cls = CLIMATE_GRASSLAND;
        } else if (k < grass_n + plains_n) {
            cls = CLIMATE_PLAINS;
        }
        out->climate[idx] = cls;
    }
    return out;
}

void Generate_MapClimate::free_result (MapClimateResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->climate;
    delete[] res->tiles;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
