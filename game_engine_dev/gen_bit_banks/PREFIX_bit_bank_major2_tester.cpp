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

void set_all_flags_for_batch ([CLASS_NAME_PREFIX]BitBank& bank, u16 batch_idx, u16 batch_size, bool value) {
    for (u16 flag_idx = 0; flag_idx < batch_size; ++flag_idx) {
        if (value) {
            [USER_CLASS_NAME]::set_flag(bank, batch_idx, flag_idx);
        } else {
            [USER_CLASS_NAME]::clear_flag(bank, batch_idx, flag_idx);
        }
    }
}

void test_batch_isolation_over_500_batches_for_batch_sizes_1_to_20 () {
    const u16 batch_count = 500;

    for (u16 batch_size = 1; batch_size <= 20; ++batch_size) {
        [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(batch_size);

        u16 last_batch_idx = 0;
        for (u16 i = 0; i < batch_count; ++i) {
            last_batch_idx = [USER_CLASS_NAME]::claim_batch(*bank);
        }

        {
            str msg = str("Claim sweep works for batch size ") + std::to_string(batch_size);
            note_result(last_batch_idx == static_cast<u16>(batch_count - 1), msg.c_str());
        }
        {
            str msg = str("Claim count is 500 for batch size ") + std::to_string(batch_size);
            note_result([USER_CLASS_NAME]::get_claimed_batch_count(*bank) == batch_count, msg.c_str());
        }

        for (u16 active_batch = 0; active_batch < batch_count; ++active_batch) {
            set_all_flags_for_batch(*bank, active_batch, batch_size, true);

            for (u16 check_batch = 0; check_batch < batch_count; ++check_batch) {
                for (u16 flag_idx = 0; flag_idx < batch_size; ++flag_idx) {
                    bool expected = (check_batch == active_batch);
                    bool actual = [USER_CLASS_NAME]::is_flagged(*bank, check_batch, flag_idx);
                    if (actual != expected) {
                        str msg = str("Isolation mismatch: size ")
                                + std::to_string(batch_size)
                                + ", active " + std::to_string(active_batch)
                                + ", checked " + std::to_string(check_batch)
                                + ", flag " + std::to_string(flag_idx);
                        note_result(false, msg.c_str());
                        break;
                    }
                }
            }

            set_all_flags_for_batch(*bank, active_batch, batch_size, false);
        }

        {
            str msg = str("Isolation holds for batch size ") + std::to_string(batch_size);
            note_result(total_test_fails == 0, msg.c_str());
        }

        [USER_CLASS_NAME]::destroy_bank(bank);
    }

    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_batch_isolation_over_500_batches_for_batch_sizes_1_to_20();

    printf("=======================================================\n");
    printf(" TESTING [MACRO_PREFIX] BIT BANK: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
