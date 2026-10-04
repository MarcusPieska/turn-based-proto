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

#ifndef EVAL_ARMY_STATE_PEACE_H
#define EVAL_ARMY_STATE_PEACE_H

#include "game_primitives.h"
#include "log_dbg_toggles.h"

class GameState;

//================================================================================================================================
//=> - EVAL_ARMY_STATE_PEACE -
//================================================================================================================================
//
//  Dumps one army linked list at peace/demobilize: army state=peace, unit lines, army state=end.
//
//================================================================================================================================

#if defined(LOG_DBG_SO_BUILD) || defined(LOG_DBG_FORCE_ALL) || (defined(LOG_DBG_ENABLE) && defined(ENABLED_EVAL_ARMY_STATE_PEACE))

class EVAL_ARMY_STATE_PEACE {
public:
    static void EVAL (GameState& state, u16 head);

private:
    EVAL_ARMY_STATE_PEACE () = delete;
};

#else

class EVAL_ARMY_STATE_PEACE {
public:
    static void EVAL (GameState& state, u16 head) {
        (void)state;
        (void)head;
    }

private:
    EVAL_ARMY_STATE_PEACE () = delete;
};

#endif

#endif // EVAL_ARMY_STATE_PEACE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
