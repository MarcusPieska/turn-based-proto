//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tile_req_chk.h"

#include "game_array_simple.h"
#include "game_map_defs.h"
#include "general_assessor.h"
#include "map_attribute_enum.h"
#include "map_climate_enum.h"
#include "map_overlay_enum.h"
#include "map_terrain_enum.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u8 terrain_id_for_idx (u16 idx) {
    switch (static_cast<MapTerrain>(idx)) {
        case MapTerrain::Ocean: return TERR_OCEAN[0];
        case MapTerrain::Sea: return TERR_SEA[0];
        case MapTerrain::Coastal: return TERR_COASTAL[0];
        case MapTerrain::Plains: return TERR_PLAINS[0];
        case MapTerrain::Hills: return TERR_HILLS[0];
        case MapTerrain::Mountains: return TERR_MOUNTAINS[0];
        case MapTerrain::Volcano: return TERR_VOLCANO[0];
        case MapTerrain::Inland_Sea: return TERR_INLAND_SEA[0];
        case MapTerrain::Inland_Lake: return TERR_INLAND_LAKE[0];
        default: return 0u;
    }
}

static u8 climate_id_for_idx (u16 idx) {
    switch (static_cast<MapClimate>(idx)) {
        case MapClimate::Desert: return CLIMATE_DESERT;
        case MapClimate::Plains: return CLIMATE_PLAINS;
        case MapClimate::Grassland: return CLIMATE_GRASSLAND;
        case MapClimate::Black_Soil: return CLIMATE_BLACK_SOIL;
        default: return CLIMATE_NONE;
    }
}

static bool chk_terrain (const GameArraySimple& map, u16 x, u16 y, u16 idx) {
    const u8 want = terrain_id_for_idx(idx);
    if (want == 0u) {
        return false;
    }
    return map.get_terrain(x, y) == want;
}

static bool chk_climate (const GameArraySimple& map, u16 x, u16 y, u16 idx) {
    const u8 want = climate_id_for_idx(idx);
    if (want == CLIMATE_NONE) {
        return false;
    }
    return map.get_climate(x, y) == want;
}

static bool chk_overlay (const GameArraySimple& map, u16 x, u16 y, u16 idx) {
    return map.get_overlay(x, y) == idx;
}

static bool chk_attribute (const GameArraySimple& map, u16 x, u16 y, u16 idx) {
    if (idx == static_cast<u16>(MapAttribute::River)) {
        return map.get_river(x, y) != 0u;
    }
    return false;
}

//================================================================================================================================
//=> - TileReqChk -
//================================================================================================================================

bool TileReqChk::chk_req (u8 kind, u16 idx, const AssessorCtx& ctx) {
    if (ctx.m_map == nullptr || idx == U16_KEY_NULL) {
        return false;
    }
    const GameArraySimple& map = *ctx.m_map;
    const u16 x = ctx.m_x;
    const u16 y = ctx.m_y;
    if (x >= map.width() || y >= map.height()) {
        return false;
    }
    switch (static_cast<TileReqKind>(kind)) {
        case TILE_REQ_KIND_TERRAIN:
            return chk_terrain(map, x, y, idx);
        case TILE_REQ_KIND_CLIMATE:
            return chk_climate(map, x, y, idx);
        case TILE_REQ_KIND_OVERLAY:
            return chk_overlay(map, x, y, idx);
        case TILE_REQ_KIND_ATTRIBUTE:
            return chk_attribute(map, x, y, idx);
        default:
            return false;
    }
}

bool TileReqChk::chk_reqs (const ItemReqsStruct& reqs, const AssessorCtx& ctx) {
    for (u32 j = 0; j < MAX_PREREQ_COUNT; ++j) {
        if (reqs.types[j] != ITEM_REQ_TYPE_TILE) {
            continue;
        }
        if (!chk_req(reqs.added_args[j], reqs.indices[j], ctx)) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
