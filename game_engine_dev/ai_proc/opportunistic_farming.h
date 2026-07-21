//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef OPPORTUNISTIC_FARMING_H
#define OPPORTUNISTIC_FARMING_H

#include "game_primitives.h" 

class GameState;

//================================================================================================================================
//=> - OpportunisticFarming -
//================================================================================================================================
//
//  Local worker AI: plant farms on adjacent plains without global search or city need scoring.
//  Prefers unfarmed river plains; on-river cruises up to 4 steps/turn along river farming as it goes.
//  Falls back to river-adjacent plains, then any 8-adj farmable plains, then cheb-2/3/4 approach steps.
//  Tiles must be civ-owned and in work reach. Farms are BUILD_ADD_STD bits via StdAddHelper.
//  No heap allocations.
//
//================================================================================================================================

class OpportunisticFarming {
public:
    OpportunisticFarming () = delete;

    static bool begin (GameState& state);
    static void clear ();
    static void handle (GameState& state, u16 unit_idx);
};

#endif // OPPORTUNISTIC_FARMING_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
