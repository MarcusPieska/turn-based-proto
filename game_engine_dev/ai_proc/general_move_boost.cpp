//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "general_move_boost.h"

#include "army_general.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool in_gen_grp (GameState& s, u16 seat, u16 unit_idx) {
    if (s.m_player_states == nullptr || seat >= s.m_player_n || unit_idx == U16_KEY_NULL) {
        return false;
    }
    const PlayerState& ps = s.m_player_states[seat];
    for (u16 i = 0; i < static_cast<u16>(GENERAL_SLOTS); ++i) {
        const u16 hd = ps.m_generals[i].grp();
        if (hd == U16_KEY_NULL) {
            continue;
        }
        u16 cur = hd;
        while (cur != U16_KEY_NULL) {
            if (cur == unit_idx) {
                return true;
            }
            const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(cur));
            if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = static_cast<u16>(u->m_next_unit_in_group);
        }
    }
    return false;
}

static ArmyGeneral* gen_for_hd (GameState& s, u16 seat, UnitAddKey head) {
    if (!head.is_valid() || s.m_player_states == nullptr || seat >= s.m_player_n) {
        return nullptr;
    }
    PlayerState& ps = s.m_player_states[seat];
    for (u16 i = 0; i < static_cast<u16>(GENERAL_SLOTS); ++i) {
        ArmyGeneral& g = ps.m_generals[i];
        const u16 hd = g.grp();
        if (hd == U16_KEY_NULL) {
            continue;
        }
        u16 cur = head.value();
        while (cur != U16_KEY_NULL) {
            if (cur == hd) {
                return &g;
            }
            const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(cur));
            if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = static_cast<u16>(u->m_next_unit_in_group);
        }
    }
    return nullptr;
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
    if (!in_gen_grp(s, static_cast<u16>(u->m_player_idx), unit_idx)) {
        return;
    }
    const u16 turn_mp = s.m_statics->config().get_mov_pt_per_turn();
    if (turn_mp == 0u) {
        return;
    }
    u->m_mvt_points = static_cast<i16>(static_cast<i32>(u->m_mvt_points) + static_cast<i32>(turn_mp));
}

void GeneralMoveBoost::on_grp_refilled (GameState& s, UnitAddKey head) {
    const UnitAddStruct* hu = s.m_units.get_unit_add(head);
    if (hu == nullptr) {
        return;
    }
    ArmyGeneral* g = gen_for_hd(s, static_cast<u16>(hu->m_player_idx), head);
    if (g == nullptr) {
        return;
    }
    g->boost_mvt_pts(s);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
