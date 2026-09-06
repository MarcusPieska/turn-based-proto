//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "unit_turn_handler.h"

#include "build_adds_array.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

bool UnitTurnHandler::has_full_mp (const GameState& state, const UnitAddStruct& u) {
    if (state.m_statics == nullptr) {
        return false;
    }
    const u16 pts = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u.m_unit_typ_idx)).mvt_pts;
    const u16 turn = state.m_statics->config().get_mov_pt_per_turn();
    const i16 full = static_cast<i16>(pts * turn);
    return full > 0 && u.m_mvt_points >= full;
}

bool UnitTurnHandler::tile_is_city (const GameState& state, u16 x, u16 y) {
    if (x >= state.m_map.width() || y >= state.m_map.height()) {
        return false;
    }
    return state.m_map.get_add_typ(x, y) == BUILD_ADD_CITY;
}

void UnitTurnHandler::heal_one (GameState& state, u16 unit_idx, u16 tile_x, u16 tile_y) {
    UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (u == nullptr || u->m_health == 0u || u->m_health >= UNIT_HEALTH) {
        return;
    }
    if (!has_full_mp(state, *u)) {
        return;
    }
    const u16 pct = tile_is_city(state, tile_x, tile_y)
        ? state.m_statics->config().get_unit_heal_in_city()
        : state.m_statics->config().get_unit_heal_default();
    const u16 add = static_cast<u16>((static_cast<u32>(UNIT_HEALTH) * static_cast<u32>(pct)) / 100u);
    if (add == 0u) {
        return;
    }
    const u16 next = static_cast<u16>(u->m_health) + add;
    u->m_health = static_cast<u8>(next > UNIT_HEALTH ? UNIT_HEALTH : next);
}

//================================================================================================================================
//=> - UnitTurnHandler -
//================================================================================================================================

void UnitTurnHandler::handle (GameState& state, u16 unit_idx) {
    UnitAddStruct* head = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (head == nullptr || head->m_x == U16_KEY_NULL) {
        return;
    }
    const u16 tx = static_cast<u16>(head->m_x);
    const u16 ty = static_cast<u16>(head->m_y);
    UnitAddKey cur = UnitAddKey::from_raw(unit_idx);
    while (cur.is_valid()) {
        heal_one(state, cur.value(), tx, ty);
        UnitAddStruct* u = state.m_units.get_unit_add(cur);
        if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
