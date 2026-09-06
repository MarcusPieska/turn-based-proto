//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_turn_handler_core.h"

#include "ai_unit_production_aggressive.h"
#include "ai_unit_production_default.h"
#include "bit_array.h"
#include "city.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "unit_type_action_map.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u16 k_act_is_land = 0u;
static const u16 k_unit_sup_cost = 1u;

static bool unit_typ_is_land (const GameState& state, u16 unit_typ_idx) {
    if (state.m_statics == nullptr || unit_typ_idx == U16_KEY_NULL) {
        return false;
    }
    const UnitStaticDataStruct& u = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_typ_idx));
    return state.m_statics->unit_type_action_map().unit_type_can_do(u.type, k_act_is_land);
}

//================================================================================================================================
//=> - CityTurnHandler_Core -
//================================================================================================================================

u16 CityTurnHandler_Core::find_settler_typ (const GameState& state, const BitArrayCL* units) {
    const u32 n = units->get_count();
    for (u32 i = 0; i < n; ++i) {
        if (units->get_bit(i) == 0) {
            continue;
        }
        const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(static_cast<u16>(i));
        if (state.m_statics->unit().get_item(uk).type == state.m_land_settler_type_idx) {
            return static_cast<u16>(i);
        }
    }
    return U16_KEY_NULL;
}

u16 CityTurnHandler_Core::own_land_sup_on_tile (GameState& state, u16 player, u16 x, u16 y) {
    u16 sum = 0;
    u16 cur = state.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL) {
        const UnitAddKey key = UnitAddKey::from_raw(cur);
        const UnitAddStruct* u = state.m_units.get_unit_add(key);
        if (u == nullptr) {
            break;
        }
        if (u->m_player_idx == player && unit_typ_is_land(state, u->m_unit_typ_idx)) {
            const u32 next = static_cast<u32>(sum) + static_cast<u32>(k_unit_sup_cost);
            sum = next > 65535u ? 65535u : static_cast<u16>(next);
        }
        cur = u->m_next_unit_on_tile;
    }
    return sum;
}

bool CityTurnHandler_Core::try_pick_land_unit (GameState& state, u16 city_idx, City* city) {
    if (city == nullptr || state.m_player_states == nullptr) {
        return false;
    }
    const u16 player = city->get_owner();
    if (player >= state.m_player_n) {
        return false;
    }
    switch (state.m_player_states[player].m_ai_units) {
    case AiUnits::AI_UNITS_AGGRESSIVE:
        return AiUnitProductionAggressive::try_pick_land_unit(state, city_idx, city);
    case AiUnits::AI_UNITS_DEFAULT:
    default:
        return AiUnitProductionDefault::try_pick_land_unit(state, city_idx, city);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
