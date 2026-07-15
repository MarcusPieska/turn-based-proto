//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_loop.h"

#include "game_state.h"
#include "runtime_trace_dbg.h"

//================================================================================================================================
//=> - GameLoop -
//================================================================================================================================

GameLoop::GameLoop () {
}

GameLoop::~GameLoop () {
}

bool GameLoop::run (GameState* state, cstr trace_path) {
    if (state == nullptr || trace_path == nullptr) {
        return false;
    }
    if (state->m_turn_limit == 0 || state->m_current_turn >= state->m_turn_limit) {
        return false;
    }
    TRACE_SETUP((trace_path));
    while (state->m_current_turn < state->m_turn_limit) {
        state->m_current_turn = state->m_current_turn + 1u;
        TRACE_NEW_TURN((static_cast<u16>(state->m_current_turn)));
    }
    return true; 
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
