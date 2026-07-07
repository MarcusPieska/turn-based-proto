//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <new>

#include "game_state.h"
#include "unit_movement_mng.h"

//================================================================================================================================
//=> - GameState -
//================================================================================================================================

void GameState::clear () {
    if (m_player_states != nullptr) {
        for (u16 i = 0; i < m_player_n; ++i) {
            delete m_player_states[i].m_explored_overlay;
            m_player_states[i].m_explored_overlay = nullptr;
            delete m_player_states[i].m_techs_researched;
            m_player_states[i].m_techs_researched = nullptr;
        }
        delete[] m_player_states;
        m_player_states = nullptr;
    }
    delete m_tile_ownership_array;
    m_tile_ownership_array = nullptr;
    delete m_built_wonders;
    m_built_wonders = nullptr;
    m_civ_relations.reset(0);
    m_map.clear();
    m_units.~UnitAddVector();
    new (&m_units) UnitAddVector();
    m_statics = nullptr;
    m_current_turn = 0;
    m_player_n = 0;
    m_players_remaining = 0;
}

bool GameState::spawn (u16 x, u16 y, u16 player_idx, const u16* typ_idxs, u16 typ_n) {
    if (typ_idxs == nullptr || typ_n == 0) {
        return false;
    }
    for (u16 i = 0; i < typ_n; ++i) {
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(*this, x, y, player_idx, typ_idxs[i], &key)) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
