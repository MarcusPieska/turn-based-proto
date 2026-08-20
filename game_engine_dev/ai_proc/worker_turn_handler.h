//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORKER_TURN_HANDLER_H
#define WORKER_TURN_HANDLER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - WorkerTurnHandler -
//================================================================================================================================
//
//  Per-worker unit step. Tallies into m_last_turn_worker_count, then applies one WorkerGuidance job on a
//  home-city worked tile. PlayerState::m_worker_tile_opt_scan picks best vs first hit;
//  m_worker_tile_opt_reassign re-runs stable_food_max_production after a successful job.
//
//================================================================================================================================

class WorkerTurnHandler {
public:
    typedef void (*JobNoteFn) (u16 x, u16 y, u16 job, u8 intent);

    WorkerTurnHandler () = delete;

    static void handle (GameState& state, u16 unit_idx);
    static void set_job_note (JobNoteFn fn);

private:
    static JobNoteFn m_job_note; // Optional sink for each successful apply
};

#endif // WORKER_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
