//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_turn_handler.h"

#include "assert_log.h"
#include "bit_array.h"
#include "city.h"
#include "city_tile_manager.h"
#include "game_state.h"
#include "linear_bld.h"
#include "runtime_statics.h"
#include "settler_turn_handler.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 find_settler_typ (const GameState& state, const BitArrayCL* units) {
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

static void pick_prod (GameState& state, u16 city_idx, City* city) {
    GAME_EXPECT(city != nullptr, "CityTurnHandler pick_prod got nullptr city");
    GAME_EXPECT(state.m_statics != nullptr, "CityTurnHandler pick_prod got nullptr statics");
    GAME_EXPECT(state.m_player_states != nullptr, "CityTurnHandler pick_prod got nullptr player states");

    const u16 player = city->get_owner();
    GAME_EXPECT(player < state.m_player_n, "CityTurnHandler pick_prod player out of bounds");
    
    PlayerState& ps = state.m_player_states[player];
    BitArrayCL civ(state.m_statics->civ().get_item_count());
    if (ps.m_civ_index < civ.get_count()) {
        civ.set_bit(ps.m_civ_index);
    }
    BitArrayCL* techs = ps.m_techs_researched;
    if (SettlerTurnHandler::need_settler(state, player)) {
        BitArrayCL* units = city->get_trainable_units(city_idx, techs, &civ);
        const u16 settler = find_settler_typ(state, units);
        if (settler != U16_KEY_NULL) {
            city->build_unit(settler);
            return;
        }
    }
    BitArrayCL* blds = city->get_buildable_buildings(city_idx, techs, &civ);
    if (blds != nullptr) {
        const u32 bn = blds->get_count();
        for (u32 i = 0; i < bn; ++i) {
            if (blds->get_bit(i) != 0 && city->has_building(city_idx, static_cast<u16>(i))) {
                blds->clear_bit(i);
            }
        }
        u16 pick = U16_KEY_NULL;
        if (LinearBld::pick(*blds, &pick)) {
            city->build_building(pick);
            return;
        }
    }
    city->accumulate_commerce();
}

//================================================================================================================================
//=> - CityTurnHandler -
//================================================================================================================================

void CityTurnHandler::handle (GameState& state, u16 city_idx) {
    City* city = state.m_cities.get_city(city_idx);
    GAME_EXPECT(city != nullptr, "CityTurnHandler got nullptr city");

    const u16 player = city->get_owner();
    const TotalTileYield yld = CityTileManager::gather_yields(player, city_idx);
    const u16 food = static_cast<u16>(yld.m_food > 65535u ? 65535u : yld.m_food);
    const u16 production = static_cast<u16>(yld.m_production > 65535u ? 65535u : yld.m_production);
    const u16 commerce = static_cast<u16>(yld.m_commerce > 65535u ? 65535u : yld.m_commerce);
    i16 pop_change = city->add_food(city_idx, food);
    if (pop_change > 0) {
        for (i16 p = 0; p < pop_change; ++p) {
            CityTileManager::add_new_food_tile(player, city_idx);
        }
    } else if (pop_change < 0) {
        CityTileManager::maximize_food(player, city_idx);
    }

    city->add_commerce(city_idx, commerce);
    city->add_culture(city_idx, 0);
    city->add_production(city_idx, production);
    city->finish_if_ready(city_idx);
    if (city->need_prod_pick()) {
        pick_prod(state, city_idx, city);
    }

    GAME_EXPECT(state.m_player_states != nullptr, "CityTurnHandler got nullptr player states");
    GAME_EXPECT(player < state.m_player_n, "CityTurnHandler player out of bounds");
    PlayerState& ps = state.m_player_states[player];
    ps.m_this_turn_city_count = static_cast<u16>(ps.m_this_turn_city_count + 1u);
    ps.m_this_turn_population_count = ps.m_this_turn_population_count + static_cast<u32>(city->get_current_population());
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
