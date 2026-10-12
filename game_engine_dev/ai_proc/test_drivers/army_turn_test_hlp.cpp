//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "army_turn_test_hlp.h"

//================================================================================================================================
//=> - ArmyTurnTestHlp -
//================================================================================================================================

bool ArmyTurnTestHlp::walk (GameState& s, WarArmy& army) {
    return ArmyTurnHandler::walk(s, army);
}

WarAssault ArmyTurnTestHlp::assault (GameState& s, u16 seat, WarArmy& army, u16 city_x, u16 city_y) {
    return ArmyTurnHandler::assault(s, seat, army, city_x, city_y);
}

bool ArmyTurnTestHlp::rejoin_move (GameState& s, u16 seat, u16 enemy, WarArmy& army) {
    return ArmyTurnHandler::rejoin_move(s, seat, enemy, army);
}

bool ArmyTurnTestHlp::rejoin_link (GameState& s, WarArmy& army) {
    return ArmyTurnHandler::rejoin_link(s, army);
}

u16 ArmyTurnTestHlp::rest_heal (GameState& s, WarArmy& army) {
    return ArmyTurnHandler::rest_heal(s, army);
}

bool ArmyTurnTestHlp::can_fight (const GameState& s, const WarArmy& army) {
    return ArmyTurnHandler::can_fight(s, army);
}

bool ArmyTurnTestHlp::can_cont (const GameState& s, const WarArmy& army) {
    return ArmyTurnHandler::can_cont(s, army);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
