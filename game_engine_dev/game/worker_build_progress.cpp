//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "worker_build_progress.h"

#include "assert_log.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "worker_job_imp_static_key.h"
#include "worker_job_static_key.h"

//================================================================================================================================
//=> - WorkerBuildProgress -
//================================================================================================================================

bool WorkerBuildProgress::can_start (const UnitAddStruct* u) {
    GAME_EXPECT(u != nullptr, "WorkerBuildProgress::can_start null unit");
    return u->m_mvt_points >= 0;
}

void WorkerBuildProgress::refill_mp (GameState& state, u16 unit_idx) {
    UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    GAME_EXPECT(u != nullptr, "WorkerBuildProgress::refill_mp null unit");
    GAME_EXPECT(state.m_statics != nullptr, "WorkerBuildProgress::refill_mp null statics");
    const u16 pts = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    const i32 budget = static_cast<i32>(pts) * static_cast<i32>(state.m_statics->config().get_mov_pt_per_turn());
    if (u->m_mvt_points < 0) {
        u->m_mvt_points = static_cast<i16>(static_cast<i32>(u->m_mvt_points) + budget);
    } else {
        u->m_mvt_points = static_cast<i16>(budget);
    }
    if (static_cast<i32>(u->m_mvt_points) > budget) {
        u->m_mvt_points = static_cast<i16>(budget);
    }
}

u32 WorkerBuildProgress::work_cost (const RuntimeStatics& st, u16 job_idx, u16 imp_idx) {
    if (imp_idx != U16_KEY_NULL) {
        if (imp_idx >= st.worker_job_imp().get_item_count()) {
            return 0u;
        }
        return st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(imp_idx)).cost;
    }
    if (job_idx >= st.worker_job().get_item_count()) {
        return 0u;
    }
    return st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(job_idx)).cost;
}

i16 WorkerBuildProgress::mvt_deficit (const RuntimeStatics& st, u16 build_perc, u32 cost) {
    (void)st;
    i64 d = static_cast<i64>(cost) * static_cast<i64>(build_perc);
    d = d / 100;
    if (d > 32767) {
        return 32767;
    }
    if (d < 0) {
        return 0;
    }
    return static_cast<i16>(d);
}

void WorkerBuildProgress::apply_deficit (UnitAddStruct* u, i16 deficit) {
    GAME_EXPECT(u != nullptr, "WorkerBuildProgress::apply_deficit null unit");
    const i32 next = static_cast<i32>(u->m_mvt_points) - static_cast<i32>(deficit);
    if (next < -32768) {
        u->m_mvt_points = -32768;
        return;
    }
    u->m_mvt_points = static_cast<i16>(next);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
