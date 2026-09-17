//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "job_target_preference_only.h"

#include "city_job_trait_orderings.h"
#include "dyn_job_slot_register.h"
#include "dyn_job_yield_register.h"
#include "log_dbg.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void take_job (
    u16 job_id,
    u16 take,
    DynJobYieldRegister& yld) {
    if (take == 0 || job_id >= yld.job_count()) {
        return;
    }
    u16* rem = yld.remain();
    u16* taken = yld.taken();
    DynJobYieldPack& pack = yld.pack();
    const DynJobYieldRow& r = yld.rows()[job_id];
    rem[job_id] = static_cast<u16>(rem[job_id] - take);
    taken[job_id] = static_cast<u16>(taken[job_id] + take);
    pack.m_n = static_cast<u16>(pack.m_n + take);
    const i32 n = static_cast<i32>(take);
    pack.m_food += n * static_cast<i32>(r.m_food);
    pack.m_production += n * static_cast<i32>(r.m_production);
    pack.m_commerce += n * static_cast<i32>(r.m_commerce);
    pack.m_culture += n * static_cast<i32>(r.m_culture);
    pack.m_science += n * static_cast<i32>(r.m_science);
    pack.m_religion += n * static_cast<i32>(r.m_religion);
}

static void log_pack (const DynJobYieldPack& p) {
    LOG_CITY_JOB_YIELDS::LOG(
        p.m_n,
        p.m_food,
        p.m_production,
        p.m_commerce,
        p.m_culture,
        p.m_science,
        p.m_religion);
}

//================================================================================================================================
//=> - JobTarget_PreferenceOnly -
//================================================================================================================================

u16 JobTarget_PreferenceOnly::fill_from_remain (
    u16 pop_limit,
    u16 trait_idx,
    DynJobYieldRegister& yld,
    const DynJobSlotRegister& slots,
    const EffectCtx& ctx) {
    if (!CityJobTraitOrderings::ready() || yld.remain() == nullptr || yld.taken() == nullptr || yld.rows() == nullptr) {
        return 0;
    }
    if (pop_limit == 0 || yld.pack().m_n >= pop_limit) {
        return 0;
    }
    const u16 job_n = yld.job_count();
    const u16 ord_n = CityJobTraitOrderings::job_n();
    if (job_n == 0 || ord_n == 0) {
        return 0;
    }
    u16* rem = yld.remain();
    const DynJobYieldRow* rows = yld.rows();
    const u16 begin_n = yld.pack().m_n;
    u16 left = static_cast<u16>(pop_limit - begin_n);
    for (u16 s = 0; s < ord_n && left > 0; ++s) {
        const u16 job_id = CityJobTraitOrderings::at(trait_idx, s);
        if (job_id >= job_n) {
            continue;
        }
        if (rem[job_id] == U16_KEY_NULL) {
            rem[job_id] = slots.capacity(job_id, rows[job_id].m_slots, ctx);
        }
        u16 take = rem[job_id];
        if (take > left) {
            take = left;
        }
        if (take == 0) {
            continue;
        }
        take_job(job_id, take, yld);
        left = static_cast<u16>(left - take);
    }
    return static_cast<u16>(yld.pack().m_n - begin_n);
}

DynJobYieldPack JobTarget_PreferenceOnly::fill (
    u16 pop_limit,
    u16 trait_idx,
    DynJobYieldRegister& yld,
    const DynJobSlotRegister& slots,
    const EffectCtx& ctx) {
    yld.reset_remain();
    fill_from_remain(pop_limit, trait_idx, yld, slots, ctx);
    const DynJobYieldPack& p = yld.pack();
    log_pack(p);
    ASSERT_CITY_JOB_ARE_COVERED::ASSERT(p.m_n <= pop_limit);
    return p;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
