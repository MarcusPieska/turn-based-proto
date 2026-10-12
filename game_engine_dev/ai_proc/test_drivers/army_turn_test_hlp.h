//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ARMY_TURN_TEST_HLP_H
#define ARMY_TURN_TEST_HLP_H

#include "army_turn_handler.h"
#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - ArmyTurnTestHlp -
//================================================================================================================================
//
//  Test-only friend of ArmyTurnHandler: exposes private step helpers without WarTurnHandler coupling.
//
//================================================================================================================================

class ArmyTurnTestHlp {
public:
    ArmyTurnTestHlp () = delete;

    static bool walk (GameState& s, WarArmy& army);
    static WarAssault assault (GameState& s, u16 seat, WarArmy& army, u16 city_x, u16 city_y);
    static bool rejoin_move (GameState& s, u16 seat, u16 enemy, WarArmy& army);
    static bool rejoin_link (GameState& s, WarArmy& army);
    static u16 rest_heal (GameState& s, WarArmy& army);
    static bool can_fight (const GameState& s, const WarArmy& army);
    static bool can_cont (const GameState& s, const WarArmy& army);
};

#endif // ARMY_TURN_TEST_HLP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
