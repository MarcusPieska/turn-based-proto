//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tile_yields.h"
#include "game_array_simple.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const GameArraySimple* TileYields::m_map = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u8 clamp_u8 (i32 v) {
    if (v <= 0) {
        return 0u;
    }
    if (v >= 255) {
        return 255u;
    }
    return static_cast<u8>(v);
}

static void add_row (i32* food, i32* prod, i32* comm, const TileAttributeStaticDataStruct& row) {
    *food += static_cast<i32>(row.food);
    *prod += static_cast<i32>(row.production);
    *comm += static_cast<i32>(row.commerce);
}

//================================================================================================================================
//=> - TileYields -
//================================================================================================================================

bool TileYields::setup (const RuntimeStatics& st) {
    return TileAttrTables::setup(st);
}

void TileYields::bind_map (const GameArraySimple* map) {
    m_map = map;
}

TileYield TileYields::get (u16 x, u16 y) {
    TileYield yld = {};
    if (m_map == nullptr || !TileAttrTables::ready()) {
        return yld;
    }
    if (x >= m_map->width() || y >= m_map->height()) {
        return yld;
    }
    i32 food = 0;
    i32 prod = 0;
    i32 comm = 0;
    add_row(&food, &prod, &comm, TileAttrTables::terr(m_map->get_terrain(x, y)));
    add_row(&food, &prod, &comm, TileAttrTables::clim(m_map->get_climate(x, y)));
    add_row(&food, &prod, &comm, TileAttrTables::ov(m_map->get_overlay(x, y)));
    if (m_map->get_river(x, y) != 0u) {
        add_row(&food, &prod, &comm, TileAttrTables::riv());
    }
    yld.m_food = clamp_u8(food);
    yld.m_production = clamp_u8(prod);
    yld.m_commerce = clamp_u8(comm);
    return yld;
}

bool TileYields::in_bounds (u16 x, u16 y) {
    if (m_map == nullptr) {
        return false;
    }
    return x < m_map->width() && y < m_map->height();
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
