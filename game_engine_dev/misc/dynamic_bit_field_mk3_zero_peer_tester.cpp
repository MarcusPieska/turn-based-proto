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

static bool all_zero (DynamicBitField& bf) {
    for (u16 f = 0; f < k_n; ++f) {
        if (bf.get(f) != 0) {
            return false;
        }
    }
    return true;
}

static bool peers_zero (DynamicBitField** slices, u16 tgt) {
    for (u16 s = 0; s < k_slices; ++s) {
        if (s == tgt) {
            continue;
        }
        if (!all_zero(*slices[s])) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - Sweep on one slice -
//================================================================================================================================

static bool sweep_one (DynamicBitField** slices, u16 tgt, const u8* vals) {
    DynamicBitField& bf = *slices[tgt];
    u8 snap[k_n];
    for (u16 i = 0; i < k_n; ++i) {
        bf.set(i, 0);
        snap[i] = 0;
    }
    if (!peers_zero(slices, tgt)) {
        std::printf("*** FAILED peers not zero after init\n");
        return false;
    }
    for (u16 f = 0; f < k_n; ++f) {
        const u8 vmax = vals[f];
        for (u16 v = 0; v < vmax; ++v) {
            const u8 uv = SC_U8(v);
            bf.set(f, uv);
            snap[f] = uv;
            if (bf.get(f) != uv) {
                std::printf("*** FAILED get slice %u field %u expect %u got %u\n",
                    SC_U32(tgt), SC_U32(f), SC_U32(uv), SC_U32(bf.get(f)));
                return false;
            }
            for (u16 o = 0; o < k_n; ++o) {
                if (o == f) {
                    continue;
                }
                if (bf.get(o) != snap[o]) {
                    std::printf("*** FAILED intra side field %u on slice %u\n", SC_U32(o), SC_U32(tgt));
                    return false;
                }
            }
            if (!peers_zero(slices, tgt)) {
                std::printf("*** FAILED peer non-zero writing slice %u field %u=%u\n",
                    SC_U32(tgt), SC_U32(f), SC_U32(uv));
                return false;
            }
        }
        bf.set(f, 0);
        snap[f] = 0;
    }
    if (!peers_zero(slices, tgt) || !all_zero(bf)) {
        std::printf("*** FAILED not all zero after sweep\n");
        return false;
    }
    std::printf("*** PASSED zero-peer sweep (tgt slice %u, %u peers stay zero)\n",
        SC_U32(tgt), SC_U32(k_slices - 1u));
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
        for (u16 f = 0; f < k_n; ++f) {
            slices[s]->set(f, 0);
        }
    }
    const u16 tgt = SC_U16(rnd() % k_slices);
    std::printf("DynamicBitField mk3 zero-peer  seed %u  fields %u  slices %u  tgt %u\n",
        seed, SC_U32(k_n), SC_U32(k_slices), SC_U32(tgt));
    if (ok) {
        ok = sweep_one(slices, tgt, vals);
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
