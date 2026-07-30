//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "building_enum.h"
#include "building_static_key.h"
#include "building_trait_attribution.h"
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

static void print_by_trait () {
    const u16 n = BuildingTraitAttribution::building_n();
    for (u16 ti = 0; ti < 7; ++ti) {
        const CivTrait trait = static_cast<CivTrait>(ti);
        const u16 count = BuildingTraitAttribution::count_for(trait);
        std::printf("\n%s: %u buildings\n", trait_name(trait), (unsigned)count);
        for (u16 i = 0; i < n; ++i) {
            if (!BuildingTraitAttribution::has(i, trait)) {
                continue;
            }
            std::printf("  %3u %s\n", (unsigned)i, g_rt_statics->building().get_name(BuildingStaticDataKey::from_raw(i)));
        }
    }
    u16 unclaimed = 0;
    for (u16 i = 0; i < n; ++i) {
        if (BuildingTraitAttribution::mask(i) == 0) {
            unclaimed = static_cast<u16>(unclaimed + 1u);
        }
    }
    std::printf("\nUnclaimed: %u buildings\n", (unsigned)unclaimed);
    for (u16 i = 0; i < n; ++i) {
        if (BuildingTraitAttribution::mask(i) != 0) {
            continue;
        }
        std::printf("  %3u %s\n", (unsigned)i, g_rt_statics->building().get_name(BuildingStaticDataKey::from_raw(i)));
    }
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

static void test_attribution () {
    note_result(ensure_statics(), "load runtime statics");
    if (g_rt_statics == nullptr) {
        return;
    }
    note_result(BuildingTraitAttribution::begin(g_rt_statics->building()), "begin attribution");
    note_result(BuildingTraitAttribution::ready(), "attribution ready");
    note_result(BuildingTraitAttribution::building_n() == g_rt_statics->building().get_item_count(), "building_n matches catalog");

    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Marketplace), CivTrait::Commercial), "Marketplace Commercial");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Bank), CivTrait::Commercial), "Bank Commercial");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Granary), CivTrait::Agricultural), "Granary Agricultural");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Factory), CivTrait::Industrious), "Factory Industrious");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Library), CivTrait::Scientific), "Library Scientific");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Library), CivTrait::Religious), "Library Religious (culture)");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Barracks), CivTrait::Militaristic), "Barracks Militaristic");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Courthouse), CivTrait::Expansionist), "Courthouse Expansionist");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Temple), CivTrait::Religious), "Temple Religious");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Harbor), CivTrait::Commercial), "Harbor Commercial (sea trade multi-tag)");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Commercial_Dock), CivTrait::Commercial), "Commercial Dock Commercial");
    note_result(BuildingTraitAttribution::has(static_cast<u16>(Building::Commercial_Dock), CivTrait::Expansionist), "Commercial Dock Expansionist");

    print_by_trait();

    BuildingTraitAttribution::clear();
    note_result(!BuildingTraitAttribution::ready(), "cleared");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    test_attribution();
    if (total_test_fails > 0) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
