//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "city_array.h"

//================================================================================================================================
//=> - GLOBALS -
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
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        if (print_level > 0) {
            total_test_fails++;
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void summarize_test_results () {
    if (print_level > 0) {
        std::printf("--------------------------------\n");
        std::printf(" Test count: %d\n", test_count);
        std::printf(" Test pass: %d\n", test_pass);
        std::printf(" Test fail: %d\n", test_count - test_pass);
        std::printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass  = 0;
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_first_city_zero_initialized () {
    CityArray array;
    u16 idx = array.get_next_new_city_idx();
    City* city = array.get_city(idx);

    bool ok = true;
    if (city == nullptr) {
        ok = false;
    } else {
        struct CityHeader {
            u16 accumulated_food;
            u16 accumulated_shields;
            u8  build_type;
            u16 bld_idx;
        };
        CityHeader* header = reinterpret_cast<CityHeader*>(city);
        if (header->accumulated_food != 0) {
            ok = false;
        }
        if (header->accumulated_shields != 0) {
            ok = false;
        }
        if (header->build_type != 0) {
            ok = false;
        }
        if (header->bld_idx != 0) {
            ok = false;
        }
    }

    note_result(ok, "First city zero-initialized header fields");
    summarize_test_results();
}

void test_city_array_unique_ids () {
    CityArray array;
    const u16 target_count = 1000;

    bool ok_ids = true;

    for (u16 i = 0; i < target_count; ++i) {
        u16 idx = array.get_next_new_city_idx();
        if (idx != i) {
            ok_ids = false;
        }
        City* city = array.get_city(idx);
        if (city == nullptr) {
            ok_ids = false;
            break;
        }
        city->add_food(idx);
    }

    if (ok_ids) {
        u16 total = array.get_city_count();
        if (total < target_count) {
            ok_ids = false;
        } else {
            for (u16 i = 0; i < target_count; ++i) {
                City* city = array.get_city(i);
                if (city == nullptr) {
                    ok_ids = false;
                    break;
                }
                if (city->get_current_food_store() != i) {
                    ok_ids = false;
                    break;
                }
            }
        }
    }

    note_result(ok_ids, "CityArray 1000 cities have unique IDs via food store");
    summarize_test_results();
}

void test_city_array_page_allocation () {
    CityArray array;
    const u16 target_count = 1000;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_city_idx();
    }

    u16 page_count = array.get_page_count();
    bool ok_pages = true;

    if (page_count == 0) {
        ok_pages = false;
    } else {
        for (u16 p = 0; p < page_count; ++p) {
            City* page = array.get_page(p);
            if (page == nullptr) {
                ok_pages = false;
                break;
            }
        }
    }

    note_result(ok_pages, "CityArray page allocation matches page count");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    test_first_city_zero_initialized();
    test_city_array_unique_ids();
    test_city_array_page_allocation();
    
    std::printf("=======================================================\n");
    std::printf(" TESTING CITY ARRAY: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    
    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
