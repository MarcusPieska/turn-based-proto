//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_turn_handler_industrious.h"

#include "assert_log.h"
#include "bit_array.h"
#include "building_trait_orderings.h"
#include "city.h"
#include "city_tile_manager.h"
#include "city_turn_handler_core.h"
#include "civ_trait_enum.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "settler_turn_handler.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void pick_prod (GameState& state, u16 city_idx, City* city) {
    GAME_EXPECT(city != nullptr, "CityTurnHandler_Industrious pick_prod got nullptr city");
    GAME_EXPECT(state.m_statics != nullptr, "CityTurnHandler_Industrious pick_prod got nullptr statics");
    GAME_EXPECT(state.m_player_states != nullptr, "CityTurnHandler_Industrious pick_prod got nullptr player states");

    const u16 player = city->get_owner();
    GAME_EXPECT(player < state.m_player_n, "CityTurnHandler_Industrious pick_prod player out of bounds");

    PlayerState& ps = state.m_player_states[player];
    BitArrayCL civ(state.m_statics->civ().get_item_count());
    if (ps.m_civ_index < civ.get_count()) {
        civ.set_bit(ps.m_civ_index);
    }
    BitArrayCL* techs = ps.m_techs_researched;
    if (SettlerTurnHandler::need_settler(state, player)) {
        BitArrayCL* units = city->get_trainable_units(city_idx, techs, &civ);
        const u16 settler = CityTurnHandler_Core::find_settler_typ(state, units);
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
        const u16 pick = BuildingTraitOrderings::pick(*blds, static_cast<u16>(CivTrait::Industrious));
        if (pick != U16_KEY_NULL) {
            city->build_building(pick);
            return;
        }
    }
    city->accumulate_commerce();
}

//================================================================================================================================
//=> - CityTurnHandler_Industrious -
//================================================================================================================================

void CityTurnHandler_Industrious::handle (CityTurnHandlerCtx& ctx) {
    GAME_EXPECT(ctx.m_state != nullptr, "CityTurnHandler_Industrious got nullptr state");
    GAME_EXPECT(ctx.m_city != nullptr, "CityTurnHandler_Industrious got nullptr city");

    GameState& state = *ctx.m_state;
    City* city = ctx.m_city;
    const u16 city_idx = ctx.m_city_idx;
    const u16 player = ctx.m_player;
    const u16 sanitation_boost = ctx.m_sanitation_boost;
    const i16 net_sanitation = ctx.m_net_sanitation;
    const i16 pop_change = ctx.m_pop_change;

    if (pop_change != 0) {
        const u16 start_food = TileYields::get(city->get_x(), city->get_y()).m_food;
        if (net_sanitation - pop_change > 0) {
            CityTileManager::maximize_food(player, city_idx);
        } else {
            CityTileManager::stable_food_max_production(player, city_idx, start_food, sanitation_boost);
        }
    }
    if (city->need_prod_pick()) {
        pick_prod(state, city_idx, city);
        const u16 start_food = TileYields::get(city->get_x(), city->get_y()).m_food;
        if (net_sanitation - pop_change > 0) {
            CityTileManager::maximize_food(player, city_idx);
        } else {
            CityTileManager::stable_food_max_production(player, city_idx, start_food, sanitation_boost);
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
