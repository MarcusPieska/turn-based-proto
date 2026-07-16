//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_turn_handler.h"

#include "city.h"
#include "city_tile_manager.h"
#include "game_state.h"

//================================================================================================================================
//=> - CityTurnHandler -
//================================================================================================================================

void CityTurnHandler::handle (GameState& state, u16 city_idx) {
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return;
    }
    const u16 player = city->get_owner();
    const TotalTileYield yld = CityTileManager::gather_yields(player, city_idx);
    const u16 food = static_cast<u16>(yld.m_food > 65535u ? 65535u : yld.m_food);
    const u16 production = static_cast<u16>(yld.m_production > 65535u ? 65535u : yld.m_production);
    const u16 commerce = static_cast<u16>(yld.m_commerce > 65535u ? 65535u : yld.m_commerce);
    const u16 pop0 = city->get_current_population();
    if (city->add_food(city_idx, food)) {
        const u16 pop1 = city->get_current_population();
        for (u16 p = pop0; p < pop1; ++p) {
            CityTileManager::add_new_food_tile(player, city_idx);
        }
    }
    if (city->add_production(city_idx, production)) {
        city->finish_if_ready(city_idx);
    }
    city->add_commerce(city_idx, commerce);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
