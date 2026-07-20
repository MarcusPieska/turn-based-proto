//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "profile_time_opt.h"

#include <cstdio>

#ifdef PTO_ENABLE

#include <chrono>
#include <cstdlib>

static const u16 k_pto_n = static_cast<u16>(PtoId::PTO_COUNT);

struct PtoEnt {
    u64 m_t0_ns;
    u64 m_total_ns;
    u32 m_calls;
};

static PtoEnt s_ent[k_pto_n];

static cstr s_nm[k_pto_n] = {
    "PTO_CITY_LOOP",
    "PTO_UNIT_LOOP"
};

static u64 pto_now_ns () {
    const auto t = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t).count());
}

static void pto_fatal (cstr msg, PtoId id) {
    const u16 i = static_cast<u16>(id);
    if (i < k_pto_n) {
        std::fprintf(stderr, "ProfileTimeOpt fatal: %s (%s)\n", msg, s_nm[i]);
    } else {
        std::fprintf(stderr, "ProfileTimeOpt fatal: %s (id=%u)\n", msg, i);
    }
    std::exit(1);
}

static f64 pto_ns_to_ms (u64 ns) {
    return static_cast<f64>(ns) / 1000000.0;
}

static void pto_print_ms (f64 ms) {
    if (ms >= 1000.0) {
        std::printf("%.3f s", ms / 1000.0);
        return;
    }
    if (ms >= 1.0) {
        std::printf("%.3f ms", ms);
        return;
    }
    std::printf("%.3f us", ms * 1000.0);
}

#endif

void ProfileTimeOpt::init () {
#ifdef PTO_ENABLE
    for (u16 i = 0; i < k_pto_n; ++i) {
        s_ent[i].m_t0_ns = 0;
        s_ent[i].m_total_ns = 0;
        s_ent[i].m_calls = 0;
    }
#endif
}

void ProfileTimeOpt::start (PtoId id) {
#ifdef PTO_ENABLE
    const u16 i = static_cast<u16>(id);
    if (i >= k_pto_n) {
        pto_fatal("bad id on start", id);
    }
    PtoEnt& e = s_ent[i];
    if (e.m_t0_ns != 0u) {
        std::printf("ProfileTimeOpt error: recursive call to %s\n", s_nm[i]);
        std::exit(1);
    }
    e.m_t0_ns = pto_now_ns();
#endif
}

void ProfileTimeOpt::stop (PtoId id) {
#ifdef PTO_ENABLE
    const u16 i = static_cast<u16>(id);
    if (i >= k_pto_n) {
        pto_fatal("bad id on stop", id);
    }
    PtoEnt& e = s_ent[i];
    if (e.m_t0_ns == 0u) {
        pto_fatal("stop without start", id);
    }
    const u64 dt = pto_now_ns() - e.m_t0_ns;
    e.m_t0_ns = 0;
    e.m_total_ns += dt;
    e.m_calls++;
#endif
}

void ProfileTimeOpt::print () {
#ifdef PTO_ENABLE
    for (u16 i = 0; i < k_pto_n; ++i) {
        if (s_ent[i].m_t0_ns != 0u) {
            pto_fatal("unclosed start remains", static_cast<PtoId>(i));
        }
    }
    u16 ord[k_pto_n];
    for (u16 i = 0; i < k_pto_n; ++i) {
        ord[i] = i;
    }
    for (u16 i = 0; i + 1u < k_pto_n; ++i) {
        for (u16 j = static_cast<u16>(i + 1u); j < k_pto_n; ++j) {
            if (s_ent[ord[j]].m_total_ns > s_ent[ord[i]].m_total_ns) {
                const u16 t = ord[i];
                ord[i] = ord[j];
                ord[j] = t;
            }
        }
    }
    std::printf("ProfileTimeOpt summary (%u entries)\n", k_pto_n);
    for (u16 k = 0; k < k_pto_n; ++k) {
        const u16 i = ord[k];
        const PtoEnt& e = s_ent[i];
        const f64 total_ms = pto_ns_to_ms(e.m_total_ns);
        const f64 avg_ms = e.m_calls > 0u ? total_ms / static_cast<f64>(e.m_calls) : 0.0;
        std::printf("  %-48s  calls %6u  total ", s_nm[i], e.m_calls);
        pto_print_ms(total_ms);
        std::printf("  avg ");
        pto_print_ms(avg_ms);
        std::printf("\n");
    }
#endif
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
