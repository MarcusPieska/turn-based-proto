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
#include "unit_domain_enum.h"
#include "unit_static_key.h"
#include "unit_type_action_map.h"
#include "worker_turn_handler_mk2.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u16 k_act_is_land = 0u;
static const u16 k_unit_sup_cost = 1u;
static const u16 k_workers_per_city = 1u;
static const u16 k_worker_land_on_tile = 2u;
static const u16 k_assess_mod = 10u;

static bool unit_typ_is_land (const GameState& state, u16 unit_typ_idx) {
    if (state.m_statics == nullptr || unit_typ_idx == U16_KEY_NULL) {
        return false;
    }
    const UnitStaticDataStruct& u = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_typ_idx));
    return state.m_statics->unit_type_action_map().unit_type_can_do(u.type, k_act_is_land);
}

static bool unit_typ_domain_land (const GameState& state, u16 unit_typ_idx) {
    if (state.m_statics == nullptr || unit_typ_idx == U16_KEY_NULL) {
        return false;
    }
    return state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_typ_idx)).domain
        == static_cast<u16>(UnitDomain::LAND);
}

static u16 own_land_domain_on_tile (GameState& state, u16 player, u16 x, u16 y) {
    u16 sum = 0;
    u16 cur = state.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL) {
        const UnitAddKey key = UnitAddKey::from_raw(cur);
        const UnitAddStruct* u = state.m_units.get_unit_add(key);
        if (u == nullptr) {
            break;
        }
        if (u->m_player_idx == player && unit_typ_domain_land(state, u->m_unit_typ_idx)) {
            const u32 next = static_cast<u32>(sum) + 1u;
            sum = next > 65535u ? 65535u : static_cast<u16>(next);
        }
        cur = u->m_next_unit_on_tile;
    }
    return sum;
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

u16 CityTurnHandler_Core::find_worker_typ (const GameState& state, const BitArrayCL* units) {
    if (units == nullptr || state.m_statics == nullptr) {
        return U16_KEY_NULL;
    }
    const u32 n = units->get_count();
    for (u32 i = 0; i < n; ++i) {
        if (units->get_bit(i) == 0) {
            continue;
        }
        const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(static_cast<u16>(i));
        if (state.m_statics->unit().get_item(uk).type == state.m_land_worker_type_idx) {
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

bool CityTurnHandler_Core::need_worker (const GameState& state, u16 player) {
    if (state.m_player_states == nullptr || player >= state.m_player_n) {
        return false;
    }
    const PlayerState& ps = state.m_player_states[player];
    const u32 have = static_cast<u32>(ps.m_last_turn_worker_count)
        + static_cast<u32>(ps.m_last_turn_worker_build_n)
        + static_cast<u32>(ps.m_this_turn_worker_build_n);
    const u32 want = static_cast<u32>(k_workers_per_city) * static_cast<u32>(ps.m_last_turn_city_count);
    return have < want;
}

bool CityTurnHandler_Core::try_pick_worker (GameState& state, u16 city_idx, City* city) {
    if (city == nullptr || !city->need_prod_pick() || state.m_statics == nullptr || state.m_player_states == nullptr) {
        return false;
    }
    const u16 player = city->get_owner();
    if (player >= state.m_player_n || !need_worker(state, player)) {
        return false;
    }
    if (own_land_domain_on_tile(state, player, city->get_x(), city->get_y()) < k_worker_land_on_tile) {
        return false;
    }
    PlayerState& ps = state.m_player_states[player];
    BitArrayCL civ(state.m_statics->civ().get_item_count());
    if (ps.m_civ_index < civ.get_count()) {
        civ.set_bit(ps.m_civ_index);
    }
    BitArrayCL* units = city->get_trainable_units(city_idx, ps.m_techs_researched, &civ);
    const u16 worker = find_worker_typ(state, units);
    if (worker == U16_KEY_NULL) {
        return false;
    }
    city->build_unit(worker);
    return true;
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

void CityTurnHandler_Core::try_assess_imps (GameState& state, u16 city_idx, City* city) {
    if (city == nullptr || city->get_tile_imp_count() != 0u) {
        return;
    }
    if ((city_idx % k_assess_mod) != (static_cast<u16>(state.m_current_turn) % k_assess_mod)) {
        return;
    }
    WorkerTurnHandlerMk2::assess(state, city_idx);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
