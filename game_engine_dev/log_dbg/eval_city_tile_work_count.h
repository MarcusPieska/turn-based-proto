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

#ifndef EVAL_CITY_TILE_WORK_COUNT_H
#define EVAL_CITY_TILE_WORK_COUNT_H

#include "game_array_simple.h"
#include "game_primitives.h"
#include "log_dbg_toggles.h"

//================================================================================================================================
//=> - EVAL_CITY_TILE_WORK_COUNT -
//================================================================================================================================

#if defined(LOG_DBG_SO_BUILD) || defined(LOG_DBG_FORCE_ALL) || (defined(LOG_DBG_ENABLE) && defined(ENABLED_EVAL_CITY_TILE_WORK_COUNT))

class EVAL_CITY_TILE_WORK_COUNT {
public:
    static void EVAL (const GameArraySimple& map, u16 city_idx, u16 pop);

private:
    EVAL_CITY_TILE_WORK_COUNT () = delete;
};

#else

class EVAL_CITY_TILE_WORK_COUNT {
public:
    static void EVAL (const GameArraySimple& map, u16 city_idx, u16 pop) {
        (void)map;
        (void)city_idx;
        (void)pop;
    }

private:
    EVAL_CITY_TILE_WORK_COUNT () = delete;
};

#endif

#endif // EVAL_CITY_TILE_WORK_COUNT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
