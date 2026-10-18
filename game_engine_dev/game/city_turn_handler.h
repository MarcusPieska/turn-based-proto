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
//  Per-city turn step invoked by the main game loop. Gathers worked-tile yields, realizes JobCountArray seats via
//  JobEffectRealizer (JobAssigner fills leftovers when empty; refills after pop_change tile rematrix), then banks
//  tile+job yields before finish_if_ready / growth / trait AI. Tallies this-turn city and population counts.
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
