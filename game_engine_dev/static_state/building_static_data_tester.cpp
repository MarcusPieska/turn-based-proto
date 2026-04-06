//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "building_static_data.h"

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

class BuildingStaticDataTester {
public:
    static BuildingStaticDataKey make_key (u16 raw) {
        return BuildingStaticDataKey::from_raw(raw);
    }
};

void test_set_items_updates_count () {
    static BuildingStaticDataStruct items[3];
    items[0].cost = 1;  items[0].effects.indices[0] = 10;
    items[1].cost = 2;  items[1].effects.indices[0] = 20;
    items[2].cost = 3;  items[2].effects.indices[0] = 30;
    BuildingStaticData::set_items(items, 3);
    note_result(BuildingStaticData::get_item_count() == 3, "set_items updates static item count");
    summarize_test_results();
}

void test_get_item_returns_expected_struct_by_key () {
    static BuildingStaticDataStruct items[4];
    items[0].cost = 7;   items[0].effects.indices[0] = 70;
    items[1].cost = 8;   items[1].effects.indices[0] = 80;
    items[2].cost = 9;   items[2].effects.indices[0] = 90;
    items[3].cost = 10;  items[3].effects.indices[0] = 100;
    BuildingStaticData::set_items(items, 4);

    BuildingStaticDataKey key = BuildingStaticDataTester::make_key(2);
    const BuildingStaticDataStruct& item = BuildingStaticData::get_item(key);

    note_result(item.cost == 9, "get_item returns expected member1");
    note_result(item.effects.indices[0] == 90, "get_item returns expected member2");
    summarize_test_results();
}

void test_get_item_returns_reference_to_backing_array () {
    static BuildingStaticDataStruct items[2];
    items[0].cost = 11;  items[0].effects.indices[0] = 110;
    items[1].cost = 12;  items[1].effects.indices[0] = 120;
    BuildingStaticData::set_items(items, 2);

    BuildingStaticDataKey key = BuildingStaticDataTester::make_key(1);
    const BuildingStaticDataStruct& item = BuildingStaticData::get_item(key);

    note_result(&item == &items[1], "get_item returns reference into backing array");
    summarize_test_results();
}

void test_set_items_can_replace_array_and_count () {
    static BuildingStaticDataStruct items_a[2];
    items_a[0].cost = 21; items_a[0].effects.indices[0] = 210;
    items_a[1].cost = 22; items_a[1].effects.indices[0] = 220;
    static BuildingStaticDataStruct items_b[5];
    items_b[0].cost = 31; items_b[0].effects.indices[0] = 310;
    items_b[1].cost = 32; items_b[1].effects.indices[0] = 320;
    items_b[2].cost = 33; items_b[2].effects.indices[0] = 330;
    items_b[3].cost = 34; items_b[3].effects.indices[0] = 340;
    items_b[4].cost = 35; items_b[4].effects.indices[0] = 350;

    BuildingStaticData::set_items(items_a, 2);
    note_result(BuildingStaticData::get_item_count() == 2, "initial array count set");

    BuildingStaticData::set_items(items_b, 5);
    note_result(BuildingStaticData::get_item_count() == 5, "replacement array count set");
    BuildingStaticDataKey key = BuildingStaticDataTester::make_key(4);
    const BuildingStaticDataStruct& item = BuildingStaticData::get_item(key);
    note_result(item.cost == 35, "replacement array item accessible");
    summarize_test_results();
}

void test_zero_count_is_allowed () {
    static BuildingStaticDataStruct items[1];
    items[0].cost = 41; items[0].effects.indices[0] = 410;
    BuildingStaticData::set_items(items, 0);
    note_result(BuildingStaticData::get_item_count() == 0, "zero item count can be stored");
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
    printf(" TESTING BUILDING REQ DATA: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
