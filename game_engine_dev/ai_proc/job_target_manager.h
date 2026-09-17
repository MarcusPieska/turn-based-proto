//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef JOB_TARGET_MANAGER_H
#define JOB_TARGET_MANAGER_H

#include "dyn_job_yield_register.h"
#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - JobTargetManager -
//================================================================================================================================
//
//  After tile work (or a skipped assign), counts worked tiles, takes leftover pops = pop - worked, and
//  fills city jobs: PreferenceOnly normally, CommerceThenPreference when the seat is lucky.
//  Returns DynJobYieldPack (empty when no leftover pops).
//
//================================================================================================================================

class JobTargetManager {
public:
    JobTargetManager () = delete;

    static DynJobYieldPack fill (GameState& state, u16 city_idx, u16 trait_idx);
};

#endif // JOB_TARGET_MANAGER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
