//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <string>
#include "static_parsing_manager.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

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
        printf("-----------------------------------------------------------\n");
        printf(" Test count: %d\n", test_count);
        printf(" Test pass: %d\n", test_pass);
        printf(" Test fail: %d\n", test_count - test_pass);
        printf("-----------------------------------------------------------\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

void print_u16_member (cstr label, u16 value) {
    printf(" %s: %u\n", label, value);
}

void print_item_counts (const StaticParsingManager& parser) {
    printf("-----------------------------------------------------------\n");
    printf("STATIC PARSING MANAGER COUNTS\n");
    print_u16_member("tech", parser.get_tech_count());
    print_u16_member("resource", parser.get_resource_count());
    print_u16_member("city_flag", parser.get_city_flag_count());
    print_u16_member("building", parser.get_building_count());
    print_u16_member("unit", parser.get_unit_count());
    print_u16_member("wonder", parser.get_wonder_count());
    print_u16_member("small_wonder", parser.get_small_wonder_count());
    print_u16_member("civ", parser.get_civ_count());
    print_u16_member("unit_type", parser.get_unit_type_count());
    print_u16_member("government", parser.get_government_count());
    print_u16_member("civ_trait", parser.get_civ_trait_count());
    print_u16_member("effect", parser.get_effect_count());
    print_u16_member("callback", parser.get_callback_count());
}

u16 get_req_limit_for_type (const StaticParsingManager& parser, u8 req_type) {
    if (req_type == ITEM_REQ_TYPE_TECH) {
        return parser.get_tech_count();
    }
    if (req_type == ITEM_REQ_TYPE_RESOURCE) {
        return parser.get_resource_count();
    }
    if (req_type == ITEM_REQ_TYPE_FLAG) {
        return parser.get_city_flag_count();
    }
    if (req_type == ITEM_REQ_TYPE_CIV) {
        return parser.get_civ_count();
    }
    if (req_type == ITEM_REQ_TYPE_BUILDING) {
        return parser.get_building_count();
    }
    return 0;
}

bool are_reqs_in_bounds (const StaticParsingManager& parser, const ItemReqsStruct& reqs, cstr label, u16 item_idx) {
    for (u16 j = 0; j < MAX_PREREQ_COUNT; ++j) {
        const u8 req_type = reqs.types[j];
        if (req_type == ITEM_REQ_TYPE_NONE) {
            continue;
        }
        const u16 req_idx = reqs.indices[j];
        const u16 req_limit = get_req_limit_for_type(parser, req_type);
        cstr frm = "*** OOB REQ: %s item=%u req_slot=%u type=%u idx=%u limit=%u\n";
        if (req_idx >= req_limit) {
            if (print_level > 0) {
                printf(frm, label, item_idx, j, req_type, req_idx, req_limit);
            }
            return false;
        }
    }
    return true;
}

template <typename T>
bool test_dataset_req_bounds (const StaticParsingManager& parser, const T* data, u16 count, cstr label) {
    for (u16 i = 0; i < count; ++i) {
        if (!are_reqs_in_bounds(parser, data[i].reqs, label, i)) {
            return false;
        }
    }
    return true;
}

void run_req_bounds_tests (const StaticParsingManager& parser) {
    bool result = test_dataset_req_bounds(parser, parser.get_tech_data(), parser.get_tech_count(), "tech");
    note_result(result, "TechStaticDataStruct req indices in bounds");
    
    result = test_dataset_req_bounds(parser, parser.get_resource_data(), parser.get_resource_count(), "resource");
    note_result(result, "ResourceStaticDataStruct req indices in bounds");
    
    result = test_dataset_req_bounds(parser, parser.get_city_flag_data(), parser.get_city_flag_count(), "city_flag");
    note_result(result, "CityFlagStaticDataStruct req indices in bounds");
    
    result = test_dataset_req_bounds(parser, parser.get_building_data(), parser.get_building_count(), "building");
    note_result(result, "BuildingStaticDataStruct req indices in bounds");
    
    result = test_dataset_req_bounds(parser, parser.get_unit_data(), parser.get_unit_count(), "unit");
    note_result(result, "UnitStaticDataStruct req indices in bounds");
    
    result = test_dataset_req_bounds(parser, parser.get_wonder_data(), parser.get_wonder_count(), "wonder");
    note_result(result, "WonderStaticDataStruct req indices in bounds");
    
    result = test_dataset_req_bounds(parser, parser.get_small_wonder_data(), parser.get_small_wonder_count(), "small_wonder");
    note_result(result, "SmallWonderStaticDataStruct req indices in bounds");
}

int run_parse_driver () {
    StaticParsingManager parser("../");
    run_req_bounds_tests(parser);
    if (print_level >= 1) {
        print_item_counts(parser);
    }
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    run_parse_driver();
    summarize_test_results();

    printf("=======================================================\n");
    printf(" TESTING STATIC PARSING MANAGER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
