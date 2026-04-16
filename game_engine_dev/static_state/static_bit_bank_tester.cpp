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

void assert_array_is_clear (StaticBitBank& bank, u16 array_idx, u16 array_size, cstr prefix) {
    for (u16 flag_idx = 0; flag_idx < array_size; ++flag_idx) {
        bool flagged = City::is_flagged(bank, array_idx, flag_idx);
        str msg = str(prefix) + " - array " + std::to_string(array_idx)
                + " flag " + std::to_string(flag_idx) + " is clear";
        note_result(!flagged, msg.c_str());
    }
}

void set_all_flags_for_array (StaticBitBank& bank, u16 array_idx, u16 array_size, bool value) {
    for (u16 flag_idx = 0; flag_idx < array_size; ++flag_idx) {
        if (value) {
            City::set_flag(bank, array_idx, flag_idx);
        } else {
            City::clear_flag(bank, array_idx, flag_idx);
        }
    }
}

void test_constructed_bank_starts_clear () {
    const u16 array_count = 5;
    const u16 array_size = 9;
    StaticBitBank* bank = City::create_bank(array_count, array_size);
    for (u16 i = 0; i < array_count; ++i) {
        assert_array_is_clear(*bank, i, array_size, "Constructed bank");
    }
    City::destroy_bank(bank);
    summarize_test_results();
}

void test_set_clear_round_trip () {
    StaticBitBank* bank = City::create_bank(8, 10);
    note_result(!City::is_flagged(*bank, 3, 7), "Flag starts clear before set");
    City::set_flag(*bank, 3, 7);
    note_result(City::is_flagged(*bank, 3, 7), "Flag reads set after set_flag");
    City::clear_flag(*bank, 3, 7);
    note_result(!City::is_flagged(*bank, 3, 7), "Flag reads clear after clear_flag");
    City::destroy_bank(bank);
    summarize_test_results();
}

void test_flags_are_independent_within_same_array () {
    StaticBitBank* bank = City::create_bank(2, 12);
    City::set_flag(*bank, 1, 1);
    City::set_flag(*bank, 1, 7);
    note_result(City::is_flagged(*bank, 1, 1), "Flag 1 set");
    note_result(!City::is_flagged(*bank, 1, 2), "Flag 2 remains clear");
    note_result(City::is_flagged(*bank, 1, 7), "Flag 7 set");
    note_result(!City::is_flagged(*bank, 1, 11), "Flag 11 remains clear");
    City::destroy_bank(bank);
    summarize_test_results();
}

void test_flags_are_independent_across_arrays () {
    StaticBitBank* bank = City::create_bank(4, 8);
    City::set_flag(*bank, 0, 4);
    note_result(City::is_flagged(*bank, 0, 4), "Set flag visible on array 0");
    note_result(!City::is_flagged(*bank, 1, 4), "Same flag index on array 1 remains clear");
    City::set_flag(*bank, 1, 2);
    note_result(City::is_flagged(*bank, 1, 2), "Array 1 can set its own flag");
    note_result(!City::is_flagged(*bank, 0, 2), "Array 1 set does not leak to array 0");
    City::destroy_bank(bank);
    summarize_test_results();
}

void test_large_indices_round_trip () {
    const u16 array_count = 700;
    const u16 array_size = 71;
    StaticBitBank* bank = City::create_bank(array_count, array_size);
    City::set_flag(*bank, 699, 70);
    note_result(City::is_flagged(*bank, 699, 70), "Highest valid index pair can be set");
    note_result(!City::is_flagged(*bank, 698, 70), "Neighbor array remains clear");
    note_result(!City::is_flagged(*bank, 699, 69), "Neighbor flag remains clear");
    City::clear_flag(*bank, 699, 70);
    note_result(!City::is_flagged(*bank, 699, 70), "Highest valid index pair can be cleared");
    City::destroy_bank(bank);
    summarize_test_results();
}

void test_single_active_array_isolation () {
    const u16 array_count = 64;
    const u16 array_size = 11;
    StaticBitBank* bank = City::create_bank(array_count, array_size);
    for (u16 active_array = 0; active_array < array_count; ++active_array) {
        set_all_flags_for_array(*bank, active_array, array_size, true);
        for (u16 check_array = 0; check_array < array_count; ++check_array) {
            for (u16 flag_idx = 0; flag_idx < array_size; ++flag_idx) {
                bool expected = (check_array == active_array);
                bool actual = City::is_flagged(*bank, check_array, flag_idx);
                if (actual != expected) {
                    str msg = str("Isolation mismatch: active array ")
                            + std::to_string(active_array)
                            + ", checked array " + std::to_string(check_array)
                            + ", flag " + std::to_string(flag_idx);
                    note_result(false, msg.c_str());
                    break;
                }
            }
        }
        set_all_flags_for_array(*bank, active_array, array_size, false);
    }
    note_result(total_test_fails == 0, "Array isolation holds across 64 arrays");
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

    test_constructed_bank_starts_clear();
    test_set_clear_round_trip();
    test_flags_are_independent_within_same_array();
    test_flags_are_independent_across_arrays();
    test_large_indices_round_trip();
    test_single_active_array_isolation();

    printf("=======================================================\n");
    printf(" TESTING STATIC BIT BANK: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
