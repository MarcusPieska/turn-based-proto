//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "booster_register_tester_shared.h"
#include "city_job_enum.h"
#include "city_job_static_key.h"
#include "city_job_trait_orderings.h"
#include "civ_trait_enum.h"
#include "dyn_job_slot_register.h"
#include "dyn_job_yield_register.h"
#include "job_target_commerce_then_preference.h"
#include "job_target_preference_only.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

typedef const char* cstr;

static int g_fails = 0;

static void note_fail (cstr msg) {
    ++g_fails;
    std::printf("FAIL: %s\n", msg);
}

static void note_ok (bool cond, cstr msg) {
    if (!cond) {
        note_fail(msg);
    }
}

static void enable_all_bld (BoosterRegisterToggleEnv& env) {
    const RuntimeStatics& st = *env.m_st;
    const u16 n = st.building().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        env.m_array.get_bld_bank()->set_flag(env.m_city_idx, i);
    }
}

static cstr job_name (const RuntimeStatics& st, u16 job_id) {
    if (job_id < st.city_job().get_item_count()) {
        return st.city_job().get_name(CityJobStaticDataKey::from_raw(job_id));
    }
    return "?";
}

static void print_taken (
    const RuntimeStatics& st,
    const DynJobYieldRegister& yld,
    const DynJobSlotRegister& slots,
    const EffectCtx& ctx) {
    const u16* rem = yld.remain();
    const DynJobYieldRow* rows = yld.rows();
    const u16 job_n = yld.job_count();
    std::printf("  taken:\n");
    for (u16 j = 0; j < job_n; ++j) {
        if (rem[j] == U16_KEY_NULL) {
            continue;
        }
        const u16 cap = slots.capacity(j, rows[j].m_slots, ctx);
        const u16 taken = static_cast<u16>(cap - rem[j]);
        if (taken == 0) {
            continue;
        }
        std::printf("    %s taken=%u cap=%u\n",
            job_name(st, j), static_cast<u32>(taken), static_cast<u32>(cap));
    }
    const DynJobYieldPack& p = yld.pack();
    std::printf("  pack n=%u food=%d prod=%d com=%d cult=%d sci=%d rel=%d\n",
        static_cast<u32>(p.m_n), p.m_food, p.m_production, p.m_commerce,
        p.m_culture, p.m_science, p.m_religion);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    RuntimeStaticLoader loader;
    if (!loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        std::printf("statics failed\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    note_ok(CityJobTraitOrderings::begin(st.city_job(), st.trait_affinity_map()), "CityJobTraitOrderings::begin");
    note_ok(CityJobTraitOrderings::ready(), "CityJobTraitOrderings::ready");

    BoosterRegisterToggleEnv env;
    if (!env.bind(st)) {
        note_fail("toggle env bind");
        return 1;
    }
    env.clear_all();
    enable_all_bld(env);
    note_ok(st.building().get_item_count() > 0, "buildings available");
    EffectCtx ctx = env.make_ctx();

    DynJobYieldRegister& yld = st.dyn_job_yield();
    const DynJobSlotRegister& slots = st.dyn_job_slot();
    const u16 trait = static_cast<u16>(CivTrait::Commercial);
    const u16 pop = 5;

    std::printf("\n--- JobTarget_CommerceThenPreference pop=%u trait=Commercial ---\n", static_cast<u32>(pop));
    const u16 n_com = JobTarget_CommerceThenPreference::fill(pop, trait, yld, slots, ctx);
    std::printf("assigned=%u\n", static_cast<u32>(n_com));
    print_taken(st, yld, slots, ctx);
    note_ok(n_com > 0 && n_com <= pop, "commerce-then assigned in (0,pop]");
    note_ok(yld.pack().m_commerce > 0, "commerce-then pack has commerce");
    note_ok(yld.pack().m_n == n_com, "commerce-then return matches pack.m_n");

    std::printf("\n--- JobTarget_PreferenceOnly pop=%u trait=Commercial ---\n", static_cast<u32>(pop));
    const u16 n_pref = JobTarget_PreferenceOnly::fill(pop, trait, yld, slots, ctx);
    std::printf("assigned=%u\n", static_cast<u32>(n_pref));
    print_taken(st, yld, slots, ctx);
    note_ok(n_pref > 0 && n_pref <= pop, "preference-only assigned in (0,pop]");
    note_ok(yld.pack().m_n == n_pref, "preference-only return matches pack.m_n");

    const u16 merchant = static_cast<u16>(CityJob::Merchant);
    const u16* rem = yld.remain();
    note_ok(rem[merchant] != U16_KEY_NULL, "preference-only touched Merchant for Commercial");

    CityJobTraitOrderings::clear();
    std::printf("\n=======================================================\n");
    std::printf(" TESTING JOB TARGET: TOTAL FAILURES: %d\n", g_fails);
    std::printf("=======================================================\n");
    return g_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
