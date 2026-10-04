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

#ifndef EVAL_ARMY_STATE_TO_WAR_H
#define EVAL_ARMY_STATE_TO_WAR_H

#include "game_primitives.h"
#include "log_dbg_toggles.h"

class GameState;

//================================================================================================================================
//=> - EVAL_ARMY_STATE_TO_WAR -
//================================================================================================================================
//
//  Dumps one army linked list after form: army state=toWar, unit lines, army state=end.
//
//================================================================================================================================

#if defined(LOG_DBG_SO_BUILD) || defined(LOG_DBG_FORCE_ALL) || (defined(LOG_DBG_ENABLE) && defined(ENABLED_EVAL_ARMY_STATE_TO_WAR))

class EVAL_ARMY_STATE_TO_WAR {
public:
    static void EVAL (GameState& state, u16 head);

private:
    EVAL_ARMY_STATE_TO_WAR () = delete;
};

#else

class EVAL_ARMY_STATE_TO_WAR {
public:
    static void EVAL (GameState& state, u16 head) {
        (void)state;
        (void)head;
    }

private:
    EVAL_ARMY_STATE_TO_WAR () = delete;
};

#endif

#endif // EVAL_ARMY_STATE_TO_WAR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
