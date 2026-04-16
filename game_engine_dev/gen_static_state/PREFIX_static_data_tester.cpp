//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "[MEMBER_TAG]_static_data.h"

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

class [CLASS_NAME]Tester {
public:
    static [DATA_KEY] make_key (u16 raw) {
        return [DATA_KEY]::from_raw(raw);
    }
};

void test_set_items_updates_count () {
    static [DATA_STRUCT] items[3];
    items[0].[STRUCT_MEMBER1] = 1;  items[0].[STRUCT_MEMBER2] = 10;
    items[1].[STRUCT_MEMBER1] = 2;  items[1].[STRUCT_MEMBER2] = 20;
    items[2].[STRUCT_MEMBER1] = 3;  items[2].[STRUCT_MEMBER2] = 30;
    [CLASS_NAME]::set_items(items, 3);
    note_result([CLASS_NAME]::get_item_count() == 3, "set_items updates static item count");
    summarize_test_results();
}

void test_get_item_returns_expected_struct_by_key () {
    static [DATA_STRUCT] items[4];
    items[0].[STRUCT_MEMBER1] = 7;   items[0].[STRUCT_MEMBER2] = 70;
    items[1].[STRUCT_MEMBER1] = 8;   items[1].[STRUCT_MEMBER2] = 80;
    items[2].[STRUCT_MEMBER1] = 9;   items[2].[STRUCT_MEMBER2] = 90;
    items[3].[STRUCT_MEMBER1] = 10;  items[3].[STRUCT_MEMBER2] = 100;
    [CLASS_NAME]::set_items(items, 4);

    [DATA_KEY] key = [CLASS_NAME]Tester::make_key(2);
    const [DATA_STRUCT]& item = [CLASS_NAME]::get_item(key);

    note_result(item.[STRUCT_MEMBER1] == 9, "get_item returns expected member1");
    note_result(item.[STRUCT_MEMBER2] == 90, "get_item returns expected member2");
    summarize_test_results();
}

void test_get_item_returns_reference_to_backing_array () {
    static [DATA_STRUCT] items[2];
    items[0].[STRUCT_MEMBER1] = 11;  items[0].[STRUCT_MEMBER2] = 110;
    items[1].[STRUCT_MEMBER1] = 12;  items[1].[STRUCT_MEMBER2] = 120;
    [CLASS_NAME]::set_items(items, 2);

    [DATA_KEY] key = [CLASS_NAME]Tester::make_key(1);
    const [DATA_STRUCT]& item = [CLASS_NAME]::get_item(key);

    note_result(&item == &items[1], "get_item returns reference into backing array");
    summarize_test_results();
}

void test_set_items_can_replace_array_and_count () {
    static [DATA_STRUCT] items_a[2];
    items_a[0].[STRUCT_MEMBER1] = 21; items_a[0].[STRUCT_MEMBER2] = 210;
    items_a[1].[STRUCT_MEMBER1] = 22; items_a[1].[STRUCT_MEMBER2] = 220;
    static [DATA_STRUCT] items_b[5];
    items_b[0].[STRUCT_MEMBER1] = 31; items_b[0].[STRUCT_MEMBER2] = 131;
    items_b[1].[STRUCT_MEMBER1] = 32; items_b[1].[STRUCT_MEMBER2] = 132;
    items_b[2].[STRUCT_MEMBER1] = 33; items_b[2].[STRUCT_MEMBER2] = 133;
    items_b[3].[STRUCT_MEMBER1] = 34; items_b[3].[STRUCT_MEMBER2] = 134;
    items_b[4].[STRUCT_MEMBER1] = 35; items_b[4].[STRUCT_MEMBER2] = 135;

    [CLASS_NAME]::set_items(items_a, 2);
    note_result([CLASS_NAME]::get_item_count() == 2, "initial array count set");

    [CLASS_NAME]::set_items(items_b, 5);
    note_result([CLASS_NAME]::get_item_count() == 5, "replacement array count set");
    [DATA_KEY] key = [CLASS_NAME]Tester::make_key(4);
    const [DATA_STRUCT]& item = [CLASS_NAME]::get_item(key);
    note_result(item.[STRUCT_MEMBER1] == 35, "replacement array item accessible");
    summarize_test_results();
}

void test_zero_count_is_allowed () {
    static [DATA_STRUCT] items[1];
    items[0].[STRUCT_MEMBER1] = 41; items[0].[STRUCT_MEMBER2] = 141;
    [CLASS_NAME]::set_items(items, 0);
    note_result([CLASS_NAME]::get_item_count() == 0, "zero item count can be stored");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_set_items_updates_count();
    test_get_item_returns_expected_struct_by_key();
    test_get_item_returns_reference_to_backing_array();
    test_set_items_can_replace_array_and_count();
    test_zero_count_is_allowed();

    printf("=======================================================\n");
    printf(" TESTING [MACRO_PREFIX] REQ DATA: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
