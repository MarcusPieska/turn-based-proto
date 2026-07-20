//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SETTLER_TURN_HANDLER_H
#define SETTLER_TURN_HANDLER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - SettlerTurnHandler -
//================================================================================================================================
//
//  AI settler lifecycle for one match. m_target_settlements is the desired settler count (0 = off). Each turn
//  refresh_targets fills PlayerState settle cache when empty, then sets target to SETTLER_MISSION_SLOTS if sites else 2.
//  try_assign consumes that cache; points drop when their mission ends. GameLoop zeros unit counts after cities then
//  calls handle per settler; handle tallies into m_last_turn_settler_count. Mission slots = SETTLER_MISSION_SLOTS.
//
//================================================================================================================================

class SettlerTurnHandler {
public:
    SettlerTurnHandler () = delete;

    static bool begin (GameState& state);
    static void clear ();

    static void refresh_targets (GameState& state);
    static bool need_settler (GameState& state, u16 player);
    static void handle (GameState& state, u16 unit_idx);
    static bool tgt_xy (u16 player, u16 slot, u16* x, u16* y);
};

#endif // SETTLER_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
