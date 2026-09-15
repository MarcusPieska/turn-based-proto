//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "city_job_enum.h"
#include "city_job_static_key.h"
#include "city_job_trait_attribution.h"
#include "city_job_trait_orderings.h"
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

static cstr job_name (u16 idx) {
    return g_rt_statics->city_job().get_name(CityJobStaticDataKey::from_raw(idx));
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

static void test_orderings () {
    note_result(ensure_statics(), "load runtime statics");
    if (g_rt_statics == nullptr) {
        return;
    }
    note_result(CityJobTraitOrderings::begin(g_rt_statics->city_job(), g_rt_statics->trait_affinity_map()), "begin orderings");
    note_result(CityJobTraitOrderings::ready(), "orderings ready");
    const u16 n = CityJobTraitOrderings::job_n();
    note_result(n == g_rt_statics->city_job().get_item_count(), "job_n matches");

    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        u8 seen[64];
        std::memset(seen, 0, sizeof(seen));
        note_result(n <= 64, "job_n fits seen buf");
        u8 ok_perm = 1;
        for (u16 s = 0; s < n; ++s) {
            const u16 j = CityJobTraitOrderings::at(ti, s);
            if (j >= n || seen[j] != 0) {
                ok_perm = 0;
                break;
            }
            seen[j] = 1;
        }
        note_result(ok_perm != 0, "ordering is permutation");
    }

    note_result(CityJobTraitAttribution::has(static_cast<u16>(CityJob::Scientist), CivTrait::Scientific), "Scientist tagged Scientific");
    note_result(CityJobTraitAttribution::has(static_cast<u16>(CityJob::Priest), CivTrait::Religious), "Priest tagged Religious");
    note_result(CityJobTraitAttribution::has(static_cast<u16>(CityJob::Entertainer), CivTrait::Religious), "Entertainer tagged Religious");
    note_result(CityJobTraitAttribution::has(static_cast<u16>(CityJob::Policeman), CivTrait::Expansionist), "Policeman tagged Expansionist");
    note_result(CityJobTraitAttribution::has(static_cast<u16>(CityJob::Doctor), CivTrait::Agricultural), "Doctor tagged Agricultural");

    std::printf("\ncity job orderings:\n");
    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait trait = static_cast<CivTrait>(ti);
        std::printf("\n%s:\n", trait_name(trait));
        for (u16 s = 0; s < n; ++s) {
            const u16 j = CityJobTraitOrderings::at(ti, s);
            std::printf("  %2u. %2u %s (base=%u)\n",
                (unsigned)s, (unsigned)j, job_name(j),
                (unsigned)CityJobTraitAttribution::base(j));
        }
    }

    BitArrayCL avail(n);
    avail.set_bit(static_cast<u32>(CityJob::Scientist));
    avail.set_bit(static_cast<u32>(CityJob::Merchant));
    avail.set_bit(static_cast<u32>(CityJob::Technician));
    avail.set_bit(static_cast<u32>(CityJob::Priest));
    avail.set_bit(static_cast<u32>(CityJob::Doctor));

    const u16 sci = CityJobTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Scientific));
    const u16 com = CityJobTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Commercial));
    const u16 ind = CityJobTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Industrious));
    const u16 rel = CityJobTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Religious));
    const u16 ag = CityJobTraitOrderings::pick(avail, static_cast<u16>(CivTrait::Agricultural));

    std::printf("\npick from {Scientist,Merchant,Technician,Priest,Doctor}:\n");
    std::printf("  Scientific   -> %u %s\n", (unsigned)sci, (sci == U16_KEY_NULL) ? "-" : job_name(sci));
    std::printf("  Commercial   -> %u %s\n", (unsigned)com, (com == U16_KEY_NULL) ? "-" : job_name(com));
    std::printf("  Industrious  -> %u %s\n", (unsigned)ind, (ind == U16_KEY_NULL) ? "-" : job_name(ind));
    std::printf("  Religious    -> %u %s\n", (unsigned)rel, (rel == U16_KEY_NULL) ? "-" : job_name(rel));
    std::printf("  Agricultural -> %u %s\n", (unsigned)ag, (ag == U16_KEY_NULL) ? "-" : job_name(ag));

    note_result(sci == static_cast<u16>(CityJob::Scientist), "Scientific picks Scientist");
    note_result(com == static_cast<u16>(CityJob::Merchant), "Commercial picks Merchant");
    note_result(ind == static_cast<u16>(CityJob::Technician), "Industrious picks Technician");
    note_result(rel == static_cast<u16>(CityJob::Priest), "Religious picks Priest");
    note_result(ag == static_cast<u16>(CityJob::Doctor), "Agricultural picks Doctor");

    CityJobTraitOrderings::clear();
    CityJobTraitAttribution::clear();
    note_result(!CityJobTraitOrderings::ready(), "cleared");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    test_orderings();
    std::printf("=======================================================\n");
    std::printf(" TESTING CITY JOB TRAIT ORDERINGS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
