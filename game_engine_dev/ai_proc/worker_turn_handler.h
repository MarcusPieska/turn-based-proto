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
//  Per-worker unit step. Tallies into m_last_turn_worker_count, applies one WorkerGuidance job on a
//  home-city worked tile via WorkerBuildProgress (mp deficit freeze). Each worker keeps a work tile until
//  it is fully upgraded; only then scan (resource overlay, pick_first, or pick_best when disk is done).
//  When GameState::m_path_worker != 0, steps toward the job tile before apply; else applies remotely.
//
//================================================================================================================================

class WorkerTurnHandler {
public:
    typedef void (*JobNoteFn) (u16 x, u16 y, u16 job, u16 imp, u8 intent);

    WorkerTurnHandler () = delete;

    static void handle (GameState& state, u16 unit_idx);
    static void set_job_note (JobNoteFn fn);
    static void clear_work_tgt (GameState& state, u16 unit_idx);

private:
    static JobNoteFn m_job_note; // Optional sink for each successful apply
};

#endif // WORKER_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
