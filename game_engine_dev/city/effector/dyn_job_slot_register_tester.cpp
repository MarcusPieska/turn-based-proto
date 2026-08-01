//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "booster_register_tester_shared.h"
#include "building_static_key.h"
#include "city_job_static_key.h"
#include "dyn_job_slot_register.h"
#include "item_effect_helpers.h"
#include "small_wonder_static_key.h"
#include "tech_static_key.h"
#include "wonder_static_key.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static RuntimeStaticLoader g_rt_loader;
static int g_fails = 0;
static u64 g_look_ns = 0;
static u32 g_look_n = 0;

static void note_fail (cstr msg) {
    ++g_fails;
    std::printf("FAIL: %s\n", msg);
}

static u16 timed_cap (const DynJobSlotRegister& reg, u16 job_id, u16 base, const EffectCtx& ctx,
    u64& buck_ns, u32& buck_n) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    const u16 cap = reg.capacity(job_id, base, ctx);
    const auto t1 = std::chrono::high_resolution_clock::now();
    const u64 dt = static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    g_look_ns += dt;
    ++g_look_n;
    buck_ns += dt;
    ++buck_n;
    return cap;
}

struct DynJobLine {
    char m_txt[160];
};

static void push_line (DynJobLine*& rows, u32& n, u32& cap, cstr fmt, ...) {
    if (n == cap) {
        const u32 next = (cap == 0) ? 8u : (cap * 2u);
        DynJobLine* neu = new DynJobLine[next];
        for (u32 i = 0; i < n; ++i) {
            neu[i] = rows[i];
        }
        delete[] rows;
        rows = neu;
        cap = next;
    }
    std::va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(rows[n].m_txt, sizeof(rows[n].m_txt), fmt, ap);
    va_end(ap);
    ++n;
}

static cstr kind_label (EffectEnablerKind k) {
    switch (k) {
    case EffectEnablerKind::BUILDING: return "building";
    case EffectEnablerKind::TECH: return "tech";
    case EffectEnablerKind::SMALL_WONDER: return "small_wonder";
    case EffectEnablerKind::WONDER: return "wonder";
    default: return "?";
    }
}

static cstr host_name (const RuntimeStatics& st, const EffectEnabler& en) {
    switch (en.m_kind) {
    case EffectEnablerKind::BUILDING:
        if (en.m_idx < st.building().get_item_count()) {
            return st.building().get_name(BuildingStaticDataKey::from_raw(en.m_idx));
        }
        break;
    case EffectEnablerKind::TECH:
        if (en.m_idx < st.tech().get_item_count()) {
            return st.tech().get_name(TechStaticDataKey::from_raw(en.m_idx));
        }
        break;
    case EffectEnablerKind::SMALL_WONDER:
        if (en.m_idx < st.small_wonder().get_item_count()) {
            return st.small_wonder().get_name(SmallWonderStaticDataKey::from_raw(en.m_idx));
        }
        break;
    case EffectEnablerKind::WONDER:
        if (en.m_idx < st.wonder().get_item_count()) {
            return st.wonder().get_name(WonderStaticDataKey::from_raw(en.m_idx));
        }
        break;
    default:
        break;
    }
    return nullptr;
}

static cstr job_name (const RuntimeStatics& st, u16 job_id) {
    if (job_id < st.city_job().get_item_count()) {
        return st.city_job().get_name(CityJobStaticDataKey::from_raw(job_id));
    }
    return "?";
}

static bool job_selected (cstr filter, cstr nm) {
    if (filter == nullptr || filter[0] == '\0') {
        return true;
    }
    return std::strcmp(filter, nm) == 0;
}

static void activate (BoosterRegisterToggleEnv& env, const EffectEnabler& en) {
    switch (en.m_kind) {
    case EffectEnablerKind::BUILDING:
        env.m_array.get_bld_bank()->set_flag(env.m_city_idx, en.m_idx);
        break;
    case EffectEnablerKind::TECH:
        env.m_techs->set_bit(en.m_idx);
        break;
    case EffectEnablerKind::SMALL_WONDER:
        env.m_sw_city[en.m_idx] = env.m_city_idx;
        break;
    case EffectEnablerKind::WONDER:
        env.m_wonder_city[en.m_idx] = env.m_city_idx;
        break;
    default:
        break;
    }
}

static i16 expected_delta (const DynJobSlotEntry& e, u16 base) {
    if (e.m_mode == ItemEffectAmountMode::PERCENTAGE) {
        return static_cast<i16>((static_cast<i32>(base) * static_cast<i32>(e.m_amount)) / 100);
    }
    return e.m_amount;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    cstr filter = (argc > 1) ? argv[1] : nullptr;
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        std::printf("statics failed\n");
        return 1;
    }
    RuntimeStatics& st = g_rt_loader.statics();
    const DynJobSlotRegister& reg = st.dyn_job_slot();
    const u16 job_n = st.city_job().get_item_count();
    std::printf("dyn_job_slot_register entries=%u jobs=%u\n",
        static_cast<u32>(reg.entry_count()),
        static_cast<u32>(reg.job_count()));

    if (reg.job_count() != job_n) {
        note_fail("job_n mismatch vs city_job statics");
        return 1;
    }

    const DynJobSlotEntry* rows = reg.entries();
    const u16 entry_n = reg.entry_count();
    for (u16 i = 0; i < entry_n; ++i) {
        const DynJobSlotEntry& e = rows[i];
        if (e.m_job_id >= job_n) {
            note_fail("entry job_id out of range");
            continue;
        }
        if (host_name(st, e.m_enabler) == nullptr) {
            note_fail("entry enabler does not resolve");
            continue;
        }
        if (e.m_mode != ItemEffectAmountMode::COUNT && e.m_mode != ItemEffectAmountMode::PERCENTAGE) {
            note_fail("entry has invalid amount mode");
        }
    }

    BoosterRegisterToggleEnv env;
    if (!env.bind(st)) {
        note_fail("toggle env bind");
        return 1;
    }

    u32 jobs_checked = 0;
    u32 entries_toggled = 0;
    for (u16 job_id = 0; job_id < job_n; ++job_id) {
        cstr jnm = job_name(st, job_id);
        if (!job_selected(filter, jnm)) {
            continue;
        }
        ++jobs_checked;
        const u16 base = st.city_job().get_item(CityJobStaticDataKey::from_raw(job_id)).slots;
        DynJobLine* lines = nullptr;
        u32 line_n = 0;
        u32 line_cap = 0;
        u64 buck_ns = 0;
        u32 buck_n = 0;

        env.clear_all();
        EffectCtx ctx = env.make_ctx();
        const u16 empty_cap = timed_cap(reg, job_id, base, ctx, buck_ns, buck_n);
        if (empty_cap != base) {
            push_line(lines, line_n, line_cap, "  FAIL: empty capacity=%u want base=%u",
                static_cast<u32>(empty_cap), static_cast<u32>(base));
            ++g_fails;
        }

        u32 job_entries = 0;
        for (u16 i = 0; i < entry_n; ++i) {
            const DynJobSlotEntry& e = rows[i];
            if (e.m_job_id != job_id) {
                continue;
            }
            ++job_entries;
            env.clear_all();
            activate(env, e.m_enabler);
            ctx = env.make_ctx();
            const u16 after = timed_cap(reg, job_id, base, ctx, buck_ns, buck_n);
            const i16 delta = expected_delta(e, base);
            const i32 expect = static_cast<i32>(base) + static_cast<i32>(delta);
            const u16 expect_u = (expect <= 0) ? 0 : static_cast<u16>(expect > 65535 ? 65535 : expect);
            const cstr mode = (e.m_mode == ItemEffectAmountMode::PERCENTAGE) ? "PERCENTAGE" : "COUNT";
            push_line(lines, line_n, line_cap, "  %s %s  +%d (%s)  -> %u",
                kind_label(e.m_enabler.m_kind),
                host_name(st, e.m_enabler),
                static_cast<int>(delta),
                mode,
                static_cast<u32>(after));
            if (after != expect_u) {
                push_line(lines, line_n, line_cap, "  FAIL: capacity got %u want %u",
                    static_cast<u32>(after), static_cast<u32>(expect_u));
                ++g_fails;
            }
            ++entries_toggled;
        }
        if (job_entries == 0) {
            push_line(lines, line_n, line_cap, "  (no jobSlots sources)");
        }

        const u64 avg_job = (buck_n == 0) ? 0 : (buck_ns / static_cast<u64>(buck_n));
        std::printf("%s  base=%u (%llu ns)\n", jnm, static_cast<u32>(base), static_cast<unsigned long long>(avg_job));
        for (u32 i = 0; i < line_n; ++i) {
            std::printf("%s\n", lines[i].m_txt);
        }
        delete[] lines;
    }

    if (jobs_checked == 0) {
        note_fail("no jobs matched filter");
    }
    const u64 avg_ns = (g_look_n == 0) ? 0 : (g_look_ns / static_cast<u64>(g_look_n));
    std::printf("checked jobs=%u toggled_entries=%u fails=%d\n", jobs_checked, entries_toggled, g_fails);
    std::printf("avg lookup_ns=%llu\n", static_cast<unsigned long long>(avg_ns));
    return g_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
