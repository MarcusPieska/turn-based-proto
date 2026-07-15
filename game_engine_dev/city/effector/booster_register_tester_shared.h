//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BOOSTER_REGISTER_TESTER_SHARED_H
#define BOOSTER_REGISTER_TESTER_SHARED_H

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "bit_array.h"
#include "booster_effect_register.h"
#include "building_static_key.h"
#include "city_array.h"
#include "effect_ctx.h"
#include "general_bit_bank.h"
#include "runtime_static_loader.h"
#include "small_wonder_static_key.h"
#include "tech_static_key.h"
#include "wonder_static_key.h"

typedef const char* cstr;

//================================================================================================================================
//=> - ToggleEnv -
//================================================================================================================================

struct BoosterRegisterToggleEnv {
    CityArray m_array;
    BitArrayCL* m_techs = nullptr;
    u16* m_wonder_city = nullptr;
    u16* m_sw_city = nullptr;
    u16 m_city_idx = 0;
    RuntimeStatics* m_st = nullptr;

    ~BoosterRegisterToggleEnv () {
        delete m_techs;
        delete[] m_wonder_city;
        delete[] m_sw_city;
    }

    bool bind (RuntimeStatics& st) {
        m_st = &st;
        if (!m_array.bind_statics(st)) {
            return false;
        }
        const u16 tech_n = st.tech().get_item_count();
        const u16 wonder_n = st.wonder().get_item_count();
        const u16 sw_n = st.small_wonder().get_item_count();
        m_techs = new BitArrayCL(tech_n);
        m_wonder_city = new u16[wonder_n];
        m_sw_city = new u16[sw_n];
        m_city_idx = m_array.get_next_new_city_idx();
        clear_all();
        return true;
    }

    void clear_all () {
        if (m_st == nullptr) {
            return;
        }
        const u16 bld_n = m_st->building().get_item_count();
        for (u16 i = 0; i < bld_n; ++i) {
            m_array.get_bld_bank()->clear_flag(m_city_idx, i);
        }
        m_techs->clear_all();
        for (u16 i = 0; i < m_st->wonder().get_item_count(); ++i) {
            m_wonder_city[i] = U16_KEY_NULL;
        }
        for (u16 i = 0; i < m_st->small_wonder().get_item_count(); ++i) {
            m_sw_city[i] = U16_KEY_NULL;
        }
    }

    EffectCtx make_ctx () const {
        EffectCtx ctx = {};
        ctx.m_owner = 0;
        ctx.m_city_idx = m_city_idx;
        ctx.m_tech = m_techs;
        ctx.m_bld_bank = m_array.get_bld_bank();
        ctx.m_wonder_city = m_wonder_city;
        ctx.m_wonder_n = m_st->wonder().get_item_count();
        ctx.m_small_wonder_city = m_sw_city;
        ctx.m_small_wonder_n = m_st->small_wonder().get_item_count();
        return ctx;
    }
};

//================================================================================================================================
//=> - ScanTiming -
//================================================================================================================================

struct BoosterRegisterScanTiming {
    u64 m_det_ns = 0;
    u32 m_det_n = 0;

    void add_det (const std::chrono::high_resolution_clock::time_point& t0,
        const std::chrono::high_resolution_clock::time_point& t1) {
        m_det_ns += static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        ++m_det_n;
    }
};

//================================================================================================================================
//=> - Shared scan helpers -
//================================================================================================================================

inline bool booster_register_has_boost (const BoosterRegisterResult& r) {
    return r.m_unit != 0 || r.m_perc != 0;
}

inline double booster_register_ns_to_us (u64 ns) {
    return static_cast<double>(ns) / 1000.0;
}

inline void booster_register_print_us_line (cstr lbl, u64 ns) {
    std::printf("  %s: %.2f us\n", lbl, booster_register_ns_to_us(ns));
}

template<class Register>
inline BoosterRegisterResult booster_register_timed_determine (
    BoosterRegisterToggleEnv& env,
    BoosterRegisterScanTiming& tm
) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    BoosterRegisterResult r = Register::determine_effect(env.make_ctx());
    const auto t1 = std::chrono::high_resolution_clock::now();
    tm.add_det(t0, t1);
    return r;
}

template<class Register>
inline void booster_register_scan_buildings (BoosterRegisterToggleEnv& env, BoosterRegisterScanTiming& tm) {
    const RuntimeStatics& st = *env.m_st;
    const u16 n = st.building().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        env.clear_all();
        env.m_array.set_building_flag(env.m_city_idx, i);
        BoosterRegisterResult r = booster_register_timed_determine<Register>(env, tm);
        if (!booster_register_has_boost(r)) {
            continue;
        }
        cstr nm = st.building().get_name(BuildingStaticDataKey::from_raw(i));
        std::printf("  building %s -> unit=%d perc=%d\n", nm != nullptr ? nm : "?", static_cast<int>(r.m_unit), static_cast<int>(r.m_perc));
    }
}

template<class Register>
inline void booster_register_scan_techs (BoosterRegisterToggleEnv& env, BoosterRegisterScanTiming& tm) {
    const RuntimeStatics& st = *env.m_st;
    const u16 n = st.tech().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        env.clear_all();
        env.m_techs->set_bit(i);
        BoosterRegisterResult r = booster_register_timed_determine<Register>(env, tm);
        if (!booster_register_has_boost(r)) {
            continue;
        }
        cstr nm = st.tech().get_name(TechStaticDataKey::from_raw(i));
        std::printf("  tech %s -> unit=%d perc=%d\n", nm != nullptr ? nm : "?", static_cast<int>(r.m_unit), static_cast<int>(r.m_perc));
    }
}

template<class Register>
inline void booster_register_scan_small_wonders (BoosterRegisterToggleEnv& env, BoosterRegisterScanTiming& tm) {
    const RuntimeStatics& st = *env.m_st;
    const u16 n = st.small_wonder().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        env.clear_all();
        env.m_sw_city[i] = env.m_city_idx;
        BoosterRegisterResult r = booster_register_timed_determine<Register>(env, tm);
        if (!booster_register_has_boost(r)) {
            continue;
        }
        cstr nm = st.small_wonder().get_name(SmallWonderStaticDataKey::from_raw(i));
        std::printf("  small_wonder %s -> unit=%d perc=%d\n", nm != nullptr ? nm : "?", static_cast<int>(r.m_unit), static_cast<int>(r.m_perc));
    }
}

template<class Register>
inline void booster_register_scan_wonders (BoosterRegisterToggleEnv& env, BoosterRegisterScanTiming& tm) {
    const RuntimeStatics& st = *env.m_st;
    const u16 n = st.wonder().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        env.clear_all();
        env.m_wonder_city[i] = env.m_city_idx;
        BoosterRegisterResult r = booster_register_timed_determine<Register>(env, tm);
        if (!booster_register_has_boost(r)) {
            continue;
        }
        cstr nm = st.wonder().get_name(WonderStaticDataKey::from_raw(i));
        std::printf("  wonder %s -> unit=%d perc=%d\n", nm != nullptr ? nm : "?", static_cast<int>(r.m_unit), static_cast<int>(r.m_perc));
    }
}

template<class Register>
inline int booster_register_run_toggle_scan (RuntimeStatics& st, cstr scope_lbl, cstr type_lbl) {
    BoosterRegisterToggleEnv env;
    if (!env.bind(st)) {
        std::printf("ERROR: failed to bind toggle env\n");
        return 1;
    }

    std::printf("%s %s register toggle scan (one active source at a time)\n", scope_lbl, type_lbl);
    std::printf("register entries: %u\n", static_cast<unsigned>(Register::ENTRY_N));
    std::printf("hits:\n");

    BoosterRegisterScanTiming tm = {};
    const auto scan_t0 = std::chrono::high_resolution_clock::now();
    booster_register_scan_buildings<Register>(env, tm);
    booster_register_scan_techs<Register>(env, tm);
    booster_register_scan_small_wonders<Register>(env, tm);
    booster_register_scan_wonders<Register>(env, tm);
    const auto scan_t1 = std::chrono::high_resolution_clock::now();
    const u64 scan_ns = static_cast<u64>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(scan_t1 - scan_t0).count());

    std::printf("timing:\n");
    booster_register_print_us_line("full toggle scan", scan_ns);
    booster_register_print_us_line("determine_effect total", tm.m_det_ns);
    if (tm.m_det_n > 0) {
        std::printf("  determine_effect avg: %.2f us (n=%u)\n",
            booster_register_ns_to_us(tm.m_det_ns / static_cast<u64>(tm.m_det_n)),
            tm.m_det_n);
    }

    std::printf("done\n");
    return 0;
}

#endif // BOOSTER_REGISTER_TESTER_SHARED_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
