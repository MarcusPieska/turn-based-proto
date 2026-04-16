//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

#include "data_parser_base.h"
#include "data_reader.h"
#include "path_mng.h"
#include "str_mng.h"

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

const DataParserBase* g_tech_parser = NULL;
const DataParserBase* g_resource_parser = NULL;
const DataParserBase* g_city_flag_parser = NULL;
const DataParserBase* g_building_parser = NULL;
const DataParserBase* g_civ_parser = NULL;

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

class DataParserBaseHarness : public DataParserBase {
public:
    DataParserBaseHarness (const std::vector<RawItem>& raw_items, const NameToIdxCbs& cbs) :
        DataParserBase(raw_items, cbs) {}

    void parse_reqs (ItemReqsStruct* out_reqs, const std::vector<std::string>& line_items, u16 start_idx) const {
        *out_reqs = parse_item_reqs(line_items, start_idx);
    }

    const std::vector<std::string> split_line (const std::string& line) const {
        return get_line_items(line);
    }

    static u32 get_error_count_for_harness () {
        return get_error_count_for_tests();
    }

    static void reset_error_count_for_harness () {
        reset_error_count_for_tests();
    }
};

u16 cb_tech_name_to_idx (cstr name) {
    return g_tech_parser->name_to_idx(name);
}

u16 cb_resource_name_to_idx (cstr name) {
    return g_resource_parser->name_to_idx(name);
}

u16 cb_city_flag_name_to_idx (cstr name) {
    return g_city_flag_parser->name_to_idx(name);
}

u16 cb_building_name_to_idx (cstr name) {
    return g_building_parser->name_to_idx(name);
}

u16 cb_civ_name_to_idx (cstr name) {
    return g_civ_parser->name_to_idx(name);
}

void run_item_reqs_parse_file_tests (const DataParserBaseHarness& parser, cstr filepath, cstr tag, bool expect_valid) {
    StringReader reader(filepath);
    StringSplitter line_splitter("\n");
    StringTrimmer trimmer(" \t\r\n");

    const std::string content = reader.read();
    const std::vector<std::string> lines = line_splitter.split(content);

    u32 line_count = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }

        ++line_count;
        const std::vector<std::string> line_items = parser.split_line(line);
        ItemReqsStruct reqs;

        const u32 err_before = DataParserBaseHarness::get_error_count_for_harness();

        int stdout_backup_fd = -1;
        FILE* null_file = NULL;
        if (print_level < 3) {
            fflush(stdout);
            stdout_backup_fd = dup(fileno(stdout));
            null_file = fopen("/dev/null", "w");
            if (stdout_backup_fd != -1 && null_file != NULL) {
                dup2(fileno(null_file), fileno(stdout));
            }
        }

        parser.parse_reqs(&reqs, line_items, 0);

        if (print_level < 3) {
            fflush(stdout);
            if (stdout_backup_fd != -1) {
                dup2(stdout_backup_fd, fileno(stdout));
                close(stdout_backup_fd);
            }
            if (null_file != NULL) {
                fclose(null_file);
            }
        }

        const u32 err_after = DataParserBaseHarness::get_error_count_for_harness();
        const bool got_valid_parse = (err_after == err_before);
        std::string msg = std::string(tag) + " line " + std::to_string(line_count) + ": " + line;
        note_result(got_valid_parse == expect_valid, msg.c_str());
    }
}

void run_item_reqs_parse_tests () {
    NameToIdxCbs cbs = {};
    cbs.tech_name_to_idx = cb_tech_name_to_idx;
    cbs.resource_name_to_idx = cb_resource_name_to_idx;
    cbs.city_flag_name_to_idx = cb_city_flag_name_to_idx;
    cbs.building_name_to_idx = cb_building_name_to_idx;
    cbs.civ_name_to_idx = cb_civ_name_to_idx;

    PathMng paths("../");
    DataReader tech_reader(paths.get_path_to_techs());
    DataReader resource_reader(paths.get_path_to_resources());
    DataReader city_flag_reader(paths.get_path_to_city_flags());
    DataReader building_reader(paths.get_path_to_buildings());
    DataReader civ_reader(paths.get_path_to_civs());
    DataReader unit_reader(paths.get_path_to_units());

    DataParserBaseHarness tech_parser(tech_reader.get_raw_items(), cbs);
    DataParserBaseHarness resource_parser(resource_reader.get_raw_items(), cbs);
    DataParserBaseHarness city_flag_parser(city_flag_reader.get_raw_items(), cbs);
    DataParserBaseHarness building_parser(building_reader.get_raw_items(), cbs);
    DataParserBaseHarness civ_parser(civ_reader.get_raw_items(), cbs);

    DataParserBaseHarness unit_parser(unit_reader.get_raw_items(), cbs);
    g_tech_parser = &tech_parser;
    g_resource_parser = &resource_parser;
    g_city_flag_parser = &city_flag_parser;
    g_building_parser = &building_parser;
    g_civ_parser = &civ_parser;

    DataParserBaseHarness::reset_error_count_for_harness();
    run_item_reqs_parse_file_tests(unit_parser, "item_reqs_valid", "valid", true);
    run_item_reqs_parse_file_tests(unit_parser, "item_reqs_invalid", "invalid", false);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    run_item_reqs_parse_tests();
    summarize_test_results();

    printf("=======================================================\n");
    printf(" TESTING DATA PARSER BASE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
