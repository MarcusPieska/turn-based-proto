//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORKER_TURN_HANDLER_MK2_H
#define WORKER_TURN_HANDLER_MK2_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - WorkerTurnHandlerMk2 -
//================================================================================================================================
//
//  Mark-and-count worker AI. Cities keep m_tile_imp_count as the sum of missing jobs on the worked
//  disk (e.g. place+irr+mill = 3); tiles use m_tile_work_needed. CityTurnHandler reassesses when
//  count is 0 on a city_idx % 10 schedule. Workers stay tied to a base city (WorkerHelper data),
//  greedily path to marked tiles, and may spill to connected cities when the base is idle. Prefer
//  no stalls over optimal play. Optional CityConnector promote (k_conn) extends roads beyond the disc.
//
//================================================================================================================================

class WorkerTurnHandlerMk2 {
public:
    typedef void (*JobNoteFn) (u16 x, u16 y, u16 job, u16 imp, u8 intent);

    WorkerTurnHandlerMk2 () = delete;

    static void handle (GameState& state, u16 unit_idx);
    static u32 assess (GameState& state, u16 city_idx); // Raw job sum; city stores min(sum, 255)
    static void set_job_note (JobNoteFn fn);
    static void clear_work_tgt (GameState& state, u16 unit_idx);

private:
    static JobNoteFn m_job_note; // Optional sink for each successful apply
};

#endif // WORKER_TURN_HANDLER_MK2_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
