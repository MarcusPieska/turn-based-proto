//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "std_add_helper.h"
#include "assert_log.h"
#include "build_adds_array.h"
#include "game_array_simple.h" 

//================================================================================================================================
//=> - StdAddHelper -
//================================================================================================================================

bool StdAddHelper::has_farm (const GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::has_farm null tile");
    GAME_EXPECT(t->m_add_typ == BUILD_ADD_STD, "StdAddHelper::has_farm not BUILD_ADD_STD");
    return (static_cast<u16>(t->m_add_idx) & m_farm_bit) != 0u;
}

void StdAddHelper::set_farm (GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::set_farm null tile");
    GAME_EXPECT(t->m_add_typ == BUILD_ADD_STD, "StdAddHelper::set_farm not BUILD_ADD_STD");
    t->m_add_idx = static_cast<u16>(t->m_add_idx) | m_farm_bit;
}

bool StdAddHelper::has_mill (const GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::has_mill null tile");
    GAME_EXPECT(t->m_add_typ == BUILD_ADD_STD, "StdAddHelper::has_mill not BUILD_ADD_STD");
    return (static_cast<u16>(t->m_add_idx) & m_mill_bit) != 0u;
}

void StdAddHelper::set_mill (GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::set_mill null tile");
    GAME_EXPECT(t->m_add_typ == BUILD_ADD_STD, "StdAddHelper::set_mill not BUILD_ADD_STD");
    t->m_add_idx = static_cast<u16>(t->m_add_idx) | m_mill_bit;
}

bool StdAddHelper::has_irr (const GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::has_irr null tile");
    GAME_EXPECT(t->m_add_typ == BUILD_ADD_STD, "StdAddHelper::has_irr not BUILD_ADD_STD");
    return (static_cast<u16>(t->m_add_idx) & m_irr_bit) != 0u;
}

void StdAddHelper::set_irr (GameTileSimple* t) {
    GAME_EXPECT(t != nullptr, "StdAddHelper::set_irr null tile");
    GAME_EXPECT(t->m_add_typ == BUILD_ADD_STD, "StdAddHelper::set_irr not BUILD_ADD_STD");
    t->m_add_idx = static_cast<u16>(t->m_add_idx) | m_irr_bit;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
