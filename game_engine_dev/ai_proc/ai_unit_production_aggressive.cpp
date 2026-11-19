//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "ai_unit_production_aggressive.h"

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

static bool pick_build (City* city, u16 city_idx, BitArrayCL* techs, BitArrayCL* civ, u16 type_idx) {
    const u16 u = city->best_unit_of_type(city_idx, techs, civ, type_idx);
    if (u == U16_KEY_NULL) {
        return false;
    }
    city->build_unit(u);
    return true;
}

static u16 want_n (const PlayerState& ps) {
    const float t = static_cast<float>(ps.m_free_land_unit_support) * PlayerState::k_land_army_sup_fac;
    if (t <= 0.0f) {
        return 0u;
    }
    const u32 r = static_cast<u32>(t + 0.5f);
    return r > 65535u ? 65535u : static_cast<u16>(r);
}

static void bump_role (ArmyRoleCount& c, u16 role) {
    if (role == 0u) {
        c.add_def(1);
    } else if (role == 1u) {
        c.add_atk(1);
    } else {
        c.add_brg(1);
    }
}

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
    if (city->try_city_def_up(city_idx)) {
        return true;
    }
    if (find_city_def(state, player, city->get_x(), city->get_y()) == nullptr) {
        if (city->get_free_land_unit_support() > 0u) {
            if (!pick_build(city, city_idx, techs, &civ, state.m_land_defense_type_idx)) {
                return false;
            }
            return true;
        }
    }
    const u16 want = want_n(ps);
    const u16 role_cap = static_cast<u16>(want / 3u);
    if (role_cap == 0u) {
        return false;
    }
    const u16 roles[3] = {
        state.m_land_defense_type_idx,
        state.m_land_attack_type_idx,
        state.m_land_artillery_type_idx
    };
    const u16 counts[3] = {
        ps.m_army_roles.def(),
        ps.m_army_roles.atk(),
        ps.m_army_roles.brg()
    };
    u16 ord[3] = {0u, 1u, 2u};
    for (u16 a = 0u; a < 3u; ++a) {
        for (u16 b = static_cast<u16>(a + 1u); b < 3u; ++b) {
            if (counts[ord[b]] < counts[ord[a]]) {
                const u16 t = ord[a];
                ord[a] = ord[b];
                ord[b] = t;
            }
        }
    }
    for (u16 i = 0u; i < 3u; ++i) {
        const u16 r = ord[i];
        if (counts[r] >= role_cap) {
            continue;
        }
        if (pick_build(city, city_idx, techs, &civ, roles[r])) {
            if (r != 0u || find_city_def(state, player, city->get_x(), city->get_y()) != nullptr) {
                bump_role(ps.m_army_roles, r);
            }
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
