//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "booster_register_tester_shared.h"
#include "building_static_key.h"
#include "city_job_static_key.h"
#include "dyn_job_yield_register.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

typedef const char* cstr;

static int g_fails = 0;
static double g_look_ns = 0.0;
static u32 g_look_n = 0;
static double g_fill_ns = 0.0;
static u32 g_fill_n = 0;

static void note_fail (cstr msg) {
    ++g_fails;
    std::printf("FAIL: %s\n", msg);
}

static double elapsed_ns (std::chrono::high_resolution_clock::time_point t0,
    std::chrono::high_resolution_clock::time_point t1) {
    return std::chrono::duration<double, std::nano>(t1 - t0).count();
}

static cstr yield_name (DynJobYield y) {
    switch (y) {
    case DynJobYield::FOOD: return "food";
    case DynJobYield::PRODUCTION: return "production";
    case DynJobYield::COMMERCE: return "commerce";
    case DynJobYield::CULTURE: return "culture";
    case DynJobYield::SCIENCE: return "science";
    case DynJobYield::RELIGION: return "religion";
    default: return "?";
    }
}

static cstr job_name (const RuntimeStatics& st, u16 job_id) {
    if (job_id < st.city_job().get_item_count()) {
        return st.city_job().get_name(CityJobStaticDataKey::from_raw(job_id));
    }
    return "?";
}

static bool enable_bld (BoosterRegisterToggleEnv& env, cstr want) {
    const RuntimeStatics& st = *env.m_st;
    const u16 n = st.building().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = st.building().get_name(BuildingStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, want) == 0) {
            env.m_array.get_bld_bank()->set_flag(env.m_city_idx, i);
            return true;
        }
    }
    return false;
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
    DynJobYieldRegister& reg = st.dyn_job_yield();
    const u16 job_n = st.city_job().get_item_count();
    std::printf("dyn_job_yield_register entries=%u jobs=%u\n",
        static_cast<u32>(reg.entry_count()),
        static_cast<u32>(reg.job_count()));
    if (reg.job_count() != job_n) {
        note_fail("job_n mismatch vs city_job statics");
        return 1;
    }
    if (reg.remain() == nullptr) {
        note_fail("remain scratch missing");
        return 1;
    }

    const DynJobYieldEntry* rows = reg.entries();
    for (u16 yi = 0; yi < DynJobYieldRegister::YIELD_N; ++yi) {
        const DynJobYield y = static_cast<DynJobYield>(yi);
        const auto t0 = std::chrono::high_resolution_clock::now();
        const u16 begin = reg.group_begin(y);
        const u16 end = reg.group_end(y);
        const auto t1 = std::chrono::high_resolution_clock::now();
        const double dt = elapsed_ns(t0, t1);
        g_look_ns += dt;
        ++g_look_n;
        std::printf("%s  n=%u (%.2f ns)\n", yield_name(y), static_cast<u32>(end - begin), dt);
        i16 prev = 32767;
        for (u16 i = begin; i < end; ++i) {
            const DynJobYieldEntry& e = rows[i];
            std::printf("  %s  score=%d\n", job_name(st, e.m_job_id), static_cast<int>(e.m_score));
            if (e.m_score > prev) {
                note_fail("group not sorted descending by score");
            }
            if (e.m_score <= 0) {
                note_fail("non-positive score in group");
            }
            prev = e.m_score;
        }
    }

    BoosterRegisterToggleEnv env;
    if (!env.bind(st)) {
        note_fail("toggle env bind");
        return 1;
    }
    env.clear_all();
    if (!enable_bld(env, "Marketplace")) {
        note_fail("Marketplace building not found");
        return 1;
    }
    EffectCtx ctx = env.make_ctx();

    reg.reset_remain();
    for (u16 i = 0; i < job_n; ++i) {
        if (reg.remain()[i] != U16_KEY_NULL) {
            note_fail("reset_remain did not set U16_KEY_NULL");
            break;
        }
    }
    if (reg.pack().m_n != 0 || reg.pack().m_commerce != 0) {
        note_fail("reset_remain did not clear pack");
    }

    DynJobYieldPack& pack = reg.reset_remain();
    const auto f0 = std::chrono::high_resolution_clock::now();
    const u16 added = reg.fill(DynJobYield::COMMERCE, 5, st.dyn_job_slot(), ctx);
    const auto f1 = std::chrono::high_resolution_clock::now();
    const double fill_dt = elapsed_ns(f0, f1);
    g_fill_ns += fill_dt;
    ++g_fill_n;
    std::printf("fill commerce pop_limit=5 marketplace -> added=%u n=%u food=%d prod=%d com=%d cult=%d sci=%d rel=%d (%.2f ns)\n",
        static_cast<u32>(added),
        static_cast<u32>(pack.m_n),
        pack.m_food, pack.m_production, pack.m_commerce,
        pack.m_culture, pack.m_science, pack.m_religion,
        fill_dt);
    if (added == 0 || pack.m_commerce <= 0) {
        note_fail("commerce fill expected positive commerce");
    }
    if (pack.m_n > 5) {
        note_fail("fill exceeded pop_limit");
    }

    const u16 added2 = reg.fill(DynJobYield::PRODUCTION, 5, st.dyn_job_slot(), ctx);
    std::printf("fill production same pack -> added=%u n=%u prod=%d com=%d\n",
        static_cast<u32>(added2),
        static_cast<u32>(pack.m_n),
        pack.m_production, pack.m_commerce);
    if (pack.m_n > 5) {
        note_fail("cumulative fill exceeded pop_limit");
    }
    if (added2 > 0 && pack.m_n != static_cast<u16>(added + added2)) {
        note_fail("pack m_n not cumulative");
    }

    u32 touched = 0;
    u32 untouched = 0;
    for (u16 i = 0; i < job_n; ++i) {
        if (reg.remain()[i] == U16_KEY_NULL) {
            ++untouched;
        } else {
            ++touched;
        }
    }
    std::printf("jit remain touched=%u untouched=%u\n", touched, untouched);
    if (touched == 0) {
        note_fail("expected JIT capacity lookups");
    }
    if (untouched == 0) {
        note_fail("expected some jobs never looked up");
    }

    const double avg_look = (g_look_n == 0) ? 0.0 : (g_look_ns / static_cast<double>(g_look_n));
    const double avg_fill = (g_fill_n == 0) ? 0.0 : (g_fill_ns / static_cast<double>(g_fill_n));
    std::printf("avg group_lookup_ns=%.2f  avg fill_ns=%.2f\n", avg_look, avg_fill);
    std::printf("fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
