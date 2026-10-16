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
static const u16 k_slices = 8;

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

static u8 pattern_val (u16 slice, u16 field, u8 vmax) {
    if (vmax <= 1) {
        return 0;
    }
    return SC_U8((SC_U32(slice) * 17u + SC_U32(field) * 3u) % vmax);
}

static bool snap_eq (DynamicBitField& bf, const u8* snap) {
    for (u16 f = 0; f < k_n; ++f) {
        if (bf.get(f) != snap[f]) {
            return false;
        }
    }
    return true;
}

static void take_snap (DynamicBitField& bf, u8* snap) {
    for (u16 f = 0; f < k_n; ++f) {
        snap[f] = bf.get(f);
    }
}

//================================================================================================================================
//=> - Interference checks -
//================================================================================================================================

static bool check_slice_isolation (DynamicBitField** slices, const u8* vals) {
    u8 snap[k_slices][k_n];
    for (u16 s = 0; s < k_slices; ++s) {
        for (u16 f = 0; f < k_n; ++f) {
            const u8 v = pattern_val(s, f, vals[f]);
            slices[s]->set(f, v);
        }
        take_snap(*slices[s], snap[s]);
    }
    for (u16 s = 0; s < k_slices; ++s) {
        if (!snap_eq(*slices[s], snap[s])) {
            std::printf("*** FAILED isolation baseline slice %u\n", SC_U32(s));
            return false;
        }
    }
    for (u16 tgt = 0; tgt < k_slices; ++tgt) {
        for (u16 f = 0; f < k_n; ++f) {
            const u8 vmax = vals[f];
            for (u16 v = 0; v < vmax; ++v) {
                slices[tgt]->set(f, SC_U8(v));
                snap[tgt][f] = SC_U8(v);
                if (slices[tgt]->get(f) != SC_U8(v)) {
                    std::printf("*** FAILED get slice %u field %u expect %u got %u\n",
                        SC_U32(tgt), SC_U32(f), SC_U32(v), SC_U32(slices[tgt]->get(f)));
                    return false;
                }
                for (u16 o = 0; o < k_slices; ++o) {
                    if (o == tgt) {
                        continue;
                    }
                    if (!snap_eq(*slices[o], snap[o])) {
                        std::printf("*** FAILED interference: writing slice %u field %u=%u touched slice %u\n",
                            SC_U32(tgt), SC_U32(f), SC_U32(v), SC_U32(o));
                        return false;
                    }
                }
                for (u16 of = 0; of < k_n; ++of) {
                    if (of == f) {
                        continue;
                    }
                    if (slices[tgt]->get(of) != snap[tgt][of]) {
                        std::printf("*** FAILED intra-slice side field %u on slice %u\n",
                            SC_U32(of), SC_U32(tgt));
                        return false;
                    }
                }
            }
            const u8 restore = pattern_val(tgt, f, vals[f]);
            slices[tgt]->set(f, restore);
            snap[tgt][f] = restore;
        }
    }
    std::printf("*** PASSED interference (%u slices, full sweeps, no cross-slice writes)\n",
        SC_U32(k_slices));
    return true;
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
    GeneralBitBank* bank = City::create_bank(lay.m_bits);
    if (bank == nullptr) {
        std::printf("*** FAILED bank create\n");
        return 1;
    }
    DynamicBitField* slices[k_slices];
    for (u16 s = 0; s < k_slices; ++s) {
        slices[s] = nullptr;
    }
    bool ok = true;
    for (u16 s = 0; s < k_slices; ++s) {
        slices[s] = new DynamicBitField(&lay, bank);
        if (slices[s] == nullptr || slices[s]->n() != k_n || slices[s]->ix() == U16_KEY_NULL) {
            std::printf("*** FAILED construct slice %u\n", SC_U32(s));
            ok = false;
            break;
        }
    }
    std::printf("DynamicBitField mk3 interference  seed %u  fields %u  slices %u  bits %u\n",
        seed, SC_U32(k_n), SC_U32(k_slices), SC_U32(lay.m_bits));
    if (ok) {
        for (u16 s = 0; s < k_slices; ++s) {
            std::printf("  slice %u -> batch ix %u\n", SC_U32(s), SC_U32(slices[s]->ix()));
        }
        ok = check_slice_isolation(slices, vals);
    }
    for (u16 s = 0; s < k_slices; ++s) {
        delete slices[s];
        slices[s] = nullptr;
    }
    City::destroy_bank(bank);
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
