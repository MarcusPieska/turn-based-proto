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

#ifndef EVAL_ARMY_STATE_MUSTER_GROUP_H
#define EVAL_ARMY_STATE_MUSTER_GROUP_H

#include "game_primitives.h"
#include "log_dbg_toggles.h"

class GameState;

//================================================================================================================================
//=> - EVAL_ARMY_STATE_MUSTER_GROUP -
//================================================================================================================================
//
//  When muster walk stalls: army state=musterStg_sx_sy once, then per group
//  army state=musterGroup, unit lines, army state=end.
//
//================================================================================================================================

#if defined(LOG_DBG_SO_BUILD) || defined(LOG_DBG_FORCE_ALL) || (defined(LOG_DBG_ENABLE) && defined(ENABLED_EVAL_ARMY_STATE_MUSTER_GROUP))

class EVAL_ARMY_STATE_MUSTER_GROUP {
public:
    static void EVAL (GameState& state, u16 sx, u16 sy, const u16* heads, u16 n);

private:
    EVAL_ARMY_STATE_MUSTER_GROUP () = delete;
};

#else

class EVAL_ARMY_STATE_MUSTER_GROUP {
public:
    static void EVAL (GameState& state, u16 sx, u16 sy, const u16* heads, u16 n) {
        (void)state;
        (void)sx;
        (void)sy;
        (void)heads;
        (void)n;
    }

private:
    EVAL_ARMY_STATE_MUSTER_GROUP () = delete;
};

#endif

#endif // EVAL_ARMY_STATE_MUSTER_GROUP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
