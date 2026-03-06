//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <sstream>

#include "resource_data.h"
#include "tech_data.h"
#include "building_data.h"
#include "building_vector.h"
#include "bit_array.h"

#include "../game_primitives.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef std::string str;

cstr res_path = "../game_config.resources";
cstr tech_path = "../game_config.techs";
cstr bld_path = "../game_config.buildings";

i32 test_count = 0;
i32 test_pass = 0;
i32 total_test_fails = 0;
i32 total_tests_run = 0;
i32 print_level = 0;

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
//=> - Test helper functions -
//================================================================================================================================

struct BuildingVectorTestSetup {
    BuildingVector* bv;
    BitArrayCL* researched;
    u32 count;
    
    BuildingVectorTestSetup () : bv(nullptr), researched(nullptr), count(0) {}
    
    ~BuildingVectorTestSetup () {
        delete bv;
        delete researched;
    }
};

BuildingVectorTestSetup setup_building_vector_test () {
    BuildingVectorTestSetup setup;
    setup.count = static_cast<u32>(BuildingData::get_building_data_count());
    if (setup.count > 0) {
        setup.researched = new BitArrayCL(setup.count);
        setup.bv = new BuildingVector(setup.researched);
    }
    return setup;
}

void verify_building_stats (const BuildingTypeStats& stats, cstr test_name) {
    note_result (!stats.name.empty(), test_name, " - name not empty");
    note_result (stats.cost > 0, test_name, " - cost > 0");
}

void verify_building_built_status (BuildingVector* bv, u32 idx, bool should_be_built, cstr test_name) {
    bool is_built = bv->is_built(idx);
    str msg = str(test_name) + " - is_built " + (should_be_built ? "true" : "false");
    note_result (is_built == should_be_built, msg.c_str());
}

void verify_building_buildable_status (BuildingVector* bv, u32 idx, bool should_be_buildable, cstr test_name) {
    bool is_buildable = bv->is_buildable(idx);
    str msg = str(test_name) + " - is_buildable " + (should_be_buildable ? "true" : "false");
    note_result (is_buildable == should_be_buildable, msg.c_str());
}

void test_building_at_idx (BuildingVector* bv, u32 idx, cstr prefix) {
    const BuildingTypeStats& stats = bv->get_building_stats(idx);
    str test_name = str(prefix) + " - idx " + std::to_string(idx);
    verify_building_stats(stats, test_name.c_str());
    verify_building_built_status(bv, idx, false, test_name.c_str());
}

void test_bounds_get_building_stats (BuildingVector* bv, u32 count, cstr prefix) {
    test_building_at_idx(bv, 0, prefix);
    if (count > 1) {
        test_building_at_idx(bv, count / 2, prefix);
        test_building_at_idx(bv, count - 1, prefix);
    }
}

void test_bounds_set_built (BuildingVector* bv, u32 count, cstr prefix) {
    bv->set_built(0);
    str msg1 = str(prefix) + " - set_built valid idx 0";
    note_result (bv->is_built(0) == true, msg1.c_str());
    
    if (count > 1) {
        bv->set_built(count - 1);
        str msg2 = str(prefix) + " - set_built valid idx (last)";
        note_result (bv->is_built(count - 1) == true, msg2.c_str());
    }
    
    bv->set_built(count);
    str msg3 = str(prefix) + " - set_built out of bounds (idx " + std::to_string(count) + ")";
    note_result (true, msg3.c_str(), " - no crash");
}

void test_bounds_clear_built (BuildingVector* bv, u32 count, cstr prefix) {
    bv->set_built(0);
    bv->clear_built(0);
    str msg1 = str(prefix) + " - clear_built valid idx 0";
    note_result (bv->is_built(0) == false, msg1.c_str());
    
    if (count > 1) {
        bv->set_built(count - 1);
        bv->clear_built(count - 1);
        str msg2 = str(prefix) + " - clear_built valid idx (last)";
        note_result (bv->is_built(count - 1) == false, msg2.c_str());
    }
    
    bv->clear_built(count);
    str msg3 = str(prefix) + " - clear_built out of bounds (idx " + std::to_string(count) + ")";
    note_result (true, msg3.c_str(), " - no crash");
}

void test_bounds_toggle_built (BuildingVector* bv, u32 count, cstr prefix) {
    bool initial = bv->is_built(0);
    bv->toggle_built(0);
    str msg1 = str(prefix) + " - toggle_built valid idx 0";
    note_result (bv->is_built(0) != initial, msg1.c_str());
    
    if (count > 1) {
        bool initial_last = bv->is_built(count - 1);
        bv->toggle_built(count - 1);
        str msg2 = str(prefix) + " - toggle_built valid idx (last)";
        note_result (bv->is_built(count - 1) != initial_last, msg2.c_str());
    }
    
    bv->toggle_built(count);
    str msg3 = str(prefix) + " - toggle_built out of bounds (idx " + std::to_string(count) + ")";
    note_result (true, msg3.c_str(), " - no crash");
}

void test_multiple_buildings (BuildingVector* bv, BitArrayCL* researched, u32 count, cstr prefix) {
    researched->set_bit(0);
    test_building_at_idx(bv, 0, (str(prefix) + " - building idx 0").c_str());
    
    if (count > 2) {
        u32 mid = count / 2;
        researched->set_bit(mid);
        test_building_at_idx(bv, mid, (str(prefix) + " - building idx " + std::to_string(mid)).c_str());
    }
    
    if (count > 1) {
        researched->set_bit(count - 1);
        test_building_at_idx(bv, count - 1, (str(prefix) + " - building idx " + std::to_string(count - 1)).c_str());
    }
}

void test_built_status_calc (BuildingVector* bv, u32 idx, cstr prefix) {
    verify_building_built_status(bv, idx, false, (str(prefix) + " - not built initially").c_str());
    
    bv->set_built(idx);
    verify_building_built_status(bv, idx, true, (str(prefix) + " - built after set_built").c_str());
    
    bv->clear_built(idx);
    verify_building_built_status(bv, idx, false, (str(prefix) + " - not built after clear_built").c_str());
}

void test_repeated_set_built (BuildingVector* bv, u32 idx, cstr prefix) {
    bv->set_built(idx);
    bool status1 = bv->is_built(idx);
    verify_building_built_status(bv, idx, true, (str(prefix) + " - first set_built").c_str());
    
    bv->set_built(idx);
    bool status2 = bv->is_built(idx);
    verify_building_built_status(bv, idx, true, (str(prefix) + " - second set_built").c_str());
    
    bv->set_built(idx);
    bool status3 = bv->is_built(idx);
    verify_building_built_status(bv, idx, true, (str(prefix) + " - third set_built").c_str());
    
    str msg = str(prefix) + " - repeated calls consistent";
    note_result (status1 == status2 && status2 == status3, msg.c_str());
}

void test_get_count_consistency (u32 data_count, BuildingVector* bv, cstr prefix) {
    i32 io_count = static_cast<i32>(data_count);
    u32 bv_count = bv->get_count();
    str msg = str(prefix) + " - get_count matches validate_and_count";
    note_result (static_cast<i32>(bv_count) == io_count, msg.c_str());
}

void test_buildingio_idempotency (cstr filename, cstr prefix) {
    BuildingData::load_static_data(filename);
    u16 count1 = BuildingData::get_building_data_count();
    
    // Loading again should not change the count or corrupt data
    BuildingData::load_static_data(filename);
    u16 count2 = BuildingData::get_building_data_count();
    
    str msg = str(prefix) + " - parse_and_allocate idempotent";
    note_result (count1 == count2, msg.c_str());
}

void test_multiple_indices (BuildingVector* bv, u32 count, cstr prefix) {
    test_building_at_idx(bv, 0, (str(prefix) + " - first").c_str());
    
    if (count > 2) {
        test_building_at_idx(bv, count / 2, (str(prefix) + " - middle").c_str());
    }
    
    if (count > 1) {
        test_building_at_idx(bv, count - 1, (str(prefix) + " - last").c_str());
    }
}

void test_buildable_logic (BuildingVector* bv, BitArrayCL* researched, u32 idx, cstr prefix) {
    verify_building_buildable_status(bv, idx, false, (str(prefix) + " - initial state").c_str());
    researched->set_bit(idx);
    verify_building_buildable_status(bv, idx, false, (str(prefix) + " - researched but not unlocked").c_str());
}

void test_built_independent (BuildingVector* bv, u32 count, cstr prefix) {
    u32 idx_a = 0;
    u32 idx_b = count / 2;
    u32 idx_c = count - 1;
    
    bv->set_built(idx_a);
    bv->set_built(idx_b);
    bv->set_built(idx_c);
    
    verify_building_built_status(bv, idx_a, true, (str(prefix) + " - building A").c_str());
    verify_building_built_status(bv, idx_b, true, (str(prefix) + " - building B").c_str());
    verify_building_built_status(bv, idx_c, true, (str(prefix) + " - building C").c_str());
    
    bv->clear_built(idx_b);
    verify_building_built_status(bv, idx_a, true, (str(prefix) + " - building A still built").c_str());
    verify_building_built_status(bv, idx_b, false, (str(prefix) + " - building B cleared").c_str());
    verify_building_built_status(bv, idx_c, true, (str(prefix) + " - building C still built").c_str());
}

void test_all_not_built_initially (BuildingVector* bv, u32 count, cstr prefix) {
    u32 checked = 0;
    for (u32 i = 0; i < count && checked < 10; i++) {
        str msg = str(prefix) + " - idx " + std::to_string(i);
        verify_building_built_status(bv, i, false, msg.c_str());
        checked++;
    }
}

void test_toggle_built_consistency (BuildingVector* bv, u32 idx, cstr prefix) {
    bool initial = bv->is_built(idx);
    bv->toggle_built(idx);
    bool after_first = bv->is_built(idx);
    note_result (after_first != initial, (str(prefix) + " - first toggle changes state").c_str());
    
    bv->toggle_built(idx);
    bool after_second = bv->is_built(idx);
    note_result (after_second == initial, (str(prefix) + " - second toggle restores state").c_str());
    
    bv->toggle_built(idx);
    bool after_third = bv->is_built(idx);
    note_result (after_third != initial, (str(prefix) + " - third toggle changes state again").c_str());
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_building_io () {
    BuildingData::load_static_data(bld_path);
    i32 count = static_cast<i32>(BuildingData::get_building_data_count());
    note_result (count > 0, "BuildingData validate and count");
    if (print_level > 1) {
        BuildingData::print_content();
    }
    summarize_test_results();
}

void test_building_io_idempotency () {
    test_buildingio_idempotency(bld_path, "BuildingIO idempotency");
    summarize_test_results();
}

void test_building_vector () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    note_result (setup.bv->get_count() == setup.count, "BuildingVec get_count");
    const BuildingTypeStats& stats = setup.bv->get_building_stats(0);
    verify_building_stats(stats, "BuildingVec basic");
    verify_building_built_status(setup.bv, 0, false, "BuildingVec is_built (initial, not built)");
    summarize_test_results();
}

void test_building_vector_built () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    verify_building_built_status(setup.bv, 0, false, "BuildingVec is_built (not built)");
    setup.bv->set_built(0);
    verify_building_built_status(setup.bv, 0, true, "BuildingVec is_built (built)");
    summarize_test_results();
}

void test_building_vector_bounds () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_bounds_get_building_stats(setup.bv, setup.count, "BuildingVec bounds get_building_stats");
    test_bounds_set_built(setup.bv, setup.count, "BuildingVec bounds set_built");
    test_bounds_clear_built(setup.bv, setup.count, "BuildingVec bounds clear_built");
    test_bounds_toggle_built(setup.bv, setup.count, "BuildingVec bounds toggle_built");
    summarize_test_results();
}

void test_building_vector_multiple_indices () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_multiple_indices(setup.bv, setup.count, "BuildingVec multiple indices");
    summarize_test_results();
}

void test_building_vector_multiple_buildings () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_multiple_buildings(setup.bv, setup.researched, setup.count, "BuildingVec multiple buildings");
    summarize_test_results();
}

void test_building_vector_built_status_calc () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_built_status_calc(setup.bv, 0, "BuildingVec built status calc idx 0");
    if (setup.count > 1) {
        test_built_status_calc(setup.bv, setup.count - 1, "BuildingVec built status calc idx last");
    }
    if (setup.count > 2) {
        test_built_status_calc(setup.bv, setup.count / 2, "BuildingVec built status calc idx middle");
    }
    summarize_test_results();
}

void test_building_vector_repeated_set_built () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_repeated_set_built(setup.bv, 0, "BuildingVec repeated set_built idx 0");
    if (setup.count > 1) {
        test_repeated_set_built(setup.bv, setup.count - 1, "BuildingVec repeated set_built idx last");
    }
    summarize_test_results();
}

void test_building_vector_get_count_consistency () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    u32 data_count = static_cast<u32>(BuildingData::get_building_data_count());
    test_get_count_consistency(data_count, setup.bv, "BuildingVec get_count consistency");
    summarize_test_results();
}

void test_building_vector_built_with_different_buildings () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    for (u32 i = 0; i < setup.count && i < 5; i++) {
        verify_building_built_status(setup.bv, i, false, ("BuildingVec locked idx " + std::to_string(i)).c_str());
        
        setup.bv->set_built(i);
        str msg = "BuildingVec built different buildings idx " + std::to_string(i) + " built";
        verify_building_built_status(setup.bv, i, true, msg.c_str());
    }
    summarize_test_results();
}

void test_building_vector_all_not_built_initially () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_all_not_built_initially(setup.bv, setup.count, "BuildingVec all not built initially");
    summarize_test_results();
}

void test_building_vector_built_independent () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    if (setup.count < 3) {
        note_result (false, "BuildingVec built independent test: need at least 3 buildings");
        summarize_test_results();
        return;
    }
    test_built_independent(setup.bv, setup.count, "BuildingVec built independent");
    summarize_test_results();
}

void test_building_vector_toggle_consistency () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_toggle_built_consistency(setup.bv, 0, "BuildingVec toggle consistency idx 0");
    if (setup.count > 1) {
        test_toggle_built_consistency(setup.bv, setup.count - 1, "BuildingVec toggle consistency idx last");
    }
    summarize_test_results();
}

void test_building_vector_save_load () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    setup.bv->set_built(0);
    if (setup.count > 1) {
        setup.bv->set_built(setup.count - 1);
    }
    str filename = "building_vector_test.temp";
    setup.bv->save(filename);
    
    BitArrayCL* researched2 = new BitArrayCL(setup.count);
    BuildingVector bv2(researched2);
    bv2.load(filename);
    
    note_result (bv2.is_built(0) == setup.bv->is_built(0), "BuildingVec save/load consistency idx 0");
    if (setup.count > 1) {
        note_result (bv2.is_built(setup.count - 1) == setup.bv->is_built(setup.count - 1), "BuildingVec save/load consistency");
    }
    
    delete researched2;
    summarize_test_results();
}

void test_building_vector_buildable_logic () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    u32 idx = 0;
    verify_building_buildable_status(setup.bv, idx, false, "BuildingVec buildable logic - initial");
    setup.researched->set_bit(idx);
    verify_building_buildable_status(setup.bv, idx, false, "BuildingVec buildable logic - researched but not unlocked");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    ResourceData::load_static_data(res_path);
    TechData::load_static_data(tech_path);
    BuildingData::load_static_data(bld_path);

    test_building_io();
    test_building_io_idempotency();
    test_building_vector();
    test_building_vector_built();
    test_building_vector_bounds();
    test_building_vector_multiple_indices();
    test_building_vector_multiple_buildings();
    test_building_vector_built_status_calc();
    test_building_vector_repeated_set_built();
    test_building_vector_get_count_consistency();
    test_building_vector_built_with_different_buildings();
    test_building_vector_all_not_built_initially();
    test_building_vector_built_independent();
    test_building_vector_toggle_consistency();
    test_building_vector_save_load();
    test_building_vector_buildable_logic();

    printf("=======================================================\n");
    printf(" TESTING BUILDINGS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
