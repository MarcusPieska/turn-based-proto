//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORKER_BUILD_PROGRESS_H
#define WORKER_BUILD_PROGRESS_H

#include "game_primitives.h"

class GameState;
class RuntimeStatics;
struct UnitAddStruct;

//================================================================================================================================
//=> - WorkerBuildProgress -
//================================================================================================================================
//
//  Worker build cadence via movement-point deficit. Work applies instantly; m_mvt_points goes negative by
//  cost scaled by PlayerState::m_worker_mvt_to_build_perc as a percentage multiplier on build cost.
//
//================================================================================================================================

class WorkerBuildProgress {
public:
    static bool can_start (const UnitAddStruct* u);
    static void refill_mp (GameState& state, u16 unit_idx);

    static u32 work_cost (const RuntimeStatics& st, u16 job_idx, u16 imp_idx);
    static i16 mvt_deficit (const RuntimeStatics& st, u16 build_perc, u32 cost);
    static void apply_deficit (UnitAddStruct* u, i16 deficit);

private:
    WorkerBuildProgress () = delete;
};

#endif // WORKER_BUILD_PROGRESS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
