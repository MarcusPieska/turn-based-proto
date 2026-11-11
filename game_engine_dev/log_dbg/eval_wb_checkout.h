//================================================================================================================================
//= WARNING =
//================================================================================================================================
//=
//=  - First-create stub from gen_log_dbg.py (TEMPLATE_eval.h).
//=  - Regen will NOT overwrite this file once it exists; edit freely.
//=
//================================================================================================================================
//= Include guards =
//================================================================================================================================

#ifndef EVAL_WB_CHECKOUT_H
#define EVAL_WB_CHECKOUT_H

#include "game_primitives.h"
#include "log_dbg_toggles.h"

//================================================================================================================================
//= EVAL_WB_CHECKOUT =
//================================================================================================================================
//=
//=  End-of-turn live whiteboard inventory: wb inv tag=begin, item lines, tag=end.
//=  Needs WB_TRACE_DB on the whiteboard_mng translation unit for a non-empty list.
//=
//================================================================================================================================

#if defined(LOG_DBG_SO_BUILD) || defined(LOG_DBG_FORCE_ALL) || (defined(LOG_DBG_ENABLE) && defined(ENABLED_EVAL_WB_CHECKOUT))

class EVAL_WB_CHECKOUT {
public:
    static void EVAL (unsigned turn);

private:
    EVAL_WB_CHECKOUT () = delete;
};

#else

class EVAL_WB_CHECKOUT {
public:
    static void EVAL (unsigned turn) {
        (void)turn;
    }

private:
    EVAL_WB_CHECKOUT () = delete;
};

#endif

#endif // EVAL_WB_CHECKOUT_H

//================================================================================================================================
//= End of file =
//================================================================================================================================
