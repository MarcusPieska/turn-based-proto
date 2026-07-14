//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "profile_time.h"

#include <cstdio>

namespace {

struct PTimeScope {
    cstr m_fn;
    explicit PTimeScope (cstr fn) : m_fn(fn) {
        ProfileTime::start(fn);
    }
    ~PTimeScope () {
        ProfileTime::stop(m_fn);
    }
};

static void ptime_trace_start (cstr name) {
#ifdef PTIME_TRACE_ENABLE
    if (name != nullptr) {
        std::printf(">> %s\n", name);
        std::fflush(stdout);
    }
#endif
}

static void ptime_trace_stop (cstr name) {
#ifdef PTIME_TRACE_ENABLE
    if (name != nullptr) {
        std::printf("<< %s\n", name);
        std::fflush(stdout);
    }
#endif
}

} // namespace

#ifdef PTIME_ENABLE

#include <chrono>
#include <cstdlib>
#include <cstring>

static const u32 k_ptime_ent_max = 256u;

struct PTimeEnt {
    cstr m_name;
    u64 m_t0_ns;
    u64 m_total_ns;
    u32 m_calls;
};

static PTimeEnt s_ent[k_ptime_ent_max];
static u32 s_ent_n = 0;

static u64 ptime_now_ns () {
    const auto t = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t).count());
}

static void ptime_fatal (cstr msg, cstr name) {
    if (name != nullptr) {
        std::fprintf(stderr, "ProfileTime fatal: %s (%s)\n", msg, name);
    } else {
        std::fprintf(stderr, "ProfileTime fatal: %s\n", msg);
    }
    std::exit(1);
}

static PTimeEnt* ptime_ent_find (cstr name) {
    for (u32 i = 0; i < s_ent_n; ++i) {
        if (std::strcmp(s_ent[i].m_name, name) == 0) {
            return &s_ent[i];
        }
    }
    return nullptr;
}

static PTimeEnt* ptime_ent_get (cstr name) {
    PTimeEnt* e = ptime_ent_find(name);
    if (e != nullptr) {
        return e;
    }
    if (s_ent_n >= k_ptime_ent_max) {
        ptime_fatal("entry table full", name);
    }
    e = &s_ent[s_ent_n];
    e->m_name = name;
    e->m_t0_ns = 0;
    e->m_total_ns = 0;
    e->m_calls = 0;
    s_ent_n++;
    return e;
}

static f64 ptime_ns_to_ms (u64 ns) {
    return static_cast<f64>(ns) / 1000000.0;
}

static void ptime_print_ms (f64 ms) {
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

void ProfileTime::init () {
#ifdef PTIME_ENABLE
    s_ent_n = 0;
#endif
}

void ProfileTime::start (cstr name) {
    ptime_trace_start(name);
#ifdef PTIME_ENABLE
    if (name == nullptr) {
        ptime_fatal("null name on start", nullptr);
    }
    PTimeEnt* e = ptime_ent_get(name);
    if (e->m_t0_ns != 0u) {
        std::printf("ProfileTime error: recursive call to %s\n", name);
        std::exit(1);
    }
    e->m_t0_ns = ptime_now_ns();
#endif
}

void ProfileTime::stop (cstr name) {
#ifdef PTIME_ENABLE
    if (name == nullptr) {
        ptime_fatal("null name on stop", nullptr);
    }
    PTimeEnt* e = ptime_ent_find(name);
    if (e == nullptr || e->m_t0_ns == 0u) {
        ptime_fatal("stop without start", name);
    }
    const u64 dt = ptime_now_ns() - e->m_t0_ns;
    e->m_t0_ns = 0;
    e->m_total_ns += dt;
    e->m_calls++;
#endif
    ptime_trace_stop(name);
}

void ProfileTime::print () {
#ifdef PTIME_ENABLE
    for (u32 i = 0; i < s_ent_n; ++i) {
        if (s_ent[i].m_t0_ns != 0u) {
            ptime_fatal("unclosed start remains", s_ent[i].m_name);
        }
    }
    u32 ord[k_ptime_ent_max];
    for (u32 i = 0; i < s_ent_n; ++i) {
        ord[i] = i;
    }
    for (u32 i = 0; i + 1u < s_ent_n; ++i) {
        for (u32 j = i + 1u; j < s_ent_n; ++j) {
            if (s_ent[ord[j]].m_total_ns > s_ent[ord[i]].m_total_ns) {
                const u32 t = ord[i];
                ord[i] = ord[j];
                ord[j] = t;
            }
        }
    }
    std::printf("ProfileTime summary (%u entries)\n", s_ent_n);
    for (u32 k = 0; k < s_ent_n; ++k) {
        const PTimeEnt& e = s_ent[ord[k]];
        const f64 total_ms = ptime_ns_to_ms(e.m_total_ns);
        const f64 avg_ms = e.m_calls > 0u ? total_ms / static_cast<f64>(e.m_calls) : 0.0;
        std::printf("  %-48s  calls %6u  total ", e.m_name, e.m_calls);
        ptime_print_ms(total_ms);
        std::printf("  avg ");
        ptime_print_ms(avg_ms);
        std::printf("\n");
    }
#endif
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
