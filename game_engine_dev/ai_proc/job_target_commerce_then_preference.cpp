//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "job_target_commerce_then_preference.h"

#include "dyn_job_slot_register.h"
#include "dyn_job_yield_register.h"
#include "job_target_preference_only.h"
#include "log_dbg.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 clamp_u16_i32 (i32 v) {
    if (v <= 0) {
        return 0;
    }
    if (v > static_cast<i32>(0xFFFFu)) {
        return 0xFFFFu;
    }
    return static_cast<u16>(v);
}

//================================================================================================================================
//=> - JobTarget_CommerceThenPreference -
//================================================================================================================================

u16 JobTarget_CommerceThenPreference::fill (
    u16 pop_limit,
    u16 trait_idx,
    DynJobYieldRegister& yld,
    const DynJobSlotRegister& slots,
    const EffectCtx& ctx) {
    yld.reset_remain();
    const u16 com_jobs = yld.fill(DynJobYield::COMMERCE, pop_limit, slots, ctx);
    const DynJobYieldPack& after_com = yld.pack();
    LOG_CITY_JOB_COMMERCE::LOG(com_jobs, clamp_u16_i32(after_com.m_commerce));
    
    JobTarget_PreferenceOnly::fill_from_remain(pop_limit, trait_idx, yld, slots, ctx);
    const DynJobYieldPack& p = yld.pack();
    LOG_CITY_JOB_YIELDS::LOG(
        p.m_n,
        p.m_food,
        p.m_production,
        p.m_commerce,
        p.m_culture,
        p.m_science,
        p.m_religion);
    ASSERT_CITY_JOB_ARE_COVERED::ASSERT(p.m_n <= pop_limit);
    return p.m_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
