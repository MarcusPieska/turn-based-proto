//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "dynamic_bit_field.h"
#include "general_bit_bank.h"

//================================================================================================================================
//=> - Bank access (friend City pattern) -
//================================================================================================================================

class City {
public:
    static GeneralBitBank* create_bank (u16 batch_size) {
        return new GeneralBitBank(batch_size);
    }

    static void destroy_bank (GeneralBitBank* bank) {
        delete bank;
    }
};

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u16 k_n = 128;

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

static u16 pick_field (const DynamicBitFieldLay& lay) {
    u16 best = U16_KEY_NULL;
    u8 best_w = 0;
    for (u16 f = 0; f < k_n; ++f) {
        const u8 w = lay.m_slots[f].m_wid;
        if (w > best_w) {
            best_w = w;
            best = f;
        }
    }
    return best;
}

//================================================================================================================================
//=> - Layout viz -
//================================================================================================================================

static void print_layout (DynamicBitField& bf, const DynamicBitFieldLay& lay, u16 sel) {
    static const cstr k_red = "\033[31m";
    static const cstr k_grn = "\033[32m";
    static const cstr k_blu = "\033[34m";
    static const cstr k_rst = "\033[0m";
    u32 col = 0;
    for (u16 f = 0; f < k_n; ++f) {
        const u8 w = lay.m_slots[f].m_wid;
        const u8 val = bf.get(f);
        const cstr tint = (f == sel) ? k_blu : ((f & 1u) ? k_grn : k_red);
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
    const u16 sel = pick_field(lay);
    if (sel == U16_KEY_NULL || lay.m_slots[sel].m_wid < 2) {
        std::printf("*** FAILED no non-trivial field\n");
        return 1;
    }
    GeneralBitBank* bank = City::create_bank(lay.m_bits);
    if (bank == nullptr) {
        std::printf("*** FAILED bank create\n");
        return 1;
    }
    DynamicBitField bf(&lay, bank);
    if (bf.n() != k_n || bf.ix() == U16_KEY_NULL) {
        std::printf("*** FAILED construct\n");
        City::destroy_bank(bank);
        return 1;
    }
    for (u16 f = 0; f < k_n; ++f) {
        bf.set(f, 0);
    }
    const u8 vmax = lay.m_vals[sel];
    for (u16 v = 0; v < vmax; ++v) {
        bf.set(sel, SC_U8(v));
        print_layout(bf, lay, sel);
        if (v + 1u < vmax) {
            std::printf("\n");
        }
    }
    City::destroy_bank(bank);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
