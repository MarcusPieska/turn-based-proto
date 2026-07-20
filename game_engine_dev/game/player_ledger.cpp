//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_ledger.h"
#include "game_state.h"
#include "assert_log.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

static GameState* s_state = nullptr;

//================================================================================================================================
//=> - PlayerLedger -
//================================================================================================================================

void PlayerLedger::bind_state (GameState* state) {
    s_state = state; 
}

bool PlayerLedger::add_commerce (u16 player, u16 amount) {
    GAME_EXPECT_RET(s_state != nullptr, false, "PlayerLedger state");
    GAME_EXPECT_RET(s_state->m_player_states != nullptr, false, "PlayerLedger player states");
    GAME_EXPECT_RET(player < s_state->m_player_n, false, "PlayerLedger commerce seat");
    s_state->m_player_states[player].m_commerce_from_turn += static_cast<u32>(amount);
    return true;
}

bool PlayerLedger::add_research (u16 player, u16 amount) {
    GAME_EXPECT_RET(s_state != nullptr, false, "PlayerLedger state");
    GAME_EXPECT_RET(s_state->m_player_states != nullptr, false, "PlayerLedger player states");
    GAME_EXPECT_RET(player < s_state->m_player_n, false, "PlayerLedger research seat");
    s_state->m_player_states[player].m_research += static_cast<u32>(amount);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
