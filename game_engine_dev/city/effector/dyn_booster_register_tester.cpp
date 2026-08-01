//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "booster_register_tester_shared.h"
#include "building_static_key.h"
#include "city_commerce_booster_register.h"
#include "dyn_booster_register.h"
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

static BoosterRegisterResult timed_det (const DynBoosterRegister& reg,
    ItemEffectBoosterType tp, ItemEffectsScope sc, const EffectCtx& ctx) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    const BoosterRegisterResult r = reg.determine(tp, sc, ctx);
    const auto t1 = std::chrono::high_resolution_clock::now();
    g_look_ns += static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    ++g_look_n;
    return r;
}

static bool has_boost (const BoosterRegisterResult& r) {
    return r.m_unit != 0 || r.m_perc != 0;
}

struct DynBoostHit {
    cstr m_kind;
    cstr m_name;
    i16 m_unit;
    i16 m_perc;
};

static void push_hit (DynBoostHit*& rows, u32& n, u32& cap, cstr kind, cstr name, const BoosterRegisterResult& r) {
    if (n == cap) {
        const u32 next = (cap == 0) ? 16u : (cap * 2u);
        DynBoostHit* neu = new DynBoostHit[next];
        for (u32 i = 0; i < n; ++i) {
            neu[i] = rows[i];
        }
        delete[] rows;
        rows = neu;
        cap = next;
    }
    rows[n].m_kind = kind;
    rows[n].m_name = (name != nullptr) ? name : "?";
    rows[n].m_unit = r.m_unit;
    rows[n].m_perc = r.m_perc;
    ++n;
}

static void scan_bucket (const DynBoosterRegister& reg, BoosterRegisterToggleEnv& env,
    ItemEffectBoosterType tp, ItemEffectsScope sc) {
    const RuntimeStatics& st = *env.m_st;
    const u16 n_ent = reg.entry_count(tp, sc);
    if (n_ent == 0) {
        return;
    }

    DynBoostHit* hits = nullptr;
    u32 hit_n = 0;
    u32 hit_cap = 0;
    u64 buck_ns = 0;
    u32 buck_n = 0;

    const u16 bld_n = st.building().get_item_count();
    for (u16 i = 0; i < bld_n; ++i) {
        env.clear_all();
        env.m_array.get_bld_bank()->set_flag(env.m_city_idx, i);
        const u64 before = g_look_ns;
        const u32 before_n = g_look_n;
        const BoosterRegisterResult r = timed_det(reg, tp, sc, env.make_ctx());
        buck_ns += (g_look_ns - before);
        buck_n += (g_look_n - before_n);
        if (has_boost(r)) {
            push_hit(hits, hit_n, hit_cap, "building",
                st.building().get_name(BuildingStaticDataKey::from_raw(i)), r);
        }
    }
    const u16 tech_n = st.tech().get_item_count();
    for (u16 i = 0; i < tech_n; ++i) {
        env.clear_all();
        env.m_techs->set_bit(i);
        const u64 before = g_look_ns;
        const u32 before_n = g_look_n;
        const BoosterRegisterResult r = timed_det(reg, tp, sc, env.make_ctx());
        buck_ns += (g_look_ns - before);
        buck_n += (g_look_n - before_n);
        if (has_boost(r)) {
            push_hit(hits, hit_n, hit_cap, "tech",
                st.tech().get_name(TechStaticDataKey::from_raw(i)), r);
        }
    }
    const u16 sw_n = st.small_wonder().get_item_count();
    for (u16 i = 0; i < sw_n; ++i) {
        env.clear_all();
        env.m_sw_city[i] = env.m_city_idx;
        const u64 before = g_look_ns;
        const u32 before_n = g_look_n;
        const BoosterRegisterResult r = timed_det(reg, tp, sc, env.make_ctx());
        buck_ns += (g_look_ns - before);
        buck_n += (g_look_n - before_n);
        if (has_boost(r)) {
            push_hit(hits, hit_n, hit_cap, "small_wonder",
                st.small_wonder().get_name(SmallWonderStaticDataKey::from_raw(i)), r);
        }
    }
    const u16 w_n = st.wonder().get_item_count();
    for (u16 i = 0; i < w_n; ++i) {
        env.clear_all();
        env.m_wonder_city[i] = env.m_city_idx;
        const u64 before = g_look_ns;
        const u32 before_n = g_look_n;
        const BoosterRegisterResult r = timed_det(reg, tp, sc, env.make_ctx());
        buck_ns += (g_look_ns - before);
        buck_n += (g_look_n - before_n);
        if (has_boost(r)) {
            push_hit(hits, hit_n, hit_cap, "wonder",
                st.wonder().get_name(WonderStaticDataKey::from_raw(i)), r);
        }
    }

    const u64 avg_ns = (buck_n == 0) ? 0 : (buck_ns / static_cast<u64>(buck_n));
    std::printf("%s %s entries=%u (%llu ns)\n",
        ItemEffectHelper::effects_scope_enum_to_str(sc),
        ItemEffectHelper::booster_type_enum_to_str(tp),
        static_cast<u32>(n_ent),
        static_cast<unsigned long long>(avg_ns));
    for (u32 i = 0; i < hit_n; ++i) {
        std::printf("  %s %s -> unit=%d perc=%d\n",
            hits[i].m_kind, hits[i].m_name,
            static_cast<int>(hits[i].m_unit), static_cast<int>(hits[i].m_perc));
    }
    delete[] hits;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    ItemEffectBoosterType filter_tp = ItemEffectBoosterType::NONE;
    ItemEffectsScope filter_sc = ItemEffectsScope::NONE;
    if (argc >= 2) {
        filter_tp = ItemEffectHelper::booster_type_str_to_enum(argv[1]);
        if (filter_tp == ItemEffectBoosterType::NONE) {
            std::printf("unknown booster type '%s'\n", argv[1]);
            return 1;
        }
    }
    if (argc >= 3) {
        filter_sc = ItemEffectHelper::effects_scope_str_to_enum(argv[2]);
        if (filter_sc == ItemEffectsScope::NONE) {
            std::printf("unknown scope '%s'\n", argv[2]);
            return 1;
        }
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        std::printf("statics failed\n");
        return 1;
    }
    RuntimeStatics& st = g_rt_loader.statics();
    const DynBoosterRegister& reg = st.dyn_booster();
    std::printf("dyn_booster_register entries=%u keys=%u\n",
        static_cast<u32>(reg.entry_count()),
        static_cast<u32>(reg.key_count()));

    const u16 city_com = reg.entry_count(ItemEffectBoosterType::COMMERCE, ItemEffectsScope::CITY);
    if (city_com != CityCommerceBoosterRegister::ENTRY_N) {
        std::printf("FAIL: CITY COMMERCE entries dyn=%u codegen=%u\n",
            static_cast<u32>(city_com),
            static_cast<u32>(CityCommerceBoosterRegister::ENTRY_N));
        ++g_fails;
    }

    BoosterRegisterToggleEnv env;
    if (!env.bind(st)) {
        note_fail("toggle env bind");
        return 1;
    }

    for (u16 tp = 1; tp < DynBoosterRegister::TYPE_N; ++tp) {
        for (u16 sc = 1; sc < DynBoosterRegister::SCOPE_N; ++sc) {
            const auto tpe = static_cast<ItemEffectBoosterType>(tp);
            const auto sce = static_cast<ItemEffectsScope>(sc);
            if (filter_tp != ItemEffectBoosterType::NONE && tpe != filter_tp) {
                continue;
            }
            if (filter_sc != ItemEffectsScope::NONE && sce != filter_sc) {
                continue;
            }
            scan_bucket(reg, env, tpe, sce);
        }
    }

    const u64 avg_ns = (g_look_n == 0) ? 0 : (g_look_ns / static_cast<u64>(g_look_n));
    std::printf("fails=%d lookups=%u\n", g_fails, g_look_n);
    std::printf("avg lookup_ns=%llu\n", static_cast<unsigned long long>(avg_ns));
    return g_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
