//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tile_yields.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const GameArraySimple* TileYields::m_map = nullptr;

//================================================================================================================================
//=> - TileYields -
//================================================================================================================================

void TileYields::bind_map (const GameArraySimple* map) {
    m_map = map;
}

TileYield TileYields::get (u16 x, u16 y) {
    TileYield yld = {};
    if (m_map == nullptr) {
        return yld;
    }
    if (x >= m_map->width() || y >= m_map->height()) {
        return yld;
    }
    const u8 clim = m_map->get_climate(x, y);
    if (clim == CLIMATE_DESERT) {
        yld.m_food = 0;
    } else if (clim == CLIMATE_PLAINS) {
        yld.m_food = 2;
    } else if (clim == CLIMATE_GRASSLAND) {
        yld.m_food = 3;
    } else if (clim == CLIMATE_BLACK_SOIL) {
        yld.m_food = 4;
    }
    const u8 terr = m_map->get_terrain(x, y);
    if (terr == TERR_PLAINS[0]) {
        yld.m_production = 1;
    }
    if (terr == TERR_HILLS[0]) {
        yld.m_production = 2;
        if (yld.m_food > 0) {
            yld.m_food -= 1;
        }
    }
    if (m_map->get_river(x, y) != 0) {
        yld.m_commerce = 1;
    }
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
