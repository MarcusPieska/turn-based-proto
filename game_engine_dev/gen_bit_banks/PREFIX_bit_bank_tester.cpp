//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "[MEMBER_TAG]_bit_bank.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;
typedef std::string str;
typedef uint32_t u32;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

void note_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        if (print_level > 0) {
            printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void note_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_result (cond, msg.c_str());
}

void summarize_test_results () {
    if (print_level > 0) {
        printf("--------------------------------\n");
        printf(" Test count: %d\n", test_count);
        printf(" Test pass: %d\n", test_pass);
        printf(" Test fail: %d\n", test_count - test_pass);
        printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

class [USER_CLASS_NAME] {
public:
    static [CLASS_NAME_PREFIX]BitBank* create_bank (u16 batch_size) {
        return new [CLASS_NAME_PREFIX]BitBank(batch_size);
    }

    static void destroy_bank ([CLASS_NAME_PREFIX]BitBank* bank) {
        delete bank;
    }

    static u16 claim_batch ([CLASS_NAME_PREFIX]BitBank& bank) {
        return bank.claim_batch();
    }

    static bool is_flagged (const [CLASS_NAME_PREFIX]BitBank& bank, u16 batch_idx, u16 flag_idx) {
        return bank.is_flagged(batch_idx, flag_idx);
    }

    static void set_flag ([CLASS_NAME_PREFIX]BitBank& bank, u16 batch_idx, u16 flag_idx) {
        bank.set_flag(batch_idx, flag_idx);
    }

    static void clear_flag ([CLASS_NAME_PREFIX]BitBank& bank, u16 batch_idx, u16 flag_idx) {
        bank.clear_flag(batch_idx, flag_idx);
    }

    static u16 get_claimed_batch_count (const [CLASS_NAME_PREFIX]BitBank& bank) {
        return bank.m_claimed_batch_count;
    }

    static u8 get_allocated_page_count (const [CLASS_NAME_PREFIX]BitBank& bank) {
        return bank.m_allocated_page_count;
    }

    static u8* get_page ([CLASS_NAME_PREFIX]BitBank& bank, u16 page_idx) {
        return bank.m_pages[page_idx];
    }
};

void assert_batch_is_clear (
    [CLASS_NAME_PREFIX]BitBank& bank,
    u16 batch_idx,
    u16 batch_size,
    cstr prefix
) {
    for (u16 flag_idx = 0; flag_idx < batch_size; ++flag_idx) {
        bool flagged = [USER_CLASS_NAME]::is_flagged(bank, batch_idx, flag_idx);
        str msg = str(prefix) + " - batch " + std::to_string(batch_idx)
                + " flag " + std::to_string(flag_idx) + " is clear";
        note_result(!flagged, msg.c_str());
    }
}

void test_constructed_bank_has_zero_counts_and_null_pages () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(8);
    note_result([USER_CLASS_NAME]::get_claimed_batch_count(*bank) == 0, "Starts with zero claimed batches");
    note_result([USER_CLASS_NAME]::get_allocated_page_count(*bank) == 0, "Starts with zero allocated pages");
    note_result([USER_CLASS_NAME]::get_page(*bank, 0) == nullptr, "Page 0 is null at start");
    note_result([USER_CLASS_NAME]::get_page(*bank, 1) == nullptr, "Page 1 is null at start");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_first_claim_returns_zero_and_allocates_page_zero () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(8);
    u16 batch_idx = [USER_CLASS_NAME]::claim_batch(*bank);
    note_result(batch_idx == 0, "First claim returns batch 0");
    note_result([USER_CLASS_NAME]::get_claimed_batch_count(*bank) == 1, "Claimed batch count is 1 after first claim");
    note_result([USER_CLASS_NAME]::get_allocated_page_count(*bank) == 1, "One page allocated on first claim");
    note_result([USER_CLASS_NAME]::get_page(*bank, 0) != nullptr, "Page 0 allocated after first claim");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_sequential_claims_return_sequential_indices () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(8);
    for (u16 i = 0; i < 16; ++i) {
        u16 batch_idx = [USER_CLASS_NAME]::claim_batch(*bank);
        str msg = str("Claim returns sequential index ") + std::to_string(i);
        note_result(batch_idx == i, msg.c_str());
    }
    note_result([USER_CLASS_NAME]::get_claimed_batch_count(*bank) == 16, "Claimed batch count matches claim calls");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_claims_within_first_page_do_not_allocate_new_pages () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(4);
    for (u16 i = 0; i < 200; ++i) {
        [USER_CLASS_NAME]::claim_batch(*bank);
    }
    note_result([USER_CLASS_NAME]::get_allocated_page_count(*bank) == 1, "Under 256 claims remains on one page");
    note_result([USER_CLASS_NAME]::get_page(*bank, 0) != nullptr, "Page 0 still present");
    note_result([USER_CLASS_NAME]::get_page(*bank, 1) == nullptr, "Page 1 not allocated yet");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_claim_256th_is_last_batch_in_page_zero () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(4);
    u16 last_idx = 0;
    for (u16 i = 0; i < 256; ++i) {
        last_idx = [USER_CLASS_NAME]::claim_batch(*bank);
    }
    note_result(last_idx == 255, "256th claim returns batch 255");
    note_result([USER_CLASS_NAME]::get_allocated_page_count(*bank) == 1, "Still one allocated page after 256 claims");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_claim_257th_allocates_second_page () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(4);
    for (u16 i = 0; i < 256; ++i) {
        [USER_CLASS_NAME]::claim_batch(*bank);
    }
    u16 idx = [USER_CLASS_NAME]::claim_batch(*bank);
    note_result(idx == 256, "257th claim returns batch 256");
    note_result([USER_CLASS_NAME]::get_allocated_page_count(*bank) == 2, "Second page allocated at boundary crossing");
    note_result([USER_CLASS_NAME]::get_page(*bank, 1) != nullptr, "Page 1 allocated");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_newly_claimed_batches_start_clear () {
    const u16 batch_size = 6;
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(batch_size);
    u16 b0 = [USER_CLASS_NAME]::claim_batch(*bank);
    u16 b1 = [USER_CLASS_NAME]::claim_batch(*bank);
    assert_batch_is_clear(*bank, b0, batch_size, "First claimed batch starts clear");
    assert_batch_is_clear(*bank, b1, batch_size, "Second claimed batch starts clear");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_set_flag_round_trip () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(10);
    u16 b = [USER_CLASS_NAME]::claim_batch(*bank);
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, b, 3), "Flag starts clear before set");
    [USER_CLASS_NAME]::set_flag(*bank, b, 3);
    note_result([USER_CLASS_NAME]::is_flagged(*bank, b, 3), "Flag reads set after set_flag");
    [USER_CLASS_NAME]::clear_flag(*bank, b, 3);
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, b, 3), "Flag reads clear after clear_flag");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_flags_are_independent_within_same_batch () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(12);
    u16 b = [USER_CLASS_NAME]::claim_batch(*bank);
    [USER_CLASS_NAME]::set_flag(*bank, b, 1);
    [USER_CLASS_NAME]::set_flag(*bank, b, 7);
    note_result([USER_CLASS_NAME]::is_flagged(*bank, b, 1), "Flag 1 set");
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, b, 2), "Flag 2 remains clear");
    note_result([USER_CLASS_NAME]::is_flagged(*bank, b, 7), "Flag 7 set");
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, b, 11), "Flag 11 remains clear");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_flags_are_independent_across_batches_on_same_page () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(8);
    u16 b0 = [USER_CLASS_NAME]::claim_batch(*bank);
    u16 b1 = [USER_CLASS_NAME]::claim_batch(*bank);
    [USER_CLASS_NAME]::set_flag(*bank, b0, 4);
    note_result([USER_CLASS_NAME]::is_flagged(*bank, b0, 4), "Set flag visible on first batch");
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, b1, 4), "Same flag index on second batch remains clear");
    [USER_CLASS_NAME]::set_flag(*bank, b1, 2);
    note_result([USER_CLASS_NAME]::is_flagged(*bank, b1, 2), "Second batch can set its own flag");
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, b0, 2), "Second batch flag does not leak to first batch");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_flags_are_independent_across_pages () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(5);
    for (u16 i = 0; i < 257; ++i) {
        [USER_CLASS_NAME]::claim_batch(*bank);
    }
    [USER_CLASS_NAME]::set_flag(*bank, 255, 3);
    note_result([USER_CLASS_NAME]::is_flagged(*bank, 255, 3), "Last batch on page 0 holds set flag");
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, 256, 3), "First batch on page 1 unaffected");
    [USER_CLASS_NAME]::set_flag(*bank, 256, 1);
    note_result([USER_CLASS_NAME]::is_flagged(*bank, 256, 1), "Page 1 batch flag can be set");
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, 255, 1), "Page 1 set does not leak to page 0");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_highest_flag_index_round_trip () {
    const u16 batch_size = 71;
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(batch_size);
    u16 b = [USER_CLASS_NAME]::claim_batch(*bank);
    const u16 max_flag_idx = static_cast<u16>(batch_size - 1);
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, b, max_flag_idx), "Highest flag index starts clear");
    [USER_CLASS_NAME]::set_flag(*bank, b, max_flag_idx);
    note_result([USER_CLASS_NAME]::is_flagged(*bank, b, max_flag_idx), "Highest flag index can be set");
    [USER_CLASS_NAME]::clear_flag(*bank, b, max_flag_idx);
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, b, max_flag_idx), "Highest flag index can be cleared");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_claim_count_matches_number_of_claims_across_pages () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(3);
    const u16 claim_total = 700;
    for (u16 i = 0; i < claim_total; ++i) {
        [USER_CLASS_NAME]::claim_batch(*bank);
    }
    note_result([USER_CLASS_NAME]::get_claimed_batch_count(*bank) == claim_total, "Claimed count matches 700 claims");
    note_result([USER_CLASS_NAME]::get_allocated_page_count(*bank) == 3, "700 claims allocate exactly 3 pages");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_clear_does_not_change_other_batches () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(9);
    u16 b0 = [USER_CLASS_NAME]::claim_batch(*bank);
    u16 b1 = [USER_CLASS_NAME]::claim_batch(*bank);
    [USER_CLASS_NAME]::set_flag(*bank, b0, 5);
    [USER_CLASS_NAME]::set_flag(*bank, b1, 5);
    [USER_CLASS_NAME]::clear_flag(*bank, b0, 5);
    note_result(![USER_CLASS_NAME]::is_flagged(*bank, b0, 5), "Cleared flag on first batch");
    note_result([USER_CLASS_NAME]::is_flagged(*bank, b1, 5), "Second batch flag remains set");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

void test_second_page_first_batch_starts_clear_after_boundary_claim () {
    [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(7);
    for (u16 i = 0; i < 257; ++i) {
        [USER_CLASS_NAME]::claim_batch(*bank);
    }
    assert_batch_is_clear(*bank, 256, 7, "Batch 256 starts clear on newly allocated page");
    [USER_CLASS_NAME]::destroy_bank(bank);
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_constructed_bank_has_zero_counts_and_null_pages();
    test_first_claim_returns_zero_and_allocates_page_zero();
    test_sequential_claims_return_sequential_indices();
    test_claims_within_first_page_do_not_allocate_new_pages();
    test_claim_256th_is_last_batch_in_page_zero();
    test_claim_257th_allocates_second_page();
    test_newly_claimed_batches_start_clear();
    test_set_flag_round_trip();
    test_flags_are_independent_within_same_batch();
    test_flags_are_independent_across_batches_on_same_page();
    test_flags_are_independent_across_pages();
    test_highest_flag_index_round_trip();
    test_claim_count_matches_number_of_claims_across_pages();
    test_clear_does_not_change_other_batches();
    test_second_page_first_batch_starts_clear_after_boundary_claim();

    printf("=======================================================\n");
    printf(" TESTING [MACRO_PREFIX] BIT BANK: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
