//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "civ_trait_affinity.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

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

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

static void test_affinity () {
    for (u16 o = 0; o < CivTraitAffinity::k_n; ++o) {
        const CivTrait owner = static_cast<CivTrait>(o);
        note_result(CivTraitAffinity::row_is_permutation(owner), "row permutation");
        note_result(CivTraitAffinity::pref(owner, 0) == owner, "self preferred first");
        note_result(CivTraitAffinity::affinity(owner, owner) == static_cast<i8>(CivTraitAffinity::k_n - 1u), "self affinity max");
    }

    std::printf("\ntrait ring (circular):\n  ");
    for (u16 i = 0; i < CivTraitAffinity::k_n; ++i) {
        if (i != 0) {
            std::printf(" - ");
        }
        std::printf("%s", trait_name(CivTraitAffinity::ring_at(i)));
    }
    std::printf(" - (%s)\n", trait_name(CivTraitAffinity::ring_at(0)));

    std::printf("\npreference order (most -> least):\n");
    for (u16 o = 0; o < CivTraitAffinity::k_n; ++o) {
        const CivTrait owner = static_cast<CivTrait>(o);
        std::printf("  %-13s ", trait_name(owner));
        for (u16 r = 0; r < CivTraitAffinity::k_n; ++r) {
            if (r != 0) {
                std::printf(" > ");
            }
            std::printf("%s", trait_name(CivTraitAffinity::pref(owner, r)));
        }
        std::printf("\n");
    }

    std::printf("\naffinity matrix (row likes column):\n");
    const cstr short_nm[7] = { "Ag", "In", "Ex", "Re", "Mi", "Sc", "Co" };
    std::printf("              ");
    for (u16 t = 0; t < CivTraitAffinity::k_n; ++t) {
        std::printf(" %4s", short_nm[t]);
    }
    std::printf("\n");
    for (u16 o = 0; o < CivTraitAffinity::k_n; ++o) {
        const CivTrait owner = static_cast<CivTrait>(o);
        std::printf("  %-12s", trait_name(owner));
        for (u16 t = 0; t < CivTraitAffinity::k_n; ++t) {
            std::printf(" %4d", (int)CivTraitAffinity::affinity(owner, static_cast<CivTrait>(t)));
        }
        std::printf("\n");
    }

    i32 scores[CivTraitAffinity::k_n];
    CivTraitAffinity::received_scores(scores);
    i32 mn = scores[0];
    i32 mx = scores[0];
    u16 last_n = 0;
    std::printf("\nreceived affinity totals (column sums):\n");
    for (u16 t = 0; t < CivTraitAffinity::k_n; ++t) {
        const CivTrait trait = static_cast<CivTrait>(t);
        std::printf("  %-13s %d\n", trait_name(trait), (int)scores[t]);
        if (scores[t] < mn) {
            mn = scores[t];
        }
        if (scores[t] > mx) {
            mx = scores[t];
        }
        u16 as_last = 0;
        for (u16 o = 0; o < CivTraitAffinity::k_n; ++o) {
            if (CivTraitAffinity::rank_of(static_cast<CivTrait>(o), trait) == static_cast<u16>(CivTraitAffinity::k_n - 1u)) {
                as_last = static_cast<u16>(as_last + 1u);
            }
        }
        if (as_last == CivTraitAffinity::k_n) {
            last_n = static_cast<u16>(last_n + 1u);
            std::printf("    WARNING: hated by all (always last)\n");
        }
    }
    std::printf("  spread (max-min): %d\n", (int)(mx - mn));
    note_result(mx - mn <= 6, "received scores reasonably balanced (spread<=6)");
    note_result(last_n == 0, "no trait hated by all");

    summarize_test_results();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    test_affinity();
    if (total_test_fails > 0) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
