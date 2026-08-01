//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "building_enum.h"
#include "building_trait_attribution.h"
#include "civ_trait_affinity.h"
#include "civ_trait_enum.h"
#include "runtime_static_loader.h"
#include "tech_enum.h"
#include "tech_static_key.h"
#include "tech_trait_attribution.h"

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

struct RankRow {
    u16 m_idx;
    i32 m_score;
};

static int rank_cmp (const void* a, const void* b) {
    const RankRow* ra = static_cast<const RankRow*>(a);
    const RankRow* rb = static_cast<const RankRow*>(b);
    if (ra->m_score > rb->m_score) {
        return -1;
    }
    if (ra->m_score < rb->m_score) {
        return 1;
    }
    if (ra->m_idx < rb->m_idx) {
        return -1;
    }
    if (ra->m_idx > rb->m_idx) {
        return 1;
    }
    return 0;
}

static void print_by_trait () {
    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait trait = static_cast<CivTrait>(ti);
        const u16 count = TechTraitAttribution::count_for(trait);
        std::printf("\n%s: %u techs (from buildings)\n", trait_name(trait), static_cast<u32>(count));
        const u16 n = TechTraitAttribution::tech_n();
        for (u16 i = 0; i < n; ++i) {
            if (!TechTraitAttribution::has(i, trait)) {
                continue;
            }
            std::printf("  %3u %s\n", static_cast<u32>(i), tech_name(i));
        }
    }
}

static void print_ranked_for (CivTrait primary, const i32* scores, u16 n, u16 show) {
    RankRow* rows = new RankRow[n];
    u16 hit = 0;
    for (u16 i = 0; i < n; ++i) {
        if (scores[i] == 0) {
            continue;
        }
        rows[hit].m_idx = i;
        rows[hit].m_score = scores[i];
        hit = static_cast<u16>(hit + 1u);
    }
    std::qsort(rows, hit, sizeof(RankRow), rank_cmp);
    const u16 lim = (show < hit) ? show : hit;
    std::printf("\n%s tree (%u scored, top %u):\n", trait_name(primary), static_cast<u32>(hit), static_cast<u32>(lim));
    for (u16 i = 0; i < lim; ++i) {
        std::printf("  %2u. %3u  total=%5d  local=%4d  %s\n",
            static_cast<u32>(i),
            static_cast<u32>(rows[i].m_idx),
            static_cast<int>(rows[i].m_score),
            static_cast<int>(TechTraitAttribution::local_score(rows[i].m_idx, primary)),
            tech_name(rows[i].m_idx));
    }
    delete[] rows;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    note_result(ensure_statics(), "load statics");
    if (g_rt_statics == nullptr) {
        summarize_test_results();
        return 1;
    }

    note_result(TechTraitAttribution::begin(g_rt_statics->tech(), g_rt_statics->building()), "begin");
    note_result(TechTraitAttribution::ready(), "ready");
    const u16 n = TechTraitAttribution::tech_n();
    note_result(n == g_rt_statics->tech().get_item_count(), "tech_n matches");

    note_result(TechTraitAttribution::band_score(CivTrait::Agricultural, CivTrait::Agricultural)
        == TECH_TRAIT_SCORE_PRIMARY, "band primary");
    note_result(TechTraitAttribution::band_score(CivTrait::Agricultural,
        CivTraitAffinity::pref(CivTrait::Agricultural, 1)) == TECH_TRAIT_SCORE_FRIENDLY, "band friendly");
    note_result(TechTraitAttribution::band_score(CivTrait::Agricultural,
        CivTraitAffinity::pref(CivTrait::Agricultural, 3)) == TECH_TRAIT_SCORE_NEUTRAL, "band neutral");
    note_result(TechTraitAttribution::band_score(CivTrait::Agricultural,
        CivTraitAffinity::pref(CivTrait::Agricultural, 5)) == TECH_TRAIT_SCORE_HOSTILE, "band hostile");

    const u16 pottery = static_cast<u16>(Tech::Pottery);
    const u16 granary = static_cast<u16>(Building::Granary);
    note_result(BuildingTraitAttribution::has(granary, CivTrait::Agricultural), "Granary is Agricultural");
    note_result(TechTraitAttribution::has(pottery, CivTrait::Agricultural), "Pottery inherits Agricultural");
    note_result(TechTraitAttribution::local_score(pottery, CivTrait::Agricultural) >= TECH_TRAIT_SCORE_PRIMARY,
        "Pottery local >= primary for Agricultural");

    const u16 industrialization = static_cast<u16>(Tech::Industrialization);
    note_result(TechTraitAttribution::local_score(industrialization, CivTrait::Industrious) > 0,
        "Industrialization local > 0 for Industrious");

    print_by_trait();

    i32* scores = new i32[n];
    std::memset(scores, 0, sizeof(i32) * n);
    const u16 seed = static_cast<u16>(Tech::Future_Tech);
    TechTraitAttribution::propagate(seed, TECH_TRAIT_SCORE_PRIMARY, scores);
    note_result(scores[seed] == TECH_TRAIT_SCORE_PRIMARY, "propagate seed");

    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait primary = static_cast<CivTrait>(ti);
        TechTraitAttribution::score_tree(primary, scores);
        u16 nonzero = 0;
        i32 best = 0;
        for (u16 i = 0; i < n; ++i) {
            if (scores[i] != 0) {
                nonzero = static_cast<u16>(nonzero + 1u);
            }
            if (scores[i] > best) {
                best = scores[i];
            }
        }
        note_result(nonzero > 0, "score_tree has hits");
        note_result(best > 0, "score_tree best > 0");
        print_ranked_for(primary, scores, n, 15);
    }

    delete[] scores;
    TechTraitAttribution::clear();
    note_result(!TechTraitAttribution::ready(), "cleared");
    summarize_test_results();
    std::printf("TOTAL FAILS: %d / %d\n", total_test_fails, total_tests_run);
    return total_test_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
