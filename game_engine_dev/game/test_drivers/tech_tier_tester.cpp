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
#include "tech_age_mng.h"
#include "tech_age_static_data.h"
#include "tech_age_static_key.h"
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

static cstr age_name (u16 age_idx) {
    cstr nm = g_rt_statics->tech_age().get_name(TechAgeStaticDataKey::from_raw(age_idx));
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

static void print_age_status (const TechAgeMng& mng) {
    static const char* k_red = "\033[31m";
    static const char* k_grn = "\033[32m";
    static const char* k_rst = "\033[0m";
    const u16 n = TechAgeMng::age_n();
    std::printf("--- age progress (unlock at %u%%, ceil)  head=%s ---\n",
        TechAgeMng::pct(), age_name(mng.head_age()));
    for (u16 a = 0; a < n; ++a) {
        const u8 cat = TechAgeMng::cat(a);
        if (cat == 0) {
            continue;
        }
        const u8 count = mng.done(a);
        const u32 need = (static_cast<u32>(cat) * TechAgeMng::pct() + 99u) / 100u;
        const char* nums_c = count < cat ? k_red : k_grn;
        const bool open = a <= mng.head_age();
        const char* lock_c = open ? k_grn : k_red;
        std::printf("  %-18s  %scat=%u  need=%u  count=%u%s  %s%s%s\n",
            age_name(a),
            nums_c,
            static_cast<u32>(cat),
            need,
            static_cast<u32>(count),
            k_rst,
            lock_c,
            open ? "UNLOCKED" : "locked",
            k_rst);
    }
    std::printf("\n");
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
    if (!TechAgeMng::setup(*g_rt_statics)) {
        std::printf("TechAgeMng setup failed\n");
        return 1;
    }
    const u16 n = g_rt_statics->tech().get_item_count();
    GAME_EXPECT(n > 0, "tech_tier_tester tech_n");
    const TechStaticDataStruct* items = &g_rt_statics->tech().get_item(TechStaticDataKey::from_raw(0));

    TechAgeMng ages;
    print_age_status(ages);

    BitArrayCL done(n);
    u16* wave = new u16[n];
    u32 wave_i = 1;
    for (;;) {
        u16 wn = 0;
        for (u16 t = 0; t < n; ++t) {
            if (done.get_bit(t) != 0) {
                continue;
            }
            if (!ages.is_available(t)) {
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
        std::printf("=== wave %u (%u techs) ===\n", wave_i, static_cast<u32>(wn));
        for (u16 i = 0; i < wn; ++i) {
            const u16 t = wave[i];
            const u16 age = items[t].tier;
            std::printf("  %-28s  cost=%u  age=%s\n", tech_name(t), items[t].cost, age_name(age));
            done.set_bit(t);
            ages.log_tech(t);
        }
        std::printf("\n");
        print_age_status(ages);
        wave_i = wave_i + 1u;
    }

    u16 left = 0;
    for (u16 t = 0; t < n; ++t) {
        if (done.get_bit(t) == 0) {
            left = static_cast<u16>(left + 1u);
        }
    }
    std::printf("=== walk done: researched=%u/%u  unreachable=%u  head=%s ===\n",
        static_cast<u32>(n - left), static_cast<u32>(n), static_cast<u32>(left), age_name(ages.head_age()));

    delete[] wave;
    TechAgeMng::clear();
    return left == 0 ? 0 : 1;
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
