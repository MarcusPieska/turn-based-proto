//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <string>
#include <stdexcept>
#include <sstream>

#include "building_vector.h"
#include "bit_array.h"

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
    BuildingIO io("../game_config.buildings");
    int count = io.validate_and_count();
    note_test_result (count > 0, "BuildingIO validate and count");
    io.parse_and_allocate();
    io.print_content();
    summarize_test_results();
}

void test_building_vector () {
    BuildingIO io("../game_config.buildings");
    io.parse_and_allocate();
    int count = io.validate_and_count();
    if (count == 0) {
        note_test_result (false, "BuildingVector test: no buildings found");
        summarize_test_results();
        return;
    }
    BitArrayCL* buildings_unlocked = new BitArrayCL(static_cast<uint32_t>(count));
    BuildingVector bv(static_cast<uint32_t>(count), buildings_unlocked);
    note_test_result (bv.get_count() == count, "BuildingVector get_count");
    BuildingData data = bv.get_building(0);
    note_test_result (!data.name.empty(), "BuildingVector get_building name");
    note_test_result (data.cost > 0, "BuildingVector get_building cost");
    note_test_result (!data.effect.empty(), "BuildingVector get_building effect");
    note_test_result (data.exists == false, "BuildingVector get_building exists (initial)");
    delete buildings_unlocked;
    summarize_test_results();
}

void test_building_vector_save_load () {
    BuildingIO io("../game_config.buildings");
    io.parse_and_allocate();
    int count = io.validate_and_count();
    if (count == 0) {
        note_test_result (false, "BuildingVector save/load test: no buildings found");
        summarize_test_results();
        return;
    }
    BitArrayCL* buildings_unlocked1 = new BitArrayCL(static_cast<uint32_t>(count));
    BuildingVector bv1(static_cast<uint32_t>(count), buildings_unlocked1);
    bv1.get_building(0);
    string filename = "building_vector_test.temp";
    bv1.save(filename);
    BitArrayCL* buildings_unlocked2 = new BitArrayCL(static_cast<uint32_t>(count));
    BuildingVector bv2(static_cast<uint32_t>(count), buildings_unlocked2);
    bv2.load(filename);
    BuildingData data1 = bv1.get_building(0);
    BuildingData data2 = bv2.get_building(0);
    note_test_result (data1.exists == data2.exists, "BuildingVector save/load consistency");
    delete buildings_unlocked1;
    delete buildings_unlocked2;
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
