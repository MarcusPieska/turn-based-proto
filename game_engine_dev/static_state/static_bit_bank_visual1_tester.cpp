//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "static_bit_bank.h"

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

class City {
public:
    static StaticBitBank* create_bank (u16 array_count, u16 array_size) {
        return new StaticBitBank(array_count, array_size);
    }

    static void destroy_bank (StaticBitBank* bank) {
        delete bank;
    }

    static bool is_flagged (const StaticBitBank& bank, u16 array_idx, u16 flag_idx) {
        return bank.is_flagged(array_idx, flag_idx);
    }

    static void set_flag (StaticBitBank& bank, u16 array_idx, u16 flag_idx) {
        bank.set_flag(array_idx, flag_idx);
    }

    static void clear_flag (StaticBitBank& bank, u16 array_idx, u16 flag_idx) {
        bank.clear_flag(array_idx, flag_idx);
    }
};

void set_all_flags_for_array (StaticBitBank& bank, u16 array_idx, u16 array_size, bool value) {
    for (u16 flag_idx = 0; flag_idx < array_size; ++flag_idx) {
        if (value) {
            City::set_flag(bank, array_idx, flag_idx);
        } else {
            City::clear_flag(bank, array_idx, flag_idx);
        }
    }
}

void print_array_flags (StaticBitBank& bank, u16 array_idx, u16 array_size) {
    static const cstr RED = "\033[31m";
    static const cstr GREEN = "\033[32m";
    static const cstr RESET = "\033[0m";
    printf("  array %u: ", static_cast<u32>(array_idx));
    for (u16 flag_idx = 0; flag_idx < array_size; ++flag_idx) {
        bool bit = City::is_flagged(bank, array_idx, flag_idx);
        if (bit) {
            printf("%s1%s", GREEN, RESET);
        } else {
            printf("%s0%s", RED, RESET);
        }
    }
}

void print_array1_only (StaticBitBank& bank, u16 array_size) {
    print_array_flags(bank, 1, array_size);
    printf("\n");
}

void test_visual_walk_array1_flags_for_sizes_1_to_100 () {
    const u16 array_count = 3;

    for (u16 array_size = 1; array_size <= 100; ++array_size) {
        StaticBitBank* bank = City::create_bank(array_count, array_size);
        set_all_flags_for_array(*bank, 0, array_size, true);
        set_all_flags_for_array(*bank, 2, array_size, true);

        for (u16 flag_idx = 0; flag_idx < array_size; ++flag_idx) {
            str msg = str("Array 1 starts clear for size ") + std::to_string(array_size) + ", flag " + std::to_string(flag_idx);
            note_result(!City::is_flagged(*bank, 1, flag_idx), msg.c_str());
        }

        u8 seen_flags[100] = {0};
        u16 observed_indices[100] = {0};
        u16 observed_count = 0;

        for (u16 idx = 0; idx < array_size; ++idx) {
            City::set_flag(*bank, 1, idx);

            if (print_level >= 3) {
                print_array1_only(*bank, array_size);
            }

            u16 set_count_array1 = 0;
            u16 idx_array1 = 0;
            for (u16 flag_idx = 0; flag_idx < array_size; ++flag_idx) {
                bool a0 = City::is_flagged(*bank, 0, flag_idx);
                bool a1 = City::is_flagged(*bank, 1, flag_idx);
                bool a2 = City::is_flagged(*bank, 2, flag_idx);

                if (!a0 || !a2) {
                    str msg = str("Array 0/2 all 1 for size ") + std::to_string(array_size) + ", iter " + std::to_string(idx);
                    note_result(false, msg.c_str());
                    break;
                }
                if (a1) {
                    set_count_array1 = static_cast<u16>(set_count_array1 + 1);
                    idx_array1 = flag_idx;
                }
            }

            {
                str msg = str("Array 1 has one set bit for size ") + std::to_string(array_size) + ", iter " + std::to_string(idx);
                note_result(set_count_array1 == 1, msg.c_str());
            }
            {
                str msg = str("Array 1 set-bit idx match for size ") + std::to_string(array_size) + ", iter " + std::to_string(idx);
                note_result(idx_array1 == idx, msg.c_str());
            }

            observed_indices[observed_count] = idx_array1;
            observed_count = static_cast<u16>(observed_count + 1);
            seen_flags[idx_array1] = 1;
            City::clear_flag(*bank, 1, idx);
        }

        note_result(observed_count == array_size, "Observed index array has expected size");
        for (u16 i = 0; i < array_size; ++i) {
            str msg_seen = str("Observed indices contain flag ") + std::to_string(i) + " for size " + std::to_string(array_size);
            note_result(seen_flags[i] == 1, msg_seen.c_str());
            str msg_order = str("Set indices increment at pos ") + std::to_string(i) + " for size " + std::to_string(array_size);
            note_result(observed_indices[i] == i, msg_order.c_str());
        }

        if (print_level >= 3) {
            printf("\n");
        }

        City::destroy_bank(bank);
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

    test_visual_walk_array1_flags_for_sizes_1_to_100();

    printf("=======================================================\n");
    printf(" TESTING STATIC BIT BANK: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
