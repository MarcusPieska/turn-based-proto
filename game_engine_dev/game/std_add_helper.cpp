//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "std_add_helper.h"
#include "assert_log.h"
#include "game_array_simple.h"
#include "map_overlay_enum.h"

//================================================================================================================================
//=> - StdAddHelper -
//================================================================================================================================

bool StdAddHelper::has_farm (const GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::has_farm null tile");
    return static_cast<u16>(t->m_ov) == static_cast<u16>(MapOverlay::Farm);
}

void StdAddHelper::set_farm (GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::set_farm null tile");
    t->m_ov = static_cast<u16>(MapOverlay::Farm);
    t->m_add_idx = 0u;
}

bool StdAddHelper::has_mill (const GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::has_mill null tile");
    const u16 ov = static_cast<u16>(t->m_ov);
    if (ov != static_cast<u16>(MapOverlay::Farm) && ov != static_cast<u16>(MapOverlay::Forest)) {
        return false;
    }
    if (ov == static_cast<u16>(MapOverlay::Forest)) {
        return (static_cast<u16>(t->m_add_idx) & 1u) != 0u;
    }
    return (static_cast<u16>(t->m_add_idx) & m_mill_bit) != 0u;
}

void StdAddHelper::set_mill (GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::set_mill null tile");
    const u16 ov = static_cast<u16>(t->m_ov);
    if (ov == static_cast<u16>(MapOverlay::Farm)) {
        t->m_add_idx = static_cast<u16>(t->m_add_idx) | m_mill_bit;
        return;
    }
    t->m_ov = static_cast<u16>(MapOverlay::Forest);
    t->m_add_idx = static_cast<u16>(t->m_add_idx) | 1u;
}

bool StdAddHelper::has_irr (const GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::has_irr null tile");
    if (static_cast<u16>(t->m_ov) != static_cast<u16>(MapOverlay::Farm)) {
        return false;
    }
    return (static_cast<u16>(t->m_add_idx) & m_irr_bit) != 0u;
}

void StdAddHelper::set_irr (GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::set_irr null tile");
    GAME_EXPECT(static_cast<u16>(t->m_ov) == static_cast<u16>(MapOverlay::Farm), "StdAddHelper::set_irr not Farm");
    t->m_add_idx = static_cast<u16>(t->m_add_idx) | m_irr_bit;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
