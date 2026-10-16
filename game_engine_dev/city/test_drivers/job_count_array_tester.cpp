//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>

#include "city_job_static_data.h"
#include "city_job_static_key.h"
#include "job_count_array.h"
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

//================================================================================================================================
//=> - Report -
//================================================================================================================================

static void print_maxes (const RuntimeStatics& st) {
    const u16 jn = JobCountArray::job_n();
    const JobCountSlot* slots = JobCountArray::slots();
    std::printf("=== job max / layout ===\n");
    std::printf("jobs %u  batch_bits %u\n", SC_U32(jn), SC_U32(JobCountArray::bits()));
    for (u16 j = 0; j < jn; ++j) {
        cstr nm = st.city_job().get_name(CityJobStaticDataKey::from_raw(j));
        const JobCountSlot& s = slots[j];
        std::printf("  [%2u] %-20s  max=%u  wid=%u  off=%u\n",
            SC_U32(j), nm ? nm : "?", SC_U32(s.m_max), SC_U32(s.m_wid), SC_U32(s.m_off));
    }
}

static void print_layout (u16 city_idx) {
    static const cstr k_red = "\033[31m";
    static const cstr k_grn = "\033[32m";
    static const cstr k_rst = "\033[0m";
    const u16 jn = JobCountArray::job_n();
    const JobCountSlot* slots = JobCountArray::slots();
    for (u16 j = 0; j < jn; ++j) {
        JobCountArray::set(city_idx, j, JobCountArray::max_n(j));
    }
    std::printf("=== bit layout (all at max) ===\n");
    u32 col = 0;
    for (u16 j = 0; j < jn; ++j) {
        const JobCountSlot& s = slots[j];
        const u8 val = JobCountArray::get(city_idx, j);
        const cstr tint = (j & 1u) ? k_grn : k_red;
        for (u8 bi = 0; bi < s.m_wid; ++bi) {
            if (col > 0) {
                if ((col % 80u) == 0u) {
                    std::printf("\n");
                } else if ((col % 8u) == 0u) {
                    std::printf(" ");
                }
            }
            std::printf("%s%u%s", tint, SC_U32((val >> bi) & 1u), k_rst);
            ++col;
        }
    }
    if (col > 0) {
        std::printf("\n");
    }
}

static void time_access (u16 city_idx) {
    const u16 jn = JobCountArray::job_n();
    volatile u32 sink = 0;
    for (u16 j = 0; j < jn; ++j) {
        JobCountArray::set(city_idx, j, 0);
        sink += JobCountArray::get(city_idx, j);
    }
    f64 set_min = 1e300;
    f64 set_max = 0.0;
    f64 set_sum = 0.0;
    f64 get_min = 1e300;
    f64 get_max = 0.0;
    f64 get_sum = 0.0;
    for (u32 r = 0; r < k_reps; ++r) {
        const Clock::time_point s0 = Clock::now();
        for (u16 j = 0; j < jn; ++j) {
            const u8 mx = JobCountArray::max_n(j);
            const u8 v = mx == 0 ? SC_U8(0) : SC_U8((r + j) % (mx + 1u));
            JobCountArray::set(city_idx, j, v);
        }
        const Clock::time_point s1 = Clock::now();
        for (u16 j = 0; j < jn; ++j) {
            sink += JobCountArray::get(city_idx, j);
        }
        const Clock::time_point s2 = Clock::now();
        const f64 set_ns = ns_between(s0, s1) / SC_U32(jn);
        const f64 get_ns = ns_between(s1, s2) / SC_U32(jn);
        if (set_ns < set_min) set_min = set_ns;
        if (set_ns > set_max) set_max = set_ns;
        set_sum += set_ns;
        if (get_ns < get_min) get_min = get_ns;
        if (get_ns > get_max) get_max = get_ns;
        get_sum += get_ns;
    }
    std::printf("=== access timing ===\n");
    std::printf("city_idx %u  bits %u  sink %u  reps %u\n",
        SC_U32(city_idx), SC_U32(JobCountArray::bits()), sink, k_reps);
    std::printf("set ns/access: min %.2f  max %.2f  avg %.2f\n",
        set_min, set_max, set_sum / k_reps);
    std::printf("get ns/access: min %.2f  max %.2f  avg %.2f\n",
        get_min, get_max, get_sum / k_reps);
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
    print_maxes(st);
    const u16 city_idx = JobCountArray::claim();
    if (city_idx == U16_KEY_NULL) {
        std::printf("*** FAILED claim\n");
        JobCountArray::clear();
        return 1;
    }
    print_layout(city_idx);
    time_access(city_idx);
    JobCountArray::clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
