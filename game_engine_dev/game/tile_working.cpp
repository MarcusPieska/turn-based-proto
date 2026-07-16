//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tile_working.h"
#include "assert_log.h"
#include "game_array_simple.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

GameArraySimple* TileWorking::m_map = nullptr;

//================================================================================================================================
//=> - TileWorking -
//================================================================================================================================

void TileWorking::bind_map (GameArraySimple* map) {
    m_map = map;
}

bool TileWorking::in_bounds (u16 x, u16 y) {
    GAME_EXPECT(m_map != nullptr, "TileWorking map");
    return x < m_map->width() && y < m_map->height();
}

u16 TileWorking::get_worker (u16 x, u16 y) {
    GAME_EXPECT(m_map != nullptr, "TileWorking map");
    return m_map->get_city_worker(x, y);
}

bool TileWorking::mark_worked (u16 x, u16 y, u16 city_idx) {
    GAME_EXPECT(m_map != nullptr, "TileWorking map");
    return m_map->set_city_worker(x, y, city_idx);
}

void TileWorking::clear_worked (u16 x, u16 y, u16 city_idx) {
    GAME_EXPECT(m_map != nullptr, "TileWorking map");
    if (m_map->get_city_worker(x, y) == city_idx) {
        m_map->set_city_worker(x, y, U16_KEY_NULL);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
