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
//  AI settler lifecycle for one match; consumes GenAiHelpers settle order (does not regenerate city plan).
//  m_target_settlements is desired settler count (0 = off); refresh_targets punches once, then each turn sets
//  target to clamp(eligible planned sites still free/own-or-unowned, MIN 3, SETTLER_MISSION_SLOTS). Punch /
//  city-block restamp stays here for now; revisit moving it with GenAiHelpers. GameLoop zeros unit counts after
//  cities then calls handle per settler; handle tallies into m_last_turn_settler_count.
//
//================================================================================================================================

class SettlerTurnHandler {
public:
    SettlerTurnHandler () = delete;

    static bool begin (GameState& state);
    static void clear ();

    static void refresh_targets (GameState& state);
    static u16 elig_sites (GameState& state, u16 player);
    static bool need_settler (GameState& state, u16 player);
    static void handle (GameState& state, u16 unit_idx);
    static bool tgt_xy (u16 player, u16 slot, u16* x, u16* y);
};

#endif // SETTLER_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
