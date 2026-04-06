//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <string>

#include "path_mng.h"
#include "data_reader.h"

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

void note_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_result(cond, msg.c_str());
}

void summarize_test_results () {
    if (print_level > 0) {
        printf("--------------------------------\n");
        printf(" Test count: %d\n", test_count);
        printf(" Test pass: %d\n", test_pass);
        printf(" Test fail: %d\n", test_count - test_pass);
        printf("--------------------------------\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void note_reader_has_min_items (const std::string& path, cstr tag, u32 min_items) {
    DataReader reader(path);
    const std::vector<RawItem>& items = reader.get_raw_items();
    str msg = str(tag) + " has at least " + std::to_string(min_items) + " parsed items";
    note_result(items.size() >= min_items, msg.c_str());
}

void test_read_all_game_config_files_min_items () {
    PathMng paths("../");
    const u32 min_items = 5;

    note_reader_has_min_items(paths.get_path_to_techs(), "techs", min_items);
    note_reader_has_min_items(paths.get_path_to_resources(), "resources", min_items);
    note_reader_has_min_items(paths.get_path_to_wonders_small(), "wonders_small", min_items);
    note_reader_has_min_items(paths.get_path_to_city_flags(), "city_flags", min_items);
    note_reader_has_min_items(paths.get_path_to_unit_types(), "unit_types", min_items);
    note_reader_has_min_items(paths.get_path_to_wonders(), "wonders", min_items);
    note_reader_has_min_items(paths.get_path_to_governments(), "governments", min_items);
    note_reader_has_min_items(paths.get_path_to_callbacks(), "callbacks", min_items);
    note_reader_has_min_items(paths.get_path_to_civ_traits(), "civ_traits", min_items);
    note_reader_has_min_items(paths.get_path_to_units(), "units", min_items);
    note_reader_has_min_items(paths.get_path_to_civs(), "civs", min_items);
    note_reader_has_min_items(paths.get_path_to_buildings(), "buildings", min_items);
    note_reader_has_min_items(paths.get_path_to_effects(), "effects", min_items);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_read_all_game_config_files_min_items();
    summarize_test_results();

    printf("=======================================================\n");
    printf(" TESTING DATA READER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
