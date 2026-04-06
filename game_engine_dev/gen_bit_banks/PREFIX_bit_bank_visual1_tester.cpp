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
        if (print_level > 1 && print_level < 3) {
            printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        if (print_level > 0 && print_level < 3) {
            printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void note_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_result (cond, msg.c_str());
}

void summarize_test_results () {
    if (print_level > 0 && print_level < 3) {
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
    static [CLASS_NAME_PREFIX]BitBank* create_bank (u16 b_size) {
        return new [CLASS_NAME_PREFIX]BitBank(b_size);
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

void set_all_flags_for_batch ([CLASS_NAME_PREFIX]BitBank& bank, u16 batch_idx, u16 b_size, bool value) {
    for (u16 flag_idx = 0; flag_idx < b_size; ++flag_idx) {
        if (value) {
            [USER_CLASS_NAME]::set_flag(bank, batch_idx, flag_idx);
        } else {
            [USER_CLASS_NAME]::clear_flag(bank, batch_idx, flag_idx);
        }
    }
}

void print_batch_flags ([CLASS_NAME_PREFIX]BitBank& bank, u16 batch_idx, u16 b_size) {
    static const cstr RED = "\033[31m";
    static const cstr GREEN = "\033[32m";
    static const cstr RESET = "\033[0m";

    printf("  batch %u: ", static_cast<u32>(batch_idx));
    for (u16 flag_idx = 0; flag_idx < b_size; ++flag_idx) {
        bool bit = [USER_CLASS_NAME]::is_flagged(bank, batch_idx, flag_idx);
        if (bit) {
            printf("%s1%s", GREEN, RESET);
        } else {
            printf("%s0%s", RED, RESET);
        }
    }
}

void print_batch1_only ([CLASS_NAME_PREFIX]BitBank& bank, u16 b_size) {
    print_batch_flags(bank, 1, b_size);
    printf("\n");
}

void test_visual_walk_batch1_flags_for_sizes_1_to_100 () {
    const u16 batch_count = 3;

    for (u16 b_size = 1; b_size <= 100; ++b_size) {
        [CLASS_NAME_PREFIX]BitBank* bank = [USER_CLASS_NAME]::create_bank(b_size);

        for (u16 i = 0; i < batch_count; ++i) {
            [USER_CLASS_NAME]::claim_batch(*bank);
        }

        set_all_flags_for_batch(*bank, 0, b_size, true);
        set_all_flags_for_batch(*bank, 2, b_size, true);

        for (u16 flag_idx = 0; flag_idx < b_size; ++flag_idx) {
            str msg = str("Batch 1 starts clear for size ") + std::to_string(b_size) + ", flag " + std::to_string(flag_idx);
            note_result(![USER_CLASS_NAME]::is_flagged(*bank, 1, flag_idx), msg.c_str());
        }

        u8 seen_flags[100] = {0};
        u16 observed_indices[100] = {0};
        u16 observed_count = 0;

        for (u16 idx = 0; idx < b_size; ++idx) {
            [USER_CLASS_NAME]::set_flag(*bank, 1, idx);

            if (print_level >= 3) {
                print_batch1_only(*bank, b_size);
            }

            u16 set_count_batch1 = 0;
            u16 idx_batch1 = 0;
            for (u16 flag_idx = 0; flag_idx < b_size; ++flag_idx) {
                bool b0 = [USER_CLASS_NAME]::is_flagged(*bank, 0, flag_idx);
                bool b1 = [USER_CLASS_NAME]::is_flagged(*bank, 1, flag_idx);
                bool b2 = [USER_CLASS_NAME]::is_flagged(*bank, 2, flag_idx);

                if (!b0 || !b2) {
                    str msg = str("Batch 0/2 all 1 for size ") + std::to_string(b_size) + ", iter " + std::to_string(idx);
                    note_result(false, msg.c_str());
                    break;
                }
                if (b1) {
                    set_count_batch1 = static_cast<u16>(set_count_batch1 + 1);
                    idx_batch1 = flag_idx;
                }
            }

            {
                str msg = str("Batch 1 has one set bit for size ") + std::to_string(b_size) + ", iter " + std::to_string(idx);
                note_result(set_count_batch1 == 1, msg.c_str());
            }
            {
                str msg = str("Batch 1 set-bit idx match for size ") + std::to_string(b_size) + ", iter " + std::to_string(idx);
                note_result(idx_batch1 == idx, msg.c_str());
            }

            observed_indices[observed_count] = idx_batch1;
            observed_count = static_cast<u16>(observed_count + 1);
            seen_flags[idx_batch1] = 1;

            [USER_CLASS_NAME]::clear_flag(*bank, 1, idx);
        }

        note_result(observed_count == b_size, "Observed index array has expected size");
        for (u16 i = 0; i < b_size; ++i) {
            str msg_seen = str("Observed indices contain flag ") + std::to_string(i) + " for size " + std::to_string(b_size);
            note_result(seen_flags[i] == 1, msg_seen.c_str());

            str msg_order = str("Set indices increment at pos ") + std::to_string(i) + " for size " + std::to_string(b_size);
            note_result(observed_indices[i] == i, msg_order.c_str());
        }

        if (print_level >= 3) {
            printf("\n");
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

    test_visual_walk_batch1_flags_for_sizes_1_to_100();

    printf("=======================================================\n");
    printf(" TESTING [MACRO_PREFIX] BIT BANK: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
