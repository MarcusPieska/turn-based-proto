//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "city_flags_array.h"

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
//=> - Test helper functions -
//================================================================================================================================

class CityFlagsAccessor {
public:
    static CityFlagsKey make_key (u16 raw) {
        return CityFlagsKey(raw);
    }
};

void assert_all_clear (CityFlagsArray& arr, u16 count, cstr prefix) {
    for (u16 i = 0; i < count; ++i) {
        CityFlagsKey key = CityFlagsAccessor::make_key(i);
        str msg = str(prefix) + " - key " + std::to_string(i) + " starts clear";
        note_result(!arr.is_flagged(key), msg.c_str());
    }
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_array_starts_clear () {
    const u16 flag_count = 16;
    CityFlagsArray arr(flag_count);
    assert_all_clear(arr, flag_count, "CityFlagsArray starts clear");
    summarize_test_results();
}

void test_set_flag_roundtrip () {
    const u16 flag_count = 16;
    CityFlagsArray arr(flag_count);
    CityFlagsKey key = CityFlagsAccessor::make_key(5);

    note_result(!arr.is_flagged(key), "Flag is initially clear");
    arr.set_flag(key);
    note_result(arr.is_flagged(key), "Flag becomes set after set_flag");
    arr.clear_flag(key);
    note_result(!arr.is_flagged(key), "Flag becomes clear after clear_flag");

    summarize_test_results();
}

void test_multiple_flags_independent () {
    const u16 flag_count = 16;
    CityFlagsArray arr(flag_count);

    CityFlagsKey key_a = CityFlagsAccessor::make_key(2);
    CityFlagsKey key_b = CityFlagsAccessor::make_key(9);
    CityFlagsKey key_c = CityFlagsAccessor::make_key(13);

    arr.set_flag(key_a);
    arr.set_flag(key_c);

    note_result(arr.is_flagged(key_a), "First flagged key is set");
    note_result(!arr.is_flagged(key_b), "Middle untouched key stays clear");
    note_result(arr.is_flagged(key_c), "Last flagged key is set");

    arr.clear_flag(key_a);
    note_result(!arr.is_flagged(key_a), "Clearing one flag only clears that flag");
    note_result(arr.is_flagged(key_c), "Other flagged key remains set");

    summarize_test_results();
}

void test_none_key_access () {
    const u16 flag_count = 16;
    CityFlagsArray arr(flag_count);
    CityFlagsKey none_key = CityFlagsKey::None();

    note_result(!arr.is_flagged(none_key), "None key starts clear");
    arr.set_flag(none_key);
    note_result(arr.is_flagged(none_key), "None key can be set");
    arr.clear_flag(none_key);
    note_result(!arr.is_flagged(none_key), "None key can be cleared");

    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    test_array_starts_clear();
    test_set_flag_roundtrip();
    test_multiple_flags_independent();
    test_none_key_access();

    printf("=======================================================\n");
    printf(" TESTING CityFlags ARRAY: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
