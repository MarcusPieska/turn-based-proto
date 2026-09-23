//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_TURN_HANDLER_CORE_H
#define CITY_TURN_HANDLER_CORE_H

#include "game_primitives.h"

class BitArrayCL;
class City;
class GameState;

//================================================================================================================================
//=> - CityTurnHandler_Core -
//================================================================================================================================
//
//  Shared helpers for trait city AI strategies.
//
//================================================================================================================================

class CityTurnHandler_Core {
public:
    CityTurnHandler_Core () = delete;

    static u16 find_settler_typ (const GameState& state, const BitArrayCL* units);
    static u16 find_worker_typ (const GameState& state, const BitArrayCL* units);
    static u16 own_land_sup_on_tile (GameState& state, u16 player, u16 x, u16 y);
    static bool need_worker (const GameState& state, u16 player);
    static bool try_pick_worker (GameState& state, u16 city_idx, City* city);
    static bool try_pick_land_unit (GameState& state, u16 city_idx, City* city);
    static void try_assess_imps (GameState& state, u16 city_idx, City* city);
};

#endif // CITY_TURN_HANDLER_CORE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
