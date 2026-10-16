//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "dynamic_bit_field.h"

typedef std::chrono::high_resolution_clock Clock;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u16 k_n = 128;
static const u32 k_reps = 4000u;

static u32 g_seed = 1;

static u32 rnd () {
    g_seed = g_seed * 1664525u + 1013904223u;
    return g_seed;
}

static void shuffle (u8* a, u16 n) {
    for (u16 i = n; i > 1; --i) {
        const u16 j = SC_U16(rnd() % i);
        const u8 t = a[i - 1];
        a[i - 1] = a[j];
        a[j] = t;
    }
}

static void fill_vals (u8* vals) {
    for (u16 i = 0; i < k_n; ++i) {
        vals[i] = SC_U8(i + 1);
    }
    shuffle(vals, k_n);
}

static u8 bit_wid (u8 nv) {
    if (nv <= 1) {
        return 0;
    }
    u8 w = 0;
    for (u8 x = SC_U8(nv - 1); x != 0; x = SC_U8(x >> 1)) {
        ++w;
    }
    return w;
}

static void fill_lay (DynamicBitFieldLay& lay, u8* vals, DynamicBitFieldSlot* slots) {
    u32 bits = 0;
    for (u16 i = 0; i < k_n; ++i) {
        slots[i].m_off = SC_U16(bits);
        slots[i].m_wid = bit_wid(vals[i]);
        bits += SC_U32(slots[i].m_wid);
    }
    lay.m_n = k_n;
    lay.m_vals = vals;
    lay.m_slots = slots;
    lay.m_bits = SC_U16(bits);
    lay.m_bytes = (bits + 7u) / 8u;
    if (lay.m_bytes == 0) {
        lay.m_bytes = 1;
    }
}

static f64 ns_between (Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<f64, std::nano>(b - a).count();
}

//================================================================================================================================
//=> - Layout viz -
//================================================================================================================================

static void print_layout (DynamicBitField& bf, const DynamicBitFieldLay& lay) {
    static const cstr k_red = "\033[31m";
    static const cstr k_grn = "\033[32m";
    static const cstr k_rst = "\033[0m";
    for (u16 f = 0; f < k_n; ++f) {
        const u8 vmax = lay.m_vals[f];
        bf.set(f, vmax > 1 ? SC_U8(vmax - 1) : SC_U8(0));
    }
    u32 col = 0;
    for (u16 f = 0; f < k_n; ++f) {
        const u8 w = lay.m_slots[f].m_wid;
        const u8 val = bf.get(f);
        const cstr tint = (f & 1u) ? k_grn : k_red;
        for (u8 bi = 0; bi < w; ++bi) {
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

//================================================================================================================================
//=> - Correctness -
//================================================================================================================================

static bool check_correct (DynamicBitField& bf, const u8* vals) {
    u8 snap[k_n];
    for (u16 i = 0; i < k_n; ++i) {
        bf.set(i, 0);
        snap[i] = 0;
    }
    for (u16 f = 0; f < k_n; ++f) {
        const u8 vmax = vals[f];
        for (u16 v = 0; v < vmax; ++v) {
            const u8 uv = SC_U8(v);
            bf.set(f, uv);
            snap[f] = uv;
            if (bf.get(f) != uv) {
                std::printf("*** FAILED get field %u expect %u got %u\n",
                    SC_U32(f), SC_U32(uv), SC_U32(bf.get(f)));
                return false;
            }
            for (u16 o = 0; o < k_n; ++o) {
                if (o == f) {
                    continue;
                }
                if (bf.get(o) != snap[o]) {
                    std::printf("*** FAILED side field %u touched writing %u=%u; got %u expect %u\n",
                        SC_U32(o), SC_U32(f), SC_U32(uv), SC_U32(bf.get(o)), SC_U32(snap[o]));
                    return false;
                }
            }
        }
        bf.set(f, 0);
        snap[f] = 0;
    }
    std::printf("*** PASSED correctness (%u fields, full ranges, no side writes)\n", SC_U32(k_n));
    return true;
}

//================================================================================================================================
//=> - Timing -
//================================================================================================================================

static void time_access (DynamicBitField& bf, const u8* vals) {
    volatile u32 sink = 0;
    for (u16 f = 0; f < k_n; ++f) {
        bf.set(f, 0);
        sink += bf.get(f);
    }
    f64 set_min = 1e300;
    f64 set_max = 0.0;
    f64 set_sum = 0.0;
    f64 get_min = 1e300;
    f64 get_max = 0.0;
    f64 get_sum = 0.0;
    for (u32 r = 0; r < k_reps; ++r) {
        const Clock::time_point s0 = Clock::now();
        for (u16 f = 0; f < k_n; ++f) {
            bf.set(f, SC_U8((r + f) % vals[f]));
        }
        const Clock::time_point s1 = Clock::now();
        for (u16 f = 0; f < k_n; ++f) {
            sink += bf.get(f);
        }
        const Clock::time_point s2 = Clock::now();
        const f64 set_ns = ns_between(s0, s1) / SC_U32(k_n);
        const f64 get_ns = ns_between(s1, s2) / SC_U32(k_n);
        if (set_ns < set_min) set_min = set_ns;
        if (set_ns > set_max) set_max = set_ns;
        set_sum += set_ns;
        if (get_ns < get_min) get_min = get_ns;
        if (get_ns > get_max) get_max = get_ns;
        get_sum += get_ns;
    }
    std::printf("mk2 bytes %u  sink %u  reps %u\n", bf.bytes(), sink, k_reps);
    std::printf("mk2 set ns/access: min %.2f  max %.2f  avg %.2f\n",
        set_min, set_max, set_sum / k_reps);
    std::printf("mk2 get ns/access: min %.2f  max %.2f  avg %.2f\n",
        get_min, get_max, get_sum / k_reps);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: %s <seed>\n", argv[0]);
        return 1;
    }
    const u32 seed_in = SC_U32(std::strtoul(argv[1], nullptr, 10));
    const u32 seed = seed_in == 0 ? 1u : seed_in;
    g_seed = seed;
    u8 vals[k_n];
    DynamicBitFieldSlot slots[k_n];
    fill_vals(vals);
    DynamicBitFieldLay lay;
    fill_lay(lay, vals, slots);
    DynamicBitField bf(&lay);
    if (bf.n() != k_n) {
        std::printf("*** FAILED construct\n");
        return 1;
    }
    print_layout(bf, lay);
    std::printf("DynamicBitField mk2 tester  seed %u  fields %u\n", seed, SC_U32(k_n));
    if (!check_correct(bf, vals)) {
        return 1;
    }
    time_access(bf, vals);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
