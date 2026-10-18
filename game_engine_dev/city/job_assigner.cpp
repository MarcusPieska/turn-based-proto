//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "job_assigner.h"

#include "city_job_trait_orderings.h"
#include "dyn_job_slot_register.h"
#include "dyn_job_yield_register.h"
#include "effect_ctx.h"
#include "job_count_array.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u16 s_room (u16 city_idx, u16 job_id, const EffectCtx& ctx, const RuntimeStatics& st) {
    if (!JobCountArray::ready() || job_id >= JobCountArray::job_n()) {
        return 0;
    }
    const DynJobYieldRegister& yld = st.dyn_job_yield();
    const DynJobYieldRow* rows = yld.rows();
    if (rows == nullptr || job_id >= yld.job_count()) {
        return 0;
    }
    const u16 cap = st.dyn_job_slot().capacity(job_id, rows[job_id].m_slots, ctx);
    const u8 have = JobCountArray::get(city_idx, job_id);
    const u8 mx = JobCountArray::max_n(job_id);
    if (have >= mx || have >= cap) {
        return 0;
    }
    u16 room = SC_U16(cap - have);
    const u16 lim = SC_U16(mx - have);
    if (room > lim) {
        room = lim;
    }
    return room;
}

//================================================================================================================================
//=> - JobAssigner -
//================================================================================================================================

void JobAssigner::clear (u16 city_idx) {
    if (!JobCountArray::ready()) {
        return;
    }
    const u16 jn = JobCountArray::job_n();
    for (u16 j = 0; j < jn; ++j) {
        JobCountArray::set(city_idx, j, 0);
    }
}

u16 JobAssigner::assign_one (u16 city_idx, u16 trait_idx, const EffectCtx& ctx, const RuntimeStatics& st) {
    if (!JobCountArray::ready() || !CityJobTraitOrderings::ready()) {
        return U16_KEY_NULL;
    }
    const u16 ord_n = CityJobTraitOrderings::job_n();
    for (u16 s = 0; s < ord_n; ++s) {
        const u16 job_id = CityJobTraitOrderings::at(trait_idx, s);
        if (s_room(city_idx, job_id, ctx, st) == 0) {
            continue;
        }
        const u8 next = SC_U8(JobCountArray::get(city_idx, job_id) + 1u);
        JobCountArray::set(city_idx, job_id, next);
        return job_id;
    }
    return U16_KEY_NULL;
}

u16 JobAssigner::fill (u16 city_idx, u16 trait_idx, u16 pop_limit, const EffectCtx& ctx, const RuntimeStatics& st) {
    clear(city_idx);
    u16 n = 0;
    while (n < pop_limit) {
        if (assign_one(city_idx, trait_idx, ctx, st) == U16_KEY_NULL) {
            break;
        }
        ++n;
    }
    return n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
