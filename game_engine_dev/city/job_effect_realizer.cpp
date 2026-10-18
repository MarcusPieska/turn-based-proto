//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "job_effect_realizer.h"

#include "job_count_array.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - JobEffectRealizer -
//================================================================================================================================

DynJobYieldPack JobEffectRealizer::realize (u16 city_idx, const RuntimeStatics& st) {
    DynJobYieldPack pack = {};
    if (!JobCountArray::ready()) {
        return pack;
    }
    const DynJobYieldRegister& yld = st.dyn_job_yield();
    const DynJobYieldRow* rows = yld.rows();
    const u16 jn = JobCountArray::job_n();
    if (rows == nullptr || jn == 0 || jn > yld.job_count()) {
        return pack;
    }
    for (u16 j = 0; j < jn; ++j) {
        const u8 n = JobCountArray::get(city_idx, j);
        if (n == 0) {
            continue;
        }
        const i32 k = SC_I32(n);
        const DynJobYieldRow& r = rows[j];
        pack.m_n = SC_U16(pack.m_n + n);
        pack.m_food += k * SC_I32(r.m_food);
        pack.m_production += k * SC_I32(r.m_production);
        pack.m_commerce += k * SC_I32(r.m_commerce);
        pack.m_culture += k * SC_I32(r.m_culture);
        pack.m_science += k * SC_I32(r.m_science);
        pack.m_religion += k * SC_I32(r.m_religion); 
    }
    return pack;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
