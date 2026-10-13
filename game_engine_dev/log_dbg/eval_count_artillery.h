//================================================================================================================================
//=> - WARNING -
//================================================================================================================================
//
//  - First-create stub from gen_log_dbg.py (TEMPLATE_eval.h).
//  - Regen will NOT overwrite this file once it exists; edit freely.
//
//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EVAL_COUNT_ARTILLERY_H
#define EVAL_COUNT_ARTILLERY_H

#include "game_primitives.h"
#include "log_dbg_toggles.h"

class GameState;

//================================================================================================================================
//=> - EVAL_COUNT_ARTILLERY -
//================================================================================================================================

#if defined(LOG_DBG_SO_BUILD) || defined(LOG_DBG_FORCE_ALL) || (defined(LOG_DBG_ENABLE) && defined(ENABLED_EVAL_COUNT_ARTILLERY))

class EVAL_COUNT_ARTILLERY {
public:
    static void EVAL (GameState& state, u16 player);

private:
    EVAL_COUNT_ARTILLERY () = delete;
};

#else

class EVAL_COUNT_ARTILLERY {
public:
    static void EVAL (GameState& state, u16 player) {
        (void)state;
        (void)player;
    }

private:
    EVAL_COUNT_ARTILLERY () = delete;
};

#endif

#endif // EVAL_COUNT_ARTILLERY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
