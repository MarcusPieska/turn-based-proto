//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "ai_unit_production_default.h"

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

//================================================================================================================================
//=> - AiUnitProductionDefault -
//================================================================================================================================

bool AiUnitProductionDefault::try_pick_land_unit (GameState& state, u16 city_idx, City* city) {
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
        const u16 def_u = UnitTypPick::pick_linear_right(state, units, state.m_land_defense_type_idx);
        if (def_u != U16_KEY_NULL) {
            city->build_unit(def_u);
            return true;
        }
        return false;
    }
    if (ps.m_last_turn_new_land_unit_build_support < ps.m_target_new_land_unit_support) {
        const u16 atk_u = UnitTypPick::pick_linear_right(state, units, state.m_land_attack_type_idx);
        if (atk_u != U16_KEY_NULL) {
            city->build_unit(atk_u);
            if (ps.m_target_new_land_unit_support > k_unit_sup_cost) {
                ps.m_target_new_land_unit_support = static_cast<u16>(ps.m_target_new_land_unit_support - k_unit_sup_cost);
            } else {
                ps.m_target_new_land_unit_support = 0;
            }
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
