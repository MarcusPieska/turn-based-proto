//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "ai_unit_production_aggressive.h"

#include "bit_array.h"
#include "city.h"
#include "city_turn_handler_core.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_typ_pick.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u16 k_unit_sup_cost = 1u;

static bool pick_build (City* city, GameState& state, BitArrayCL* units, u16 type_idx) {
    const u16 u = UnitTypPick::pick_linear_right(state, units, type_idx);
    if (u == U16_KEY_NULL) {
        return false;
    }
    city->build_unit(u);
    return true;
}

//================================================================================================================================
//=> - AiUnitProductionAggressive -
//================================================================================================================================

bool AiUnitProductionAggressive::try_pick_land_unit (GameState& state, u16 city_idx, City* city) {
    if (city == nullptr || !city->need_prod_pick()) {
        return false;
    }
    if (state.m_statics == nullptr || state.m_player_states == nullptr) {
        return false;
    }
    const u16 player = city->get_owner();
    if (player >= state.m_player_n) {
        return false;
    }
    PlayerState& ps = state.m_player_states[player];
    BitArrayCL civ(state.m_statics->civ().get_item_count());
    if (ps.m_civ_index < civ.get_count()) {
        civ.set_bit(ps.m_civ_index);
    }
    BitArrayCL* techs = ps.m_techs_researched;
    BitArrayCL* units = city->get_trainable_units(city_idx, techs, &civ);
    if (units == nullptr) {
        return false;
    }
    const u16 on_tile = CityTurnHandler_Core::own_land_sup_on_tile(state, player, city->get_x(), city->get_y());
    if (on_tile < city->get_free_land_unit_support()) {
        const u16 first = (on_tile & 1u) == 0u ? state.m_land_attack_type_idx : state.m_land_defense_type_idx;
        const u16 second = (on_tile & 1u) == 0u ? state.m_land_defense_type_idx : state.m_land_attack_type_idx;
        if (pick_build(city, state, units, first)) {
            return true;
        }
        return pick_build(city, state, units, second);
    }
    if (ps.m_last_turn_new_land_unit_build_support < ps.m_target_new_land_unit_support) {
        const u16 first = ps.m_ai_units_tog == 0u ? state.m_land_attack_type_idx : state.m_land_artillery_type_idx;
        const u16 second = ps.m_ai_units_tog == 0u ? state.m_land_artillery_type_idx : state.m_land_attack_type_idx;
        if (!pick_build(city, state, units, first) && !pick_build(city, state, units, second)) {
            return false;
        }
        ps.m_ai_units_tog = ps.m_ai_units_tog == 0u ? 1u : 0u;
        if (ps.m_target_new_land_unit_support > k_unit_sup_cost) {
            ps.m_target_new_land_unit_support = static_cast<u16>(ps.m_target_new_land_unit_support - k_unit_sup_cost);
        } else {
            ps.m_target_new_land_unit_support = 0;
        }
        return true;
    }
    return false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
