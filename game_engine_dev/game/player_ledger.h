//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_LEDGER_H
#define PLAYER_LEDGER_H

#include "game_primitives.h"

class GameState;
class PlayerState;
class RuntimeStatics;

//================================================================================================================================
//=> - PlayerLedger -
//================================================================================================================================
//
//  Static façade over per-seat commerce and research counters in GameState::m_player_states.
//  add_commerce banks into m_commerce_from_turn; GameLoop partitions that into treasury/research after cities.
//  bind_state wires the active match; callers grant yields without holding a GameState pointer.
//  bind_seats is a light bind for drivers that only need seats + statics.
//
//================================================================================================================================

class PlayerLedger {
public:
    PlayerLedger () = delete;

    static void bind_state (GameState* state);
    static void bind_seats (PlayerState* seats, u16 n, const RuntimeStatics* statics);
    static bool add_commerce (u16 player, u16 amount);
    static bool add_research (u16 player, u16 amount);
    static u32 commerce (u16 player);
    static bool spend_commerce (u16 player, u32 amount);
    static bool upgrade_rates (u16* out_prod, u16* out_stat);

private:
    PlayerLedger (const PlayerLedger& other) = delete;
    PlayerLedger (PlayerLedger&& other) = delete;
};

#endif // PLAYER_LEDGER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
