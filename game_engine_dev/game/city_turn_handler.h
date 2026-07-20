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
//  Per-city turn step invoked by the main game loop. Gathers worked-tile yields via CityTileManager, applies growth and
//  production finish, then if the queue is idle or in commerce mode picks next: settler if needed, else LinearBld, else
//  accumulate commerce. Banks commerce and culture (add_culture(0) pulls building yields; City::add_culture claims borders
//  on expand). Tallies m_this_turn_city_count and m_this_turn_population_count for the owning seat.
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
