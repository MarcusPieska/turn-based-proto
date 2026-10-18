//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>

#include "booster_register_tester_shared.h"
#include "city_job_static_key.h"
#include "city_job_trait_orderings.h"
#include "civ_trait_enum.h"
#include "dyn_job_slot_register.h"
#include "dyn_job_yield_register.h"
#include "job_assigner.h"
#include "job_count_array.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"

typedef std::chrono::high_resolution_clock Clock;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u32 k_reps = 2000u;
static const u16 k_trait_n = 7;

static f64 ns_between (Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<f64, std::nano>(b - a).count();
}

static cstr trait_name (u16 ti) {
    static const cstr k_nm[k_trait_n] = {
        "Agricultural", "Industrious", "Expansionist", "Religious",
        "Militaristic", "Scientific", "Commercial"
    };
    return ti < k_trait_n ? k_nm[ti] : "?";
}

static cstr job_name (const RuntimeStatics& st, u16 job_id) {
    if (job_id < st.city_job().get_item_count()) {
        return st.city_job().get_name(CityJobStaticDataKey::from_raw(job_id));
    }
    return "?";
}

static void enable_all (BoosterRegisterToggleEnv& env) {
    const RuntimeStatics& st = *env.m_st;
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        env.m_array.get_bld_bank()->set_flag(env.m_city_idx, i);
    }
    for (u16 i = 0; i < st.tech().get_item_count(); ++i) {
        env.m_techs->set_bit(i);
    }
    for (u16 i = 0; i < st.wonder().get_item_count(); ++i) {
        env.m_wonder_city[i] = env.m_city_idx;
    }
    for (u16 i = 0; i < st.small_wonder().get_item_count(); ++i) {
        env.m_sw_city[i] = env.m_city_idx;
    }
}

static bool check_caps (u16 city_idx, const EffectCtx& ctx, const RuntimeStatics& st) {
    const DynJobYieldRow* rows = st.dyn_job_yield().rows();
    const u16 jn = JobCountArray::job_n();
    for (u16 j = 0; j < jn; ++j) {
        const u8 n = JobCountArray::get(city_idx, j);
        const u8 mx = JobCountArray::max_n(j);
        const u16 cap = st.dyn_job_slot().capacity(j, rows[j].m_slots, ctx);
        if (n > mx) {
            std::printf("*** FAILED over max job %u have %u max %u\n",
                SC_U32(j), SC_U32(n), SC_U32(mx));
            return false;
        }
        if (n > cap) {
            std::printf("*** FAILED over cap job %u have %u cap %u\n",
                SC_U32(j), SC_U32(n), SC_U32(cap));
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    RuntimeStaticLoader loader;
    if (!loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        std::printf("*** FAILED statics load\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    if (!JobCountArray::setup(st)) {
        std::printf("*** FAILED JobCountArray::setup\n");
        return 1;
    }
    if (!CityJobTraitOrderings::begin(st.city_job(), st.trait_affinity_map())) {
        std::printf("*** FAILED CityJobTraitOrderings::begin\n");
        JobCountArray::clear();
        return 1;
    }
    BoosterRegisterToggleEnv env;
    if (!env.bind(st)) {
        std::printf("*** FAILED toggle env bind\n");
        CityJobTraitOrderings::clear();
        JobCountArray::clear();
        return 1;
    }
    env.clear_all();
    enable_all(env);
    const EffectCtx ctx = env.make_ctx();
    const u16 city_idx = env.m_city_idx;
    bool ok = true;

    for (u16 ti = 0; ti < k_trait_n && ok; ++ti) {
        JobAssigner::clear(city_idx);
        std::printf("=== fill order trait=%s (all enablers on) ===\n", trait_name(ti));
        u16 step = 0;
        for (;;) {
            const u16 job_id = JobAssigner::assign_one(city_idx, ti, ctx, st);
            if (job_id == U16_KEY_NULL) {
                break;
            }
            const u8 n = JobCountArray::get(city_idx, job_id);
            std::printf("  %2u. %s -> count %u (max %u)\n",
                SC_U32(step), job_name(st, job_id), SC_U32(n),
                SC_U32(JobCountArray::max_n(job_id)));
            ++step;
            if (!check_caps(city_idx, ctx, st)) {
                ok = false;
                break;
            }
            if (step > 512) {
                std::printf("*** FAILED runaway assign_one\n");
                ok = false;
                break;
            }
        }
        std::printf("  total seats %u\n", SC_U32(step));
        if (step == 0) {
            std::printf("*** FAILED no seats for trait %s\n", trait_name(ti));
            ok = false;
        }
    }

    const u16 trait = static_cast<u16>(CivTrait::Commercial);
    f64 one_min = 1e300;
    f64 one_max = 0.0;
    f64 one_sum = 0.0;
    f64 fill_min = 1e300;
    f64 fill_max = 0.0;
    f64 fill_sum = 0.0;
    volatile u32 sink = 0;
    for (u32 r = 0; r < k_reps; ++r) {
        JobAssigner::clear(city_idx);
        const Clock::time_point a0 = Clock::now();
        const u16 j0 = JobAssigner::assign_one(city_idx, trait, ctx, st);
        const Clock::time_point a1 = Clock::now();
        sink += j0;
        const f64 one_ns = ns_between(a0, a1);
        if (one_ns < one_min) one_min = one_ns;
        if (one_ns > one_max) one_max = one_ns;
        one_sum += one_ns;

        const Clock::time_point f0 = Clock::now();
        const u16 n = JobAssigner::fill(city_idx, trait, 64, ctx, st);
        const Clock::time_point f1 = Clock::now();
        sink += n;
        const f64 fill_ns = ns_between(f0, f1);
        if (fill_ns < fill_min) fill_min = fill_ns;
        if (fill_ns > fill_max) fill_max = fill_ns;
        fill_sum += fill_ns;
    }
    std::printf("=== assign timing (Commercial, all enablers, reps %u, sink %u) ===\n",
        k_reps, sink);
    std::printf("assign_one ns/call: min %.2f  max %.2f  avg %.2f\n",
        one_min, one_max, one_sum / k_reps);
    std::printf("fill(64) ns/call:   min %.2f  max %.2f  avg %.2f\n",
        fill_min, fill_max, fill_sum / k_reps);

    if (ok && !check_caps(city_idx, ctx, st)) {
        ok = false;
    }
    if (ok) {
        std::printf("*** PASSED job_assigner\n");
    }
    CityJobTraitOrderings::clear();
    JobCountArray::clear();
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
