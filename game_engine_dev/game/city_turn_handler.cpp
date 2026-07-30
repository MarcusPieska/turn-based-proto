//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_turn_handler.h"

#include "assert_log.h"
#include "city.h"
#include "city_tile_manager.h"
#include "city_turn_handler_agricultural.h"
#include "city_turn_handler_commercial.h"
#include "city_turn_handler_ctx.h"
#include "city_turn_handler_default.h"
#include "city_turn_handler_expansionist.h"
#include "city_turn_handler_industrious.h"
#include "city_turn_handler_militaristic.h"
#include "city_turn_handler_religious.h"
#include "city_turn_handler_scientific.h"
#include "civ_static_key.h"
#include "civ_trait_enum.h"
#include "game_state.h"
#include "runtime_statics.h"

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
    const u16 sanitation_boost = city->get_city_sanitation_boost(city_idx);
    const i16 net_sanitation = city->get_city_net_sanitation(sanitation_boost);

    city->add_commerce(city_idx, commerce);
    city->add_culture(city_idx, 0);
    city->add_production(city_idx, production);
    city->finish_if_ready(city_idx);
    i16 pop_change = city->add_food(city_idx, food, net_sanitation);

    CityTurnHandlerCtx ctx;
    ctx.m_state = &state;
    ctx.m_city = city;
    ctx.m_city_idx = city_idx;
    ctx.m_player = player;
    ctx.m_sanitation_boost = sanitation_boost;
    ctx.m_net_sanitation = net_sanitation;
    ctx.m_pop_change = pop_change;

    GAME_EXPECT(state.m_player_states != nullptr, "CityTurnHandler got nullptr player states");
    GAME_EXPECT(player < state.m_player_n, "CityTurnHandler player out of bounds");
    GAME_EXPECT(state.m_statics != nullptr, "CityTurnHandler got nullptr statics");
    PlayerState& ps = state.m_player_states[player];
    CivTrait trait = CivTrait::Agricultural;
    if (ps.m_civ_index < state.m_statics->civ().get_item_count()) {
        const CivStaticDataStruct& civ = state.m_statics->civ().get_item(CivStaticDataKey::from_raw(ps.m_civ_index));
        trait = static_cast<CivTrait>(civ.traits.indices[0]);
    }
    switch (trait) {
    case CivTrait::Agricultural:
        CityTurnHandler_Agricultural::handle(ctx);
        break;
    case CivTrait::Industrious:
        CityTurnHandler_Industrious::handle(ctx);
        break;
    case CivTrait::Expansionist:
        CityTurnHandler_Expansionist::handle(ctx);
        break;
    case CivTrait::Religious:
        CityTurnHandler_Religious::handle(ctx);
        break;
    case CivTrait::Militaristic:
        CityTurnHandler_Militaristic::handle(ctx);
        break;
    case CivTrait::Scientific:
        CityTurnHandler_Scientific::handle(ctx);
        break;
    case CivTrait::Commercial:
        CityTurnHandler_Commercial::handle(ctx);
        break;
    default:
        CityTurnHandler_Default::handle(ctx);
        break;
    }

    ps.m_this_turn_city_count = static_cast<u16>(ps.m_this_turn_city_count + 1u);
    ps.m_this_turn_population_count = ps.m_this_turn_population_count + static_cast<u32>(city->get_current_population());
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
