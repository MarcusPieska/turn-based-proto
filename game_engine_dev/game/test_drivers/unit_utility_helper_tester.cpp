//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>

#include "game_primitives.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "unit_utility_helper.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";

//================================================================================================================================
//=> - Timing -
//================================================================================================================================

struct CallTimes {
    f64 m_min;
    f64 m_max;
    f64 m_sum;
    u32 m_n;
};

static void times_init (CallTimes& t) {
    t.m_min = 1.0e300;
    t.m_max = 0.0;
    t.m_sum = 0.0;
    t.m_n = 0u;
}

static void times_add (CallTimes& t, f64 us) {
    if (us < t.m_min) {
        t.m_min = us;
    }
    if (us > t.m_max) {
        t.m_max = us;
    }
    t.m_sum += us;
    t.m_n++;
}

static void times_print (cstr label, const CallTimes& t) {
    if (t.m_n == 0u) {
        std::printf("%s: no samples\n", label);
        return;
    }
    const f64 avg = t.m_sum / static_cast<f64>(t.m_n);
    std::printf("%s: n=%u min=%.2f us max=%.2f us avg=%.2f us\n", label, t.m_n, t.m_min, t.m_max, avg);
}

static f64 elapsed_us (std::chrono::steady_clock::time_point t0, std::chrono::steady_clock::time_point t1) {
    return std::chrono::duration<f64, std::micro>(t1 - t0).count();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("unit_utility_helper: cannot load runtime statics\n");
        return 1;
    }
    const RuntimeStatics& st = loader.statics();
    CallTimes t_setup;
    CallTimes t_query;
    CallTimes t_clear;
    times_init(t_setup);
    times_init(t_query);
    times_init(t_clear);
    {
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = UnitUtilityHelper::setup(st);
        const auto t1 = std::chrono::steady_clock::now();
        times_add(t_setup, elapsed_us(t0, t1));
        if (!ok) {
            std::printf("unit_utility_helper: setup failed\n");
            return 1;
        }
    }
    const UnitStaticData& units = st.unit();
    const u16 unit_n = units.get_item_count();
    std::printf("Will be selected for land armies\n");
    for (u16 i = 0; i < unit_n; ++i) {
        const UnitStaticDataStruct& u = units.get_item(UnitStaticDataKey::from_raw(i));
        const auto t0 = std::chrono::steady_clock::now();
        const bool sel = UnitUtilityHelper::is_type_for_land_army(u.type);
        const auto t1 = std::chrono::steady_clock::now();
        times_add(t_query, elapsed_us(t0, t1));
        if (!sel) {
            continue;
        }
        cstr nm = units.get_name(UnitStaticDataKey::from_raw(i));
        std::printf("  %s\n", nm != nullptr ? nm : "?");
    }
    std::printf("Will not be selected for land armies\n");
    for (u16 i = 0; i < unit_n; ++i) {
        const UnitStaticDataStruct& u = units.get_item(UnitStaticDataKey::from_raw(i));
        const auto t0 = std::chrono::steady_clock::now();
        const bool sel = UnitUtilityHelper::is_type_for_land_army(u.type);
        const auto t1 = std::chrono::steady_clock::now();
        times_add(t_query, elapsed_us(t0, t1));
        if (sel) {
            continue;
        }
        cstr nm = units.get_name(UnitStaticDataKey::from_raw(i));
        std::printf("  %s\n", nm != nullptr ? nm : "?");
    }
    {
        const auto t0 = std::chrono::steady_clock::now();
        UnitUtilityHelper::clear();
        const auto t1 = std::chrono::steady_clock::now();
        times_add(t_clear, elapsed_us(t0, t1));
    }
    times_print("setup", t_setup);
    times_print("is_type_for_land_army", t_query);
    times_print("clear", t_clear);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
