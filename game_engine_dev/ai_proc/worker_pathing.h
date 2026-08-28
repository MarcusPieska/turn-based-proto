//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORKER_PATHING_H
#define WORKER_PATHING_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - WorkerPathing -
//================================================================================================================================
//
//  Worker-only step pathing: mountains enterable; mtn-to-mtn only when from or to tile has a built road.
//  Uses UnitMovementMng::can_step_worker / apply_step_worker; does not touch shared can_step.
//
//================================================================================================================================

class WorkerPathing {
public:
    WorkerPathing () = delete;

    static bool step_toward (GameState& state, u16 unit_idx, u16 tx, u16 ty);

private:
    WorkerPathing (const WorkerPathing& other) = delete;
    WorkerPathing (WorkerPathing&& other) = delete;
};

#endif // WORKER_PATHING_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
