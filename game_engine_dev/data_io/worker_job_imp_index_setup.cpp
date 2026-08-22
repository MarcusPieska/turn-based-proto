//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "worker_job_imp_index_setup.h"

#include "runtime_statics.h"
#include "worker_job_imp_static_key.h"

//================================================================================================================================
//=> - WorkerJobImpIndexSetup -
//================================================================================================================================

bool WorkerJobImpIndexSetup::build (const RuntimeStatics& st, WorkerJobImpIndex& out) {
    out.clear();
    const u16 ov_n = st.map_overlay().get_item_count();
    const u16 imp_n = st.worker_job_imp().get_item_count();
    if (ov_n == 0u) {
        return false;
    }
    u16* counts = new u16[ov_n]();
    u16 total = 0;
    for (u16 i = 0; i < imp_n; ++i) {
        const u16 ov = st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(i)).map_overlay_idx;
        if (ov >= ov_n) {
            delete[] counts;
            return false;
        }
        counts[ov]++;
        total++;
    }
    out.m_ov_n = ov_n;
    out.m_imp_n = total;
    out.m_off = new u16[ov_n + 1u];
    out.m_off[0] = 0;
    for (u16 j = 0; j < ov_n; ++j) {
        out.m_off[j + 1u] = static_cast<u16>(out.m_off[j] + counts[j]);
    }
    out.m_idx = (total > 0u) ? new u16[total] : nullptr;
    u16* curs = new u16[ov_n];
    for (u16 j = 0; j < ov_n; ++j) {
        curs[j] = out.m_off[j];
    }
    for (u16 i = 0; i < imp_n; ++i) {
        const u16 ov = st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(i)).map_overlay_idx;
        out.m_idx[curs[ov]++] = i;
    }
    delete[] curs;
    delete[] counts;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
