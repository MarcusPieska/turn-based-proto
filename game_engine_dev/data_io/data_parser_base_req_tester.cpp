//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "data_parser_base.h"
#include "path_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

const DataParserBase* g_tech_parser = NULL;
const DataParserBase* g_resource_parser = NULL;
const DataParserBase* g_toggle_city_parser = NULL;
const DataParserBase* g_building_parser = NULL;
const DataParserBase* g_civ_parser = NULL; 
const DataParserBase* g_civ_trait_parser = NULL;
const DataParserBase* g_map_overlay_parser = NULL;
const DataParserBase* g_map_attribute_parser = NULL;
const DataParserBase* g_map_terrain_parser = NULL;
const DataParserBase* g_map_climate_parser = NULL;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static bool is_ws (char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool trim_to_buf (cstr in, char* out, u32 cap) {
    if (!out || cap == 0) return false;
    if (!in) in = "";
    u32 b = 0;
    u32 e = (u32)std::strlen(in);
    while (b < e && is_ws(in[b])) ++b;
    while (e > b && is_ws(in[e - 1])) --e;
    u32 n = e - b;
    if (n + 1 > cap) return false;
    if (n > 0) std::memcpy(out, in + b, n);
    out[n] = '\0';
    return true;
}

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
    DataParserBaseHarness (const StringManager& raw_lines, const NameToIdxCbs& cbs) :
        DataParserBase(raw_lines, cbs) {}

    void parse_reqs (ItemReqsStruct* out_reqs, const StringManager& line_items, u16 start_idx) const {
        *out_reqs = parse_item_reqs(line_items, start_idx);
    }

    void split_line (cstr line, StringManager& out_items) const {
        get_line_items(line, out_items);
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

u16 cb_toggle_city_name_to_idx (cstr name) {
    return g_toggle_city_parser->name_to_idx(name);
}

u16 cb_building_name_to_idx (cstr name) {
    return g_building_parser->name_to_idx(name);
}

u16 cb_civ_name_to_idx (cstr name) {
    return g_civ_parser->name_to_idx(name);
}

u16 cb_civ_trait_name_to_idx (cstr name) {
    return g_civ_trait_parser->name_to_idx(name);
}

u16 cb_map_overlay_name_to_idx (cstr name) {
    return g_map_overlay_parser->name_to_idx(name);
}

u16 cb_map_attribute_name_to_idx (cstr name) {
    return g_map_attribute_parser->name_to_idx(name);
}

u16 cb_map_terrain_name_to_idx (cstr name) {
    return g_map_terrain_parser->name_to_idx(name);
}

u16 cb_map_climate_name_to_idx (cstr name) {
    return g_map_climate_parser->name_to_idx(name);
}

void run_item_reqs_parse_file_tests (const DataParserBaseHarness& parser, cstr filepath, cstr tag, bool expect_valid) {
    StringManager lines;
    if (!lines.load_file_content(filepath)) {
        note_result(false, "failed to load req test file");
        return;
    }
    lines.split_string_by_char(0, '\n');
    lines.cull_empty_strings();

    u32 line_count = 0;
    char line_buf[512];
    for (u32 i = 0; i < lines.get_string_count(); ++i) {
        if (!trim_to_buf(lines.get_string_content(i), line_buf, sizeof(line_buf)) || line_buf[0] == '\0') {
            continue;
        }

        ++line_count;
        StringManager line_items;
        parser.split_line(line_buf, line_items);
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
        char msg[768];
        std::snprintf(msg, sizeof(msg), "%s line %u: %s", tag, line_count, line_buf);
        note_result(got_valid_parse == expect_valid, msg);
    }
}

void run_item_reqs_parse_tests () {
    NameToIdxCbs cbs = {};
    cbs.tech_name_to_idx = cb_tech_name_to_idx;
    cbs.resource_name_to_idx = cb_resource_name_to_idx;
    cbs.toggle_city_name_to_idx = cb_toggle_city_name_to_idx;
    cbs.building_name_to_idx = cb_building_name_to_idx;
    cbs.civ_name_to_idx = cb_civ_name_to_idx;
    cbs.civ_trait_name_to_idx = cb_civ_trait_name_to_idx;
    cbs.map_overlay_name_to_idx = cb_map_overlay_name_to_idx;
    cbs.map_attribute_name_to_idx = cb_map_attribute_name_to_idx;
    cbs.map_terrain_name_to_idx = cb_map_terrain_name_to_idx;
    cbs.map_climate_name_to_idx = cb_map_climate_name_to_idx;

    PathMng paths("../");
    StringManager tech_items;
    StringManager resource_items;
    StringManager toggle_city_items;
    StringManager building_items;
    StringManager civ_items;
    StringManager civ_trait_items;
    StringManager unit_items;
    StringManager map_overlay_items;
    StringManager map_attribute_items;
    StringManager map_terrain_items;
    StringManager map_climate_items;

    tech_items.load_file_content(paths.get_path_to_techs());
    tech_items.split_string_by_char(0, '\n');
    tech_items.cull_empty_strings();

    resource_items.load_file_content(paths.get_path_to_resources());
    resource_items.split_string_by_char(0, '\n');
    resource_items.cull_empty_strings();

    toggle_city_items.load_file_content(paths.get_path_to_toggle_city());
    toggle_city_items.split_string_by_char(0, '\n');
    toggle_city_items.cull_empty_strings();

    building_items.load_file_content(paths.get_path_to_buildings());
    building_items.split_string_by_char(0, '\n');
    building_items.cull_empty_strings();

    civ_items.load_file_content(paths.get_path_to_civs());
    civ_items.split_string_by_char(0, '\n');
    civ_items.cull_empty_strings();

    civ_trait_items.load_file_content(paths.get_path_to_civ_traits());
    civ_trait_items.split_string_by_char(0, '\n');
    civ_trait_items.cull_empty_strings();

    unit_items.load_file_content(paths.get_path_to_units());
    unit_items.split_string_by_char(0, '\n');
    unit_items.cull_empty_strings();

    map_overlay_items.load_file_content(paths.get_path_to_map_overlays());
    map_overlay_items.split_string_by_char(0, '\n');
    map_overlay_items.cull_empty_strings();

    map_attribute_items.load_file_content(paths.get_path_to_map_attributes());
    map_attribute_items.split_string_by_char(0, '\n');
    map_attribute_items.cull_empty_strings();

    map_terrain_items.load_file_content(paths.get_path_to_map_terrains());
    map_terrain_items.split_string_by_char(0, '\n');
    map_terrain_items.cull_empty_strings();

    map_climate_items.load_file_content(paths.get_path_to_map_climates());
    map_climate_items.split_string_by_char(0, '\n');
    map_climate_items.cull_empty_strings();

    DataParserBaseHarness tech_parser(tech_items, cbs);
    DataParserBaseHarness resource_parser(resource_items, cbs);
    DataParserBaseHarness toggle_city_parser(toggle_city_items, cbs);
    DataParserBaseHarness building_parser(building_items, cbs);
    DataParserBaseHarness civ_parser(civ_items, cbs);
    DataParserBaseHarness civ_trait_parser(civ_trait_items, cbs);
    DataParserBaseHarness unit_parser(unit_items, cbs);
    DataParserBaseHarness map_overlay_parser(map_overlay_items, cbs);
    DataParserBaseHarness map_attribute_parser(map_attribute_items, cbs);
    DataParserBaseHarness map_terrain_parser(map_terrain_items, cbs);
    DataParserBaseHarness map_climate_parser(map_climate_items, cbs);

    g_tech_parser = &tech_parser;
    g_resource_parser = &resource_parser;
    g_toggle_city_parser = &toggle_city_parser;
    g_building_parser = &building_parser;
    g_civ_parser = &civ_parser;
    g_civ_trait_parser = &civ_trait_parser;
    g_map_overlay_parser = &map_overlay_parser;
    g_map_attribute_parser = &map_attribute_parser;
    g_map_terrain_parser = &map_terrain_parser;
    g_map_climate_parser = &map_climate_parser;

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
