//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>

#include "city_job_static_data.h"
#include "city_job_static_key.h"
#include "dyn_job_yield_register.h"
#include "job_count_array.h"
#include "job_effect_realizer.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"

typedef std::chrono::high_resolution_clock Clock;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u32 k_reps = 4000u;

static f64 ns_between (Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<f64, std::nano>(b - a).count();
}

static void print_pack (cstr lbl, const DynJobYieldPack& p) {
    std::printf("%s n=%u food=%d prod=%d com=%d cult=%d sci=%d rel=%d\n",
        lbl, SC_U32(p.m_n), p.m_food, p.m_production, p.m_commerce,
        p.m_culture, p.m_science, p.m_religion);
}

static bool pack_eq (const DynJobYieldPack& a, const DynJobYieldPack& b) {
    return a.m_n == b.m_n
        && a.m_food == b.m_food
        && a.m_production == b.m_production
        && a.m_commerce == b.m_commerce
        && a.m_culture == b.m_culture
        && a.m_science == b.m_science
        && a.m_religion == b.m_religion;
}

static DynJobYieldPack pack_delta (const DynJobYieldPack& a, const DynJobYieldPack& b) {
    DynJobYieldPack d = {};
    d.m_n = SC_U16(b.m_n - a.m_n);
    d.m_food = b.m_food - a.m_food;
    d.m_production = b.m_production - a.m_production;
    d.m_commerce = b.m_commerce - a.m_commerce;
    d.m_culture = b.m_culture - a.m_culture;
    d.m_science = b.m_science - a.m_science;
    d.m_religion = b.m_religion - a.m_religion;
    return d;
}

static DynJobYieldPack row_as_pack (const DynJobYieldRow& r) {
    DynJobYieldPack p = {};
    p.m_n = 1;
    p.m_food = SC_I32(r.m_food);
    p.m_production = SC_I32(r.m_production);
    p.m_commerce = SC_I32(r.m_commerce);
    p.m_culture = SC_I32(r.m_culture);
    p.m_science = SC_I32(r.m_science);
    p.m_religion = SC_I32(r.m_religion);
    return p;
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
    const u16 city_idx = JobCountArray::claim();
    if (city_idx == U16_KEY_NULL) {
        std::printf("*** FAILED claim\n");
        JobCountArray::clear();
        return 1;
    }
    const u16 jn = JobCountArray::job_n();
    const DynJobYieldRow* rows = st.dyn_job_yield().rows();
    if (rows == nullptr) {
        std::printf("*** FAILED yield rows\n");
        JobCountArray::clear();
        return 1;
    }
    for (u16 j = 0; j < jn; ++j) {
        JobCountArray::set(city_idx, j, 0);
    }

    bool ok = true;
    std::printf("=== infer per-job impact (add one seat, diff packs) ===\n");
    DynJobYieldPack prev = JobEffectRealizer::realize(city_idx, st);
    print_pack("empty", prev);
    for (u16 j = 0; j < jn; ++j) {
        JobCountArray::set(city_idx, j, 1);
        const DynJobYieldPack cur = JobEffectRealizer::realize(city_idx, st);
        const DynJobYieldPack delta = pack_delta(prev, cur);
        const DynJobYieldPack expect = row_as_pack(rows[j]);
        cstr nm = st.city_job().get_name(CityJobStaticDataKey::from_raw(j));
        std::printf("  +%s -> ", nm ? nm : "?");
        print_pack("delta", delta);
        if (!pack_eq(delta, expect)) {
            std::printf("*** FAILED inferred impact mismatch job %u\n", SC_U32(j));
            print_pack("    expect", expect);
            ok = false;
        }
        prev = cur;
    }

    f64 rmin = 1e300;
    f64 rmax = 0.0;
    f64 rsum = 0.0;
    volatile i32 sink = 0;
    for (u32 r = 0; r < k_reps; ++r) {
        const Clock::time_point t0 = Clock::now();
        const DynJobYieldPack p = JobEffectRealizer::realize(city_idx, st);
        const Clock::time_point t1 = Clock::now();
        sink += p.m_food + p.m_science;
        const f64 ns = ns_between(t0, t1);
        if (ns < rmin) rmin = ns;
        if (ns > rmax) rmax = ns;
        rsum += ns;
    }
    std::printf("=== realize timing (all seats=1, reps %u, sink %d) ===\n", k_reps, sink);
    std::printf("ns/call: min %.2f  max %.2f  avg %.2f\n", rmin, rmax, rsum / k_reps);

    if (ok) {
        std::printf("*** PASSED job_effect_realizer\n");
    }
    JobCountArray::clear();
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
