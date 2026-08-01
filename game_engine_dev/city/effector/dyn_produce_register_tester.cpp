//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "booster_register_tester_shared.h"
#include "building_static_key.h"
#include "dyn_produce_register.h"
#include "item_effect_helpers.h"
#include "produce_register.h"
#include "resource_static_key.h"
#include "runtime_static_loader.h"
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

static void timed_apply (const DynProduceRegister& reg, DynProduceRegister::Inventory& inv,
    DynProduceRegister::CityYields& yld, const EffectCtx& ctx) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    reg.apply(inv, yld, ctx);
    const auto t1 = std::chrono::high_resolution_clock::now();
    g_look_ns += static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    ++g_look_n;
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

static bool name_selected (cstr filter, cstr nm) {
    if (filter == nullptr || filter[0] == '\0') {
        return true;
    }
    return nm != nullptr && std::strcmp(filter, nm) == 0;
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

static void fill_inv (DynProduceRegister::Inventory& inv, i16 v) {
    for (u16 r = 0; r < inv.m_n; ++r) {
        inv.m_v[r] = v;
    }
}

static bool make_inv (DynProduceRegister::Inventory& inv, u16 n) {
    delete[] inv.m_v;
    inv.m_v = nullptr;
    inv.m_n = n;
    if (n == 0) {
        return true;
    }
    inv.m_v = new i16[n]();
    return inv.m_v != nullptr;
}

static void free_inv (DynProduceRegister::Inventory& inv) {
    delete[] inv.m_v;
    inv.m_v = nullptr;
    inv.m_n = 0;
}

static void print_by_group (const RuntimeStatics& st, const DynProduceRegister& reg, cstr filter) {
    const DynProduceGroup* rows = reg.groups();
    const u16 n = reg.group_count();
    std::printf("by group:\n");
    u32 shown = 0;
    for (u16 i = 0; i < n; ++i) {
        const DynProduceGroup& g = rows[i];
        cstr hnm = host_name(st, g.m_enabler);
        if (!name_selected(filter, hnm)) {
            continue;
        }
        ++shown;
        std::printf("%s %s  slots=%u\n",
            kind_label(g.m_enabler.m_kind),
            hnm != nullptr ? hnm : "?",
            static_cast<u32>(g.m_slot_n));
        for (u8 s = 0; s < g.m_slot_n; ++s) {
            const DynProduceSlot& sl = g.m_slots[s];
            if (sl.m_kind == ItemProduceKind::YIELD) {
                std::printf("  yield %s  %+d\n",
                    ItemEffectHelper::produce_yield_enum_to_str(static_cast<ItemProduceYield>(sl.m_target)),
                    static_cast<int>(sl.m_amount));
                continue;
            }
            cstr res_nm = "?";
            const u16 cat = reg.res_id(sl.m_target);
            if (cat < st.resource().get_item_count()) {
                res_nm = st.resource().get_name(ResourceStaticDataKey::from_raw(cat));
            }
            std::printf("  resource %s  %+d\n", res_nm, static_cast<int>(sl.m_amount));
        }
    }
    if (shown == 0) {
        std::printf("  (none)\n");
    }
}

static void print_by_resource (const RuntimeStatics& st, const DynProduceRegister& reg, cstr filter) {
    const DynProduceGroup* rows = reg.groups();
    const u16 n = reg.group_count();
    std::printf("by resource:\n");
    u32 shown = 0;
    for (u16 d = 0; d < reg.inv_n(); ++d) {
        const u16 cat = reg.res_id(d);
        cstr res_nm = "?";
        if (cat < st.resource().get_item_count()) {
            res_nm = st.resource().get_name(ResourceStaticDataKey::from_raw(cat));
        }
        if (!name_selected(filter, res_nm)) {
            continue;
        }
        ++shown;
        std::printf("%s\n", res_nm);
        u32 legs = 0;
        for (u16 i = 0; i < n; ++i) {
            const DynProduceGroup& g = rows[i];
            for (u8 s = 0; s < g.m_slot_n; ++s) {
                const DynProduceSlot& sl = g.m_slots[s];
                if (sl.m_kind != ItemProduceKind::RESOURCE || sl.m_target != d) {
                    continue;
                }
                ++legs;
                cstr hnm = host_name(st, g.m_enabler);
                std::printf("  %s %s  %+d\n",
                    kind_label(g.m_enabler.m_kind),
                    hnm != nullptr ? hnm : "?",
                    static_cast<int>(sl.m_amount));
            }
        }
        if (legs == 0) {
            std::printf("  (no producers)\n");
        }
    }
    if (shown == 0) {
        std::printf("  (none)\n");
    }
}

static void print_by_yield (const RuntimeStatics& st, const DynProduceRegister& reg, cstr filter) {
    const DynProduceGroup* rows = reg.groups();
    const u16 n = reg.group_count();
    u8 used[DynProduceRegister::YIELD_N] = {};
    for (u16 i = 0; i < n; ++i) {
        const DynProduceGroup& g = rows[i];
        for (u8 s = 0; s < g.m_slot_n; ++s) {
            const DynProduceSlot& sl = g.m_slots[s];
            if (sl.m_kind == ItemProduceKind::YIELD && sl.m_target > 0 && sl.m_target < DynProduceRegister::YIELD_N) {
                used[sl.m_target] = 1;
            }
        }
    }
    std::printf("by yield:\n");
    u32 shown = 0;
    for (u16 y = 1; y < DynProduceRegister::YIELD_N; ++y) {
        if (used[y] == 0) {
            continue;
        }
        cstr ynm = ItemEffectHelper::produce_yield_enum_to_str(static_cast<ItemProduceYield>(y));
        if (!name_selected(filter, ynm)) {
            continue;
        }
        ++shown;
        std::printf("%s\n", ynm);
        for (u16 i = 0; i < n; ++i) {
            const DynProduceGroup& g = rows[i];
            for (u8 s = 0; s < g.m_slot_n; ++s) {
                const DynProduceSlot& sl = g.m_slots[s];
                if (sl.m_kind != ItemProduceKind::YIELD || sl.m_target != y) {
                    continue;
                }
                cstr hnm = host_name(st, g.m_enabler);
                std::printf("  %s %s  %+d\n",
                    kind_label(g.m_enabler.m_kind),
                    hnm != nullptr ? hnm : "?",
                    static_cast<int>(sl.m_amount));
            }
        }
    }
    if (shown == 0) {
        std::printf("  (none)\n");
    }
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
    const DynProduceRegister& reg = st.dyn_produce();
    std::printf("dyn_produce_register groups=%u inv_n=%u (of %u resources) yield_n=%u\n",
        static_cast<u32>(reg.group_count()),
        static_cast<u32>(reg.inv_n()),
        static_cast<u32>(reg.res_n()),
        static_cast<u32>(DynProduceRegister::YIELD_N));
    if (reg.group_count() != ProduceRegister::group_count()) {
        std::printf("FAIL: group_n dyn=%u codegen=%u\n",
            static_cast<u32>(reg.group_count()),
            static_cast<u32>(ProduceRegister::group_count()));
        ++g_fails;
    }
    if (reg.inv_n() == 0 || reg.inv_n() > reg.res_n()) {
        note_fail("dense inv_n out of range");
    }

    const DynProduceGroup* rows = reg.groups();
    const u16 n = reg.group_count();
    for (u16 i = 0; i < n; ++i) {
        const DynProduceGroup& g = rows[i];
        if (host_name(st, g.m_enabler) == nullptr) {
            note_fail("group enabler does not resolve");
        }
        if (g.m_slot_n == 0 || g.m_slot_n > MAX_EFFECT_COUNT) {
            note_fail("group has invalid slot_n");
        }
    }

    print_by_group(st, reg, filter);
    print_by_resource(st, reg, filter);
    print_by_yield(st, reg, filter);

    BoosterRegisterToggleEnv env;
    if (!env.bind(st)) {
        note_fail("toggle env bind");
        return 1;
    }
    DynProduceRegister::Inventory inv;
    DynProduceRegister::CityYields yld_buf;
    if (!make_inv(inv, reg.inv_n())) {
        note_fail("make_inv");
        return 1;
    }
    for (u16 w = 0; w < 8; ++w) {
        env.clear_all();
        if (n > 0) {
            activate(env, rows[0].m_enabler);
        }
        fill_inv(inv, 100);
        reg.clear_yld(yld_buf);
        reg.apply(inv, yld_buf, env.make_ctx());
    }
    g_look_ns = 0;
    g_look_n = 0;
    for (u16 i = 0; i < n; ++i) {
        env.clear_all();
        activate(env, rows[i].m_enabler);
        fill_inv(inv, 100);
        reg.clear_yld(yld_buf);
        timed_apply(reg, inv, yld_buf, env.make_ctx());
    }
    free_inv(inv);

    const u64 avg_ns = (g_look_n == 0) ? 0 : (g_look_ns / static_cast<u64>(g_look_n));
    std::printf("fails=%d\n", g_fails);
    std::printf("avg lookup_ns=%llu\n", static_cast<unsigned long long>(avg_ns));
    return g_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
