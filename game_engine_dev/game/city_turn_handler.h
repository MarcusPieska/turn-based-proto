//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_TURN_HANDLER_H
#define CITY_TURN_HANDLER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - CityTurnHandler -
//================================================================================================================================
//
//  Per-city turn step invoked by the main game loop. Gathers worked-tile yields, fills leftover pops into city jobs,
//  then banks tile+job yields (commerce/science to PlayerLedger; food/production/culture to City; religion ignored)
//  before finish_if_ready / growth / trait AI. Tallies m_this_turn_city_count and m_this_turn_population_count.
//
//================================================================================================================================

class CityTurnHandler {
public:
    CityTurnHandler () = delete;

    static void handle (GameState& state, u16 city_idx);
};

#endif // CITY_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
