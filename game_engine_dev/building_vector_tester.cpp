//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <string>
#include <stdexcept>
#include <sstream>

#include "building_vector.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

using std::string;

int test_count = 0;
int test_pass = 0;
bool verbose = true;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

void note_test_result (bool cond, cstr msg) {
    test_count++;
    if (cond) {
        test_pass++;
        if (verbose) {
            printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        printf("*** TEST FAILED: %s\n", msg);
    }
}

void note_test_result (bool cond, cstr msg1, cstr msg2) {
    std::string msg = std::string(msg1) + std::string(msg2);
    note_test_result (cond, msg.c_str());
}

void summarize_test_results () {
    printf("--------------------------------\n");
    printf(" Test count: %d\n", test_count);
    printf(" Test pass: %d\n", test_pass);
    printf(" Test fail: %d\n", test_count - test_pass);
    printf("--------------------------------\n\n\n");

    test_count = 0;
    test_pass = 0;
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_building_io () {
    BuildingIO io("buildings.temp");
    int count = io.validate_and_count();
    note_test_result (count > 0, "BuildingIO validate and count");
    io.parse_and_allocate();
    io.print_content();
    summarize_test_results();
}

void test_building_vector () {
    BuildingIO io("buildings.temp");
    io.parse_and_allocate();
    int count = io.validate_and_count();
    if (count == 0) {
        note_test_result (false, "BuildingVector test: no buildings found");
        summarize_test_results();
        return;
    }
    BuildingVector bv(static_cast<uint32_t>(count));
    note_test_result (bv.get_count() == count, "BuildingVector get_count");
    BuildingData data = bv.get_building(0);
    note_test_result (!data.name.empty(), "BuildingVector get_building name");
    note_test_result (data.cost > 0, "BuildingVector get_building cost");
    note_test_result (!data.effect.empty(), "BuildingVector get_building effect");
    note_test_result (data.exists == false, "BuildingVector get_building exists (initial)");
    summarize_test_results();
}

void test_building_vector_save_load () {
    BuildingIO io("buildings.temp");
    io.parse_and_allocate();
    int count = io.validate_and_count();
    if (count == 0) {
        note_test_result (false, "BuildingVector save/load test: no buildings found");
        summarize_test_results();
        return;
    }
    BuildingVector bv1(static_cast<uint32_t>(count));
    bv1.get_building(0);
    string filename = "building_vector_test.temp";
    bv1.save(filename);
    BuildingVector bv2(static_cast<uint32_t>(count));
    bv2.load(filename);
    BuildingData data1 = bv1.get_building(0);
    BuildingData data2 = bv2.get_building(0);
    note_test_result (data1.exists == data2.exists, "BuildingVector save/load consistency");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main () {
    test_building_io();
    test_building_vector();
    test_building_vector_save_load();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
