//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef AI_UNIT_PRODUCTION_AGGRESSIVE_H
#define AI_UNIT_PRODUCTION_AGGRESSIVE_H

#include "game_primitives.h"

class City;
class GameState;

//================================================================================================================================
//=> - AiUnitProductionAggressive -
//================================================================================================================================
//
//  Aggressive land builds: upgrade/flag one local city defender from free support, then balance
//  def/atk/brg with each role capped at floor(free_land_unit_support * k_land_army_sup_fac / 3).
//
//================================================================================================================================

class AiUnitProductionAggressive {
public:
    AiUnitProductionAggressive () = delete;

    static bool try_pick_land_unit (GameState& state, u16 city_idx, City* city);
};

#endif // AI_UNIT_PRODUCTION_AGGRESSIVE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
