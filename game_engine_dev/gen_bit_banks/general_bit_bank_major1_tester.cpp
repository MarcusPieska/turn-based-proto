//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "general_bit_bank.h"

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

class City {
public:
    static GeneralBitBank* create_bank (u16 batch_size) {
        return new GeneralBitBank(batch_size);
    }

    static void destroy_bank (GeneralBitBank* bank) {
        delete bank;
    }

    static u16 claim_batch (GeneralBitBank& bank) {
        return bank.claim_batch();
    }

    static bool is_flagged (const GeneralBitBank& bank, u16 batch_idx, u16 flag_idx) {
        return bank.is_flagged(batch_idx, flag_idx);
    }

    static void set_flag (GeneralBitBank& bank, u16 batch_idx, u16 flag_idx) {
        bank.set_flag(batch_idx, flag_idx);
    }

    static void clear_flag (GeneralBitBank& bank, u16 batch_idx, u16 flag_idx) {
        bank.clear_flag(batch_idx, flag_idx);
    }

    static u16 get_claimed_batch_count (const GeneralBitBank& bank) {
        return bank.m_claimed_batch_count;
    }

    static u8 get_allocated_page_count (const GeneralBitBank& bank) {
        return bank.m_allocated_page_count;
    }

    static u8* get_page (GeneralBitBank& bank, u16 page_idx) {
        return bank.m_pages[page_idx];
    }
};

void set_all_flags_for_batch (GeneralBitBank& bank, u16 batch_idx, u16 batch_size, bool value) {
    for (u16 flag_idx = 0; flag_idx < batch_size; ++flag_idx) {
        if (value) {
            City::set_flag(bank, batch_idx, flag_idx);
        } else {
            City::clear_flag(bank, batch_idx, flag_idx);
        }
    }
}

void test_claim_2500_batches_without_crash_11_flags_per_batch () {
    const u16 batch_size = 11;
    const u16 batch_count = 2500;
    GeneralBitBank* bank = City::create_bank(batch_size);

    u16 last_batch_idx = 0;
    for (u16 i = 0; i < batch_count; ++i) {
        last_batch_idx = City::claim_batch(*bank);
    }

    note_result(last_batch_idx == static_cast<u16>(batch_count - 1), "2500th claim returns batch index 2499");
    note_result(City::get_claimed_batch_count(*bank) == batch_count, "Claimed batch count is 2500");
    note_result(City::get_allocated_page_count(*bank) == 10, "2500 batches allocate 10 pages");
    note_result(City::get_page(*bank, 0) != nullptr, "Page 0 allocated");
    note_result(City::get_page(*bank, 3) != nullptr, "Page 3 allocated");

    City::destroy_bank(bank);
    summarize_test_results();
}

void test_each_batch_isolation_over_2500_batches_with_11_flags () {
    const u16 batch_size = 11;
    const u16 batch_count = 2500;
    GeneralBitBank* bank = City::create_bank(batch_size);

    for (u16 i = 0; i < batch_count; ++i) {
        City::claim_batch(*bank);
    }

    for (u16 active_batch = 0; active_batch < batch_count; ++active_batch) {
        set_all_flags_for_batch(*bank, active_batch, batch_size, true);

        for (u16 check_batch = 0; check_batch < batch_count; ++check_batch) {
            for (u16 flag_idx = 0; flag_idx < batch_size; ++flag_idx) {
                bool expected = (check_batch == active_batch);
                bool actual = City::is_flagged(*bank, check_batch, flag_idx);
                if (actual != expected) {
                    str msg = str("Isolation mismatch: active batch ")
                            + std::to_string(active_batch)
                            + ", checked batch " + std::to_string(check_batch)
                            + ", flag " + std::to_string(flag_idx);
                    note_result(false, msg.c_str());
                    break;
                }
            }
        }

        set_all_flags_for_batch(*bank, active_batch, batch_size, false);
    }

    note_result(total_test_fails == 0, "Batch isolation holds across all 2500 batches");

    City::destroy_bank(bank);
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_claim_2500_batches_without_crash_11_flags_per_batch();
    test_each_batch_isolation_over_2500_batches_with_11_flags();

    printf("=======================================================\n");
    printf(" TESTING GENERAL BIT BANK: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
