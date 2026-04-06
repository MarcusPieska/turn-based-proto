//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "tile_array.h"

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

void test_tile_array_corner_pointers () {
    bool ok = TileArray::init(64, 64);
    Tile* top_left = TileArray::get_tile(0, 0);
    Tile* top_right = TileArray::get_tile(63, 0);
    Tile* bottom_left = TileArray::get_tile(0, 63);
    Tile* bottom_right = TileArray::get_tile(63, 63);

    ok = ok &&
        top_left != nullptr &&
        top_right != nullptr &&
        bottom_left != nullptr &&
        bottom_right != nullptr &&
        top_left != top_right &&
        top_left != bottom_left &&
        top_left != bottom_right &&
        top_right != bottom_left &&
        top_right != bottom_right &&
        bottom_left != bottom_right;

    note_result(ok, "TileArray corners map to unique tile pointers");
    summarize_test_results();
}

void test_add_type_roundtrip_resource () {
    const u16 width = 32;
    const u16 height = 32;
    bool ok_init = TileArray::init(width, height);
    note_result(ok_init, "TileArray init for resource roundtrip");

    bool ok_empty = true;
    for (u16 y = 0; y < height; ++y) {
        for (u16 x = 0; x < width; ++x) {
            BuildAddItem item = TileArray::get_build_adds(x, y);
            if (item.build_add_type != BUILD_ADD_NONE || item.resource_idx != 0) {
                ok_empty = false;
            }
        }
    }
    note_result(ok_empty, "No adds present before resource population");

    u16 add_value = 0;
    for (u16 y = 0; y < height; ++y) {
        for (u16 x = 0; x < width; ++x) {
            TileArray::add_resource(x, y, add_value);
            add_value = static_cast<u16>(add_value + 1);
        }
    }

    add_value = 0;
    for (u16 y = 0; y < height; ++y) {
        for (u16 x = 0; x < width; ++x) {
            BuildAddItem item = TileArray::get_build_adds(x, y);
            bool ok = (item.build_add_type == BUILD_ADD_NONE) &&
                (item.resource_idx == add_value);
            note_result(ok, "Resource add roundtrip at tile");
            add_value = static_cast<u16>(add_value + 1);
        }
    }

    summarize_test_results();
}

void test_add_type_roundtrip_city () {
    const u16 width = 32;
    const u16 height = 32;
    bool ok_init = TileArray::init(width, height);
    note_result(ok_init, "TileArray init for city roundtrip");

    bool ok_empty = true;
    for (u16 y = 0; y < height; ++y) {
        for (u16 x = 0; x < width; ++x) {
            BuildAddItem item = TileArray::get_build_adds(x, y);
            if (item.build_add_type != BUILD_ADD_NONE || item.resource_idx != 0) {
                ok_empty = false;
            }
        }
    }
    note_result(ok_empty, "No adds present before city population");

    u16 add_value = 0;
    for (u16 y = 0; y < height; ++y) {
        for (u16 x = 0; x < width; ++x) {
            TileArray::add_city(x, y, add_value);
            add_value = static_cast<u16>(add_value + 1);
        }
    }

    add_value = 0;
    for (u16 y = 0; y < height; ++y) {
        for (u16 x = 0; x < width; ++x) {
            BuildAddItem item = TileArray::get_build_adds(x, y);
            bool ok = (item.build_add_type == BUILD_ADD_CITY) &&
                (item.build_add_idx == add_value);
            note_result(ok, "City add roundtrip at tile");
            add_value = static_cast<u16>(add_value + 1);
        }
    }

    summarize_test_results();
}

void test_add_type_roundtrip_fort () {
    const u16 width = 32;
    const u16 height = 32;
    bool ok_init = TileArray::init(width, height);
    note_result(ok_init, "TileArray init for fort roundtrip");

    bool ok_empty = true;
    for (u16 y = 0; y < height; ++y) {
        for (u16 x = 0; x < width; ++x) {
            BuildAddItem item = TileArray::get_build_adds(x, y);
            if (item.build_add_type != BUILD_ADD_NONE || item.resource_idx != 0) {
                ok_empty = false;
            }
        }
    }
    note_result(ok_empty, "No adds present before fort population");

    u16 add_value = 0;
    for (u16 y = 0; y < height; ++y) {
        for (u16 x = 0; x < width; ++x) {
            TileArray::add_fort(x, y, add_value);
            add_value = static_cast<u16>(add_value + 1);
        }
    }

    add_value = 0;
    for (u16 y = 0; y < height; ++y) {
        for (u16 x = 0; x < width; ++x) {
            BuildAddItem item = TileArray::get_build_adds(x, y);
            bool ok = (item.build_add_type == BUILD_ADD_FORT) &&
                (item.build_add_idx == add_value);
            note_result(ok, "Fort add roundtrip at tile");
            add_value = static_cast<u16>(add_value + 1);
        }
    }

    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    test_tile_array_corner_pointers();
    test_add_type_roundtrip_resource();
    test_add_type_roundtrip_city();
    test_add_type_roundtrip_fort();
    
    std::printf("=======================================================\n");
    std::printf(" TESTING TILE ARRAY: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    
    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
