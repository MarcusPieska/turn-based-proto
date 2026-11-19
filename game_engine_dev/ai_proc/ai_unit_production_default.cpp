//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "ai_unit_production_default.h"

#include "bit_array.h"
#include "city.h"
#include "city_turn_handler_core.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u16 k_unit_sup_cost = 1u;

static UnitAddStruct* find_city_def (GameState& state, u16 player, u16 x, u16 y) {
    u16 cur = state.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL) {
        UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        if (u->m_player_idx == player && u->m_is_city_defense != 0u) {
            return u;
        }
        cur = u->m_next_unit_on_tile;
    }
    return nullptr;
}

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
    if (city->try_city_def_up(city_idx)) {
        return true;
    }
    if (find_city_def(state, player, city->get_x(), city->get_y()) == nullptr) {
        if (city->get_free_land_unit_support() > 0u) {
            const u16 def_u = city->best_unit_of_type(city_idx, techs, &civ, state.m_land_defense_type_idx);
            if (def_u != U16_KEY_NULL) {
                city->build_unit(def_u);
                return true;
            }
            return false;
        }
    }
    if (ps.m_last_turn_new_land_unit_build_support < ps.m_target_new_land_unit_support) {
        const u16 atk_u = city->best_unit_of_type(city_idx, techs, &civ, state.m_land_attack_type_idx);
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
