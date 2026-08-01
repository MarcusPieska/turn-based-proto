//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "assert_log.h"
#include "bit_array.h"
#include "item_reqs.h"
#include "runtime_static_loader.h"
#include "tech_static_data.h"
#include "tech_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool ensure_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return g_rt_statics != nullptr;
}

static cstr tech_name (u16 idx) {
    cstr nm = g_rt_statics->tech().get_name(TechStaticDataKey::from_raw(idx));
    return nm != nullptr ? nm : "?";
}

static bool tech_prereqs_met (u16 tech_idx, const TechStaticDataStruct* items, const BitArrayCL& done) {
    const ItemReqsStruct& reqs = items[tech_idx].reqs;
    for (u8 i = 0; i < MAX_PREREQ_COUNT; ++i) {
        if (reqs.types[i] != ITEM_REQ_TYPE_TECH) {
            continue;
        }
        if (done.get_bit(reqs.indices[i]) == 0) {
            return false;
        }
    }
    return true;
}

static int u16_cost_cmp (const void* a, const void* b) {
    const u16* ia = static_cast<const u16*>(a);
    const u16* ib = static_cast<const u16*>(b);
    const u32 ca = g_rt_statics->tech().get_item(TechStaticDataKey::from_raw(*ia)).cost;
    const u32 cb = g_rt_statics->tech().get_item(TechStaticDataKey::from_raw(*ib)).cost;
    if (ca < cb) {
        return -1;
    }
    if (ca > cb) {
        return 1;
    }
    if (*ia < *ib) {
        return -1;
    }
    if (*ia > *ib) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - TechTierTester -
//================================================================================================================================

class TechTierTester {
public:
    TechTierTester () = delete;

    static int run ();
};

int TechTierTester::run () {
    if (!ensure_statics()) {
        std::printf("statics failed\n");
        return 1;
    }
    const u16 n = g_rt_statics->tech().get_item_count();
    GAME_EXPECT(n > 0, "tech_tier_tester tech_n");
    const TechStaticDataStruct* items = &g_rt_statics->tech().get_item(TechStaticDataKey::from_raw(0));

    BitArrayCL done(n);
    u16* wave = new u16[n];
    u32 tier = 1;
    for (;;) {
        u16 wn = 0;
        for (u16 t = 0; t < n; ++t) {
            if (done.get_bit(t) != 0) {
                continue;
            }
            if (!tech_prereqs_met(t, items, done)) {
                continue;
            }
            wave[wn] = t;
            wn = static_cast<u16>(wn + 1u);
        }
        if (wn == 0) {
            break;
        }
        std::qsort(wave, wn, sizeof(u16), u16_cost_cmp);
        std::printf("=== tier %u (%u techs) ===\n", tier, static_cast<u32>(wn));
        for (u16 i = 0; i < wn; ++i) {
            const u16 t = wave[i];
            std::printf("  %-28s  cost=%u\n", tech_name(t), items[t].cost);
            done.set_bit(t);
        }
        std::printf("\n");
        tier = tier + 1u;
    }

    delete[] wave;
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    return TechTierTester::run();
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
