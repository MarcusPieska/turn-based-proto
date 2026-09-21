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

#ifndef EVAL_NAVY_UNIT_SUPPORT_H
#define EVAL_NAVY_UNIT_SUPPORT_H

#include "game_primitives.h"
#include "log_dbg_toggles.h"

class GameState;

//================================================================================================================================
//= EVAL_NAVY_UNIT_SUPPORT =
//================================================================================================================================

#if defined(LOG_DBG_SO_BUILD) || defined(LOG_DBG_FORCE_ALL) || (defined(LOG_DBG_ENABLE) && defined(ENABLED_EVAL_NAVY_UNIT_SUPPORT))

class EVAL_NAVY_UNIT_SUPPORT {
public:
    static void EVAL (GameState& state);

private:
    EVAL_NAVY_UNIT_SUPPORT () = delete;
};

#else

class EVAL_NAVY_UNIT_SUPPORT {
public:
    static void EVAL (GameState& state) {
        (void)state;
    }

private:
    EVAL_NAVY_UNIT_SUPPORT () = delete;
};

#endif

#endif // EVAL_NAVY_UNIT_SUPPORT_H

//================================================================================================================================
//= End of file =
//================================================================================================================================
