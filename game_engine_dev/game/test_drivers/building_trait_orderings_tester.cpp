//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "building_enum.h"
#include "building_static_key.h"
#include "building_trait_attribution.h"
#include "building_trait_orderings.h"
#include "civ_trait_affinity.h"
#include "civ_trait_enum.h"
#include "runtime_static_loader.h"

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

static cstr bld_name (u16 idx) {
    return g_rt_statics->building().get_name(BuildingStaticDataKey::from_raw(idx));
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

static void test_orderings () {
    note_result(ensure_statics(), "load runtime statics");
    if (g_rt_statics == nullptr) {
        return;
    }
    note_result(BuildingTraitOrderings::begin(g_rt_statics->building()), "begin orderings");
    note_result(BuildingTraitOrderings::ready(), "orderings ready");
    const u16 n = BuildingTraitOrderings::building_n();
    note_result(n == g_rt_statics->building().get_item_count(), "building_n matches");

    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        u8 seen[256];
        std::memset(seen, 0, sizeof(seen));
        note_result(n <= 256, "building_n fits seen buf");
        u8 ok_perm = 1;
        for (u16 s = 0; s < n; ++s) {
            const u16 b = BuildingTraitOrderings::at(ti, s);
            if (b >= n || seen[b] != 0) {
                ok_perm = 0;
                break;
            }
            seen[b] = 1;
        }
        note_result(ok_perm != 0, "ordering is permutation");
    }

    std::printf("\nbuilding orderings (first 12):\n");
    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait trait = static_cast<CivTrait>(ti);
        std::printf("\n%s:\n", trait_name(trait));
        const u16 show = (n < 12u) ? n : 12u;
        for (u16 s = 0; s < show; ++s) {
            const u16 b = BuildingTraitOrderings::at(ti, s);
            std::printf("  %2u. %3u %s\n", (unsigned)s, (unsigned)b, bld_name(b));
        }
    }

    BitArrayCL avail(n);
    avail.set_bit(static_cast<u32>(Building::Marketplace));
    avail.set_bit(static_cast<u32>(Building::Factory));
    avail.set_bit(static_cast<u32>(Building::Library));
    avail.set_bit(static_cast<u32>(Building::Barracks));
    avail.set_bit(static_cast<u32>(Building::Granary));

    const u16 com = BuildingTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Commercial));
    const u16 ind = BuildingTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Industrious));
    const u16 sci = BuildingTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Scientific));
    const u16 mil = BuildingTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Militaristic));
    const u16 ag = BuildingTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Agricultural));

    std::printf("\npick from {Marketplace,Factory,Library,Barracks,Granary}:\n");
    std::printf("  Commercial   -> %u %s\n", (unsigned)com, (com == U16_KEY_NULL) ? "-" : bld_name(com));
    std::printf("  Industrious  -> %u %s\n", (unsigned)ind, (ind == U16_KEY_NULL) ? "-" : bld_name(ind));
    std::printf("  Scientific   -> %u %s\n", (unsigned)sci, (sci == U16_KEY_NULL) ? "-" : bld_name(sci));
    std::printf("  Militaristic -> %u %s\n", (unsigned)mil, (mil == U16_KEY_NULL) ? "-" : bld_name(mil));
    std::printf("  Agricultural -> %u %s\n", (unsigned)ag, (ag == U16_KEY_NULL) ? "-" : bld_name(ag));

    note_result(com == static_cast<u16>(Building::Marketplace), "Commercial picks Marketplace");
    note_result(ind == static_cast<u16>(Building::Factory), "Industrious picks Factory");
    note_result(mil == static_cast<u16>(Building::Barracks), "Militaristic picks Barracks");
    note_result(ag == static_cast<u16>(Building::Granary), "Agricultural picks Granary");

    BuildingTraitOrderings::clear();
    BuildingTraitAttribution::clear();
    note_result(!BuildingTraitOrderings::ready(), "cleared");
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
