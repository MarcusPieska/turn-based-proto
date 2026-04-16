//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "civ_static_data.h"

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

class CivStaticDataTester {
public:
    static CivStaticDataKey make_key (u16 raw) {
        return CivStaticDataKey::from_raw(raw);
    }
};

void test_set_items_updates_count () {
    static CivStaticDataStruct items[3];
    items[0].traits.indices[0] = 1;  items[0].traits.indices[1] = 10;
    items[1].traits.indices[0] = 2;  items[1].traits.indices[1] = 20;
    items[2].traits.indices[0] = 3;  items[2].traits.indices[1] = 30;
    CivStaticData::set_items(items, 3);
    note_result(CivStaticData::get_item_count() == 3, "set_items updates static item count");
    summarize_test_results();
}

void test_get_item_returns_expected_struct_by_key () {
    static CivStaticDataStruct items[4];
    items[0].traits.indices[0] = 7;   items[0].traits.indices[1] = 70;
    items[1].traits.indices[0] = 8;   items[1].traits.indices[1] = 80;
    items[2].traits.indices[0] = 9;   items[2].traits.indices[1] = 90;
    items[3].traits.indices[0] = 10;  items[3].traits.indices[1] = 100;
    CivStaticData::set_items(items, 4);

    CivStaticDataKey key = CivStaticDataTester::make_key(2);
    const CivStaticDataStruct& item = CivStaticData::get_item(key);

    note_result(item.traits.indices[0] == 9, "get_item returns expected member1");
    note_result(item.traits.indices[1] == 90, "get_item returns expected member2");
    summarize_test_results();
}

void test_get_item_returns_reference_to_backing_array () {
    static CivStaticDataStruct items[2];
    items[0].traits.indices[0] = 11;  items[0].traits.indices[1] = 110;
    items[1].traits.indices[0] = 12;  items[1].traits.indices[1] = 120;
    CivStaticData::set_items(items, 2);

    CivStaticDataKey key = CivStaticDataTester::make_key(1);
    const CivStaticDataStruct& item = CivStaticData::get_item(key);

    note_result(&item == &items[1], "get_item returns reference into backing array");
    summarize_test_results();
}

void test_set_items_can_replace_array_and_count () {
    static CivStaticDataStruct items_a[2];
    items_a[0].traits.indices[0] = 21; items_a[0].traits.indices[1] = 210;
    items_a[1].traits.indices[0] = 22; items_a[1].traits.indices[1] = 220;
    static CivStaticDataStruct items_b[5];
    items_b[0].traits.indices[0] = 31; items_b[0].traits.indices[1] = 131;
    items_b[1].traits.indices[0] = 32; items_b[1].traits.indices[1] = 132;
    items_b[2].traits.indices[0] = 33; items_b[2].traits.indices[1] = 133;
    items_b[3].traits.indices[0] = 34; items_b[3].traits.indices[1] = 134;
    items_b[4].traits.indices[0] = 35; items_b[4].traits.indices[1] = 135;

    CivStaticData::set_items(items_a, 2);
    note_result(CivStaticData::get_item_count() == 2, "initial array count set");

    CivStaticData::set_items(items_b, 5);
    note_result(CivStaticData::get_item_count() == 5, "replacement array count set");
    CivStaticDataKey key = CivStaticDataTester::make_key(4);
    const CivStaticDataStruct& item = CivStaticData::get_item(key);
    note_result(item.traits.indices[0] == 35, "replacement array item accessible");
    summarize_test_results();
}

void test_zero_count_is_allowed () {
    static CivStaticDataStruct items[1];
    items[0].traits.indices[0] = 41; items[0].traits.indices[1] = 141;
    CivStaticData::set_items(items, 0);
    note_result(CivStaticData::get_item_count() == 0, "zero item count can be stored");
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
    printf(" TESTING CIV REQ DATA: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
