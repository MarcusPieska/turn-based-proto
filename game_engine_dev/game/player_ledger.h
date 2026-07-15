//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_LEDGER_H
#define PLAYER_LEDGER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - PlayerLedger -
//================================================================================================================================
//
//  Static façade over per-seat commerce and research counters in GameState::m_player_states.
//  bind_state wires the active match; callers grant yields without holding a GameState pointer.
//
//================================================================================================================================

class PlayerLedger {
public:
    PlayerLedger () = delete;

    static void bind_state (GameState* state);
    static bool add_commerce (u16 player, u16 amount);
    static bool add_research (u16 player, u16 amount);

private:
    PlayerLedger (const PlayerLedger& other) = delete;
    PlayerLedger (PlayerLedger&& other) = delete;
};

#endif // PLAYER_LEDGER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
