//================================================================================================================================
//=> - WARNING -
//================================================================================================================================
//
//  - First-create stub from gen_log_dbg.py (TEMPLATE_eval.cpp).
//  - Regen will NOT overwrite this file once it exists; implement EVAL here.
//  - Real body always compiled into log_dbg.so (LOG_DBG_SO_BUILD).
//
//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#define LOG_DBG_SO_BUILD
#include "eval_city_tile_work_count.h"

#include "trace_sink.h"
#include "assert_log.h"

//================================================================================================================================
//=> - EVAL_CITY_TILE_WORK_COUNT -
//================================================================================================================================

void EVAL_CITY_TILE_WORK_COUNT::EVAL (const GameArraySimple& map, u16 city_idx, u16 pop) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 worked = 0u;
    for (u16 y = 0u; y < h; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            if (map.get_city_worker(x, y) == city_idx) {
                ++worked;
            }
        }
    }
    TraceSink::printf("City pop / tiles worked: %u / %u\n", static_cast<u32>(pop), static_cast<u32>(worked));
    GAME_EXPECT(worked <= static_cast<u32>(pop), "City pop / tiles worked");
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
