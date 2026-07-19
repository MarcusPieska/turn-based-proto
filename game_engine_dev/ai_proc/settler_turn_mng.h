//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SETTLER_TURN_MNG_H
#define SETTLER_TURN_MNG_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - SettlerTurnMng -
//================================================================================================================================
//
//  AI settler lifecycle for one match. m_target_settlements is the desired settler count (0 = off). Each turn
//  refresh_targets sets it to SETTLER_MISSION_SLOTS while SenseSettlingPtsOpt returns sites, else 2. GameLoop calls
//  begin_unit_pass then handle per settler; handle tallies into m_last_turn_settler_count. Mission slots = SETTLER_MISSION_SLOTS.
//
//================================================================================================================================

class SettlerTurnMng {
public:
    SettlerTurnMng () = delete;

    static bool begin (GameState& state);
    static void clear ();

    static void refresh_targets (GameState& state);
    static void begin_unit_pass (GameState& state);
    static bool need_settler (GameState& state, u16 player);
    static void handle (GameState& state, u16 unit_idx);
    static bool tgt_xy (u16 player, u16 slot, u16* x, u16* y);
};

#endif // SETTLER_TURN_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
