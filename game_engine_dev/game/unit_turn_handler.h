//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_TURN_HANDLER_H
#define UNIT_TURN_HANDLER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - UnitTurnHandler -
//================================================================================================================================
//
//  Generic per-unit end-of-turn work shared by all unit kinds. Heals a head and its group tails
//  when they still hold a full movement budget. Heal % comes from config UNIT_HEAL_IN_CITY /
//  UNIT_HEAL_DEFAULT applied to UNIT_HEALTH. Call on map heads only (valid m_x/m_y); tails
//  inherit the head tile for the city check.
//
//================================================================================================================================

class UnitTurnHandler {
public:
    UnitTurnHandler () = delete;

    static void handle (GameState& state, u16 unit_idx);

private:
    static bool has_full_mp (const GameState& state, const struct UnitAddStruct& u);
    static bool tile_is_city (const GameState& state, u16 x, u16 y);
    static void heal_one (GameState& state, u16 unit_idx, u16 tile_x, u16 tile_y);
};

#endif // UNIT_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
