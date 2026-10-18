//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "general_move_boost.h"
#include "job_count_array.h"
#include "mvt_pts_boost_mng.h"
#include "unit_roster_mng.h"
#include "worker_helper.h"

//================================================================================================================================
//=> - City link stubs (dyn_booster_register_comp only) -
//================================================================================================================================

bool JobCountArray::ready () {
    return false;
}

u16 JobCountArray::claim () {
    return 0;
}

u8 JobCountArray::get (u16, u16) {
    return 0;
}

void MvtPtsBoostMng::fill (GameState&, UnitAddStruct*) {
}

void GeneralMoveBoost::on_unit_refilled (GameState&, u16) {
}

u16 UnitRosterMng::get_best_unit_of_type (u16, const AssessorCtx&) {
    return U16_KEY_NULL;
}

void WorkerHelper::set_data (UnitAddStruct*, u16) {
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
