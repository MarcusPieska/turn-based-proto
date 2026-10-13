//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "general_move_boost.h"

#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

bool GeneralMoveBoost::unit_has_general (const GameState& s, u16 seat, u16 unit_idx) {
    if (s.m_player_states == nullptr || seat >= s.m_player_n || unit_idx == U16_KEY_NULL) {
        return false;
    }
    const PlayerState& ps = s.m_player_states[seat];
    for (u16 i = 0; i < static_cast<u16>(GENERAL_SLOTS); ++i) {
        const u16 g = ps.m_general_idx[i];
        if (g == U16_KEY_NULL) {
            continue;
        }
        if (g == unit_idx) {
            return true;
        }
        UnitAddKey cur = UnitAddKey::from_raw(g);
        while (cur.is_valid()) {
            if (cur.value() == unit_idx) {
                return true;
            }
            const UnitAddStruct* u = s.m_units.get_unit_add(cur);
            if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
        }
    }
    return false;
}

bool GeneralMoveBoost::chain_has_general (const GameState& s, u16 seat, UnitAddKey head) {
    if (!head.is_valid() || s.m_player_states == nullptr || seat >= s.m_player_n) {
        return false;
    }
    const PlayerState& ps = s.m_player_states[seat];
    for (u16 i = 0; i < static_cast<u16>(GENERAL_SLOTS); ++i) {
        const u16 g = ps.m_general_idx[i];
        if (g == U16_KEY_NULL) {
            continue;
        }
        UnitAddKey cur = head;
        while (cur.is_valid()) {
            if (cur.value() == g) {
                return true;
            }
            const UnitAddStruct* u = s.m_units.get_unit_add(cur);
            if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
        }
    }
    return false;
}

//================================================================================================================================
//=> - GeneralMoveBoost -
//================================================================================================================================

void GeneralMoveBoost::on_unit_refilled (GameState& s, u16 unit_idx) {
    if (s.m_statics == nullptr || unit_idx == U16_KEY_NULL) {
        return;
    }
    UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (u == nullptr || u->m_x == U16_KEY_NULL) {
        return;
    }
    const u16 seat = static_cast<u16>(u->m_player_idx);
    if (!unit_has_general(s, seat, unit_idx)) {
        return;
    }
    const u16 turn_mp = s.m_statics->config().get_mov_pt_per_turn();
    if (turn_mp == 0u) {
        return;
    }
    u->m_mvt_points = static_cast<i16>(static_cast<i32>(u->m_mvt_points) + static_cast<i32>(turn_mp));
}

void GeneralMoveBoost::on_grp_refilled (GameState& s, UnitAddKey head) {
    if (s.m_statics == nullptr || !head.is_valid()) {
        return;
    }
    const UnitAddStruct* hu = s.m_units.get_unit_add(head);
    if (hu == nullptr) {
        return;
    }
    const u16 seat = static_cast<u16>(hu->m_player_idx);
    if (!chain_has_general(s, seat, head)) {
        return;
    }
    const u16 turn_mp = s.m_statics->config().get_mov_pt_per_turn();
    if (turn_mp == 0u) {
        return;
    }
    UnitAddKey cur = head;
    while (cur.is_valid()) {
        UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        u->m_mvt_points = static_cast<i16>(static_cast<i32>(u->m_mvt_points) + static_cast<i32>(turn_mp));
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
