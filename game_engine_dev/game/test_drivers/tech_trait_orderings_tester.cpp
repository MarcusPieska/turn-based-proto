//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "civ_trait_affinity.h"
#include "civ_trait_enum.h"
#include "runtime_static_loader.h"
#include "tech_enum.h"
#include "tech_static_key.h"
#include "tech_trait_attribution.h"
#include "tech_trait_orderings.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void note_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        std::printf("*** TEST FAILED: %s\n", msg);
    }
}

static void summarize_test_results () {
    std::printf("--------------------------------\n");
    std::printf(" Test count: %d\n", test_count);
    std::printf(" Test pass: %d\n", test_pass);
    std::printf(" Test fail: %d\n", test_count - test_pass);
    std::printf("--------------------------------\n");
    test_count = 0;
    test_pass = 0;
}

static bool ensure_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

static cstr trait_name (CivTrait t) {
    switch (t) {
    case CivTrait::Agricultural: return "Agricultural";
    case CivTrait::Industrious: return "Industrious";
    case CivTrait::Expansionist: return "Expansionist";
    case CivTrait::Religious: return "Religious";
    case CivTrait::Militaristic: return "Militaristic";
    case CivTrait::Scientific: return "Scientific";
    case CivTrait::Commercial: return "Commercial";
    default: return "?";
    }
}

static cstr tech_name (u16 idx) {
    cstr nm = g_rt_statics->tech().get_name(TechStaticDataKey::from_raw(idx));
    return nm != nullptr ? nm : "?";
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

static void test_orderings () {
    note_result(ensure_statics(), "load runtime statics");
    if (g_rt_statics == nullptr) {
        return;
    }
    note_result(TechTraitOrderings::begin(g_rt_statics->tech(), g_rt_statics->building()), "begin orderings");
    note_result(TechTraitOrderings::ready(), "orderings ready");
    const u16 n = TechTraitOrderings::tech_n();
    note_result(n == g_rt_statics->tech().get_item_count(), "tech_n matches");

    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        u8* seen = new u8[n];
        std::memset(seen, 0, static_cast<size_t>(n));
        u8 ok_perm = 1;
        for (u16 s = 0; s < n; ++s) {
            const u16 t = TechTraitOrderings::at(ti, s);
            if (t >= n || seen[t] != 0) {
                ok_perm = 0;
                break;
            }
            seen[t] = 1;
        }
        note_result(ok_perm != 0, "ordering is permutation");
        delete[] seen;
    }

    std::printf("\ntech orderings (first 12):\n");
    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait trait = static_cast<CivTrait>(ti);
        std::printf("\n%s:\n", trait_name(trait));
        const u16 show = (n < 12u) ? n : 12u;
        for (u16 s = 0; s < show; ++s) {
            const u16 t = TechTraitOrderings::at(ti, s);
            std::printf("  %2u. %3u %s\n", static_cast<u32>(s), static_cast<u32>(t), tech_name(t));
        }
    }

    BitArrayCL avail(n);
    avail.set_bit(static_cast<u32>(Tech::Pottery));
    avail.set_bit(static_cast<u32>(Tech::Industrialization));
    avail.set_bit(static_cast<u32>(Tech::Currency));
    avail.set_bit(static_cast<u32>(Tech::Warrior_Code));
    avail.set_bit(static_cast<u32>(Tech::Writing));
    avail.set_bit(static_cast<u32>(Tech::Code_of_Laws));

    const u16 ag = TechTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Agricultural));
    const u16 ind = TechTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Industrious));
    const u16 com = TechTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Commercial));
    const u16 mil = TechTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Militaristic));
    const u16 sci = TechTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Scientific));
    const u16 exp = TechTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Expansionist));
    const u16 rel = TechTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Religious));

    std::printf("\npick from {Pottery,Industrialization,Currency,Warrior Code,Writing,Code of Laws}:\n");
    std::printf("  Agricultural -> %u %s\n", static_cast<u32>(ag), (ag == U16_KEY_NULL) ? "-" : tech_name(ag));
    std::printf("  Industrious  -> %u %s\n", static_cast<u32>(ind), (ind == U16_KEY_NULL) ? "-" : tech_name(ind));
    std::printf("  Commercial   -> %u %s\n", static_cast<u32>(com), (com == U16_KEY_NULL) ? "-" : tech_name(com));
    std::printf("  Militaristic -> %u %s\n", static_cast<u32>(mil), (mil == U16_KEY_NULL) ? "-" : tech_name(mil));
    std::printf("  Scientific   -> %u %s\n", static_cast<u32>(sci), (sci == U16_KEY_NULL) ? "-" : tech_name(sci));
    std::printf("  Expansionist -> %u %s\n", static_cast<u32>(exp), (exp == U16_KEY_NULL) ? "-" : tech_name(exp));
    std::printf("  Religious    -> %u %s\n", static_cast<u32>(rel), (rel == U16_KEY_NULL) ? "-" : tech_name(rel));

    note_result(ag != U16_KEY_NULL, "Agricultural pick non-null");
    note_result(ind != U16_KEY_NULL, "Industrious pick non-null");
    note_result(com != U16_KEY_NULL, "Commercial pick non-null");
    note_result(mil != U16_KEY_NULL, "Militaristic pick non-null");
    note_result(sci == static_cast<u16>(Tech::Industrialization), "Scientific picks Industrialization among avail");

    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        u16 expect = U16_KEY_NULL;
        for (u16 s = 0; s < n; ++s) {
            const u16 t = TechTraitOrderings::at(ti, s);
            if (static_cast<u32>(t) < avail.get_count() && avail.get_bit(t) != 0) {
                expect = t;
                break;
            }
        }
        const u16 got = TechTraitOrderings::pick(avail, ti);
        note_result(got == expect, "pick matches first available in order");
    }

    BitArrayCL empty(n);
    note_result(TechTraitOrderings::pick(empty, static_cast<u16>(CivTrait::Agricultural)) == U16_KEY_NULL,
        "empty available -> null");

    i32* scores = new i32[n];
    TechTraitAttribution::score_tree(CivTrait::Agricultural, scores);
    const u16 top = TechTraitOrderings::at(static_cast<u16>(CivTrait::Agricultural), 0);
    i32 best = scores[0];
    for (u16 i = 1; i < n; ++i) {
        if (scores[i] > best) {
            best = scores[i];
        }
    }
    note_result(scores[top] == best, "Agricultural slot0 is max score");
    delete[] scores;

    TechTraitOrderings::clear();
    TechTraitAttribution::clear();
    note_result(!TechTraitOrderings::ready(), "cleared");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    test_orderings();
    if (total_test_fails > 0) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
