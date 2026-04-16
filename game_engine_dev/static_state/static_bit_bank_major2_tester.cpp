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

void test_array_isolation_over_500_arrays_for_sizes_1_to_20 () {
    const u16 array_count = 500;

    for (u16 array_size = 1; array_size <= 20; ++array_size) {
        StaticBitBank* bank = City::create_bank(array_count, array_size);
        note_result(!City::is_flagged(*bank, 0, 0), "Array 0 starts clear");
        note_result(!City::is_flagged(*bank, 499, static_cast<u16>(array_size - 1)), "Last valid flag starts clear");

        for (u16 active_array = 0; active_array < array_count; ++active_array) {
            set_all_flags_for_array(*bank, active_array, array_size, true);
            for (u16 check_array = 0; check_array < array_count; ++check_array) {
                for (u16 flag_idx = 0; flag_idx < array_size; ++flag_idx) {
                    bool expected = (check_array == active_array);
                    bool actual = City::is_flagged(*bank, check_array, flag_idx);
                    if (actual != expected) {
                        str msg = str("Isolation mismatch: size ")
                                + std::to_string(array_size)
                                + ", active " + std::to_string(active_array)
                                + ", checked " + std::to_string(check_array)
                                + ", flag " + std::to_string(flag_idx);
                        note_result(false, msg.c_str());
                        break;
                    }
                }
            }
            set_all_flags_for_array(*bank, active_array, array_size, false);
        }

        {
            str msg = str("Isolation holds for array size ") + std::to_string(array_size);
            note_result(total_test_fails == 0, msg.c_str());
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

    test_array_isolation_over_500_arrays_for_sizes_1_to_20();

    printf("=======================================================\n");
    printf(" TESTING STATIC BIT BANK: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

