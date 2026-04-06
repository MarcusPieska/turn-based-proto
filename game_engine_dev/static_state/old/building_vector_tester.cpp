//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <sstream>

#include "city_flags.h"
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

cstr city_flag_path = "../game_config.city_flags";
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
    BuiltBuildings* built;
    BuildableBuildings* buildable;
    u32 count;
    
    BuildingVectorTestSetup () : built(nullptr), buildable(nullptr), count(0) {}
    
    ~BuildingVectorTestSetup () {
        delete built;
        delete buildable;
    }
};

BuildingVectorTestSetup setup_building_vector_test () {
    BuildingVectorTestSetup setup;
    setup.count = static_cast<u32>(BuildingData::get_building_data_count());
    if (setup.count > 0) {
        setup.built = new BuiltBuildings();
        setup.buildable = new BuildableBuildings();
    }
    return setup;
}

void verify_building_stats (const BuildingTypeStats& stats, cstr test_name) {
    note_result (!stats.name.empty(), test_name, " - name not empty");
    note_result (stats.cost > 0, test_name, " - cost > 0");
}

void verify_building_built_status (BuiltBuildings* built, u32 idx, bool should_be_built, cstr test_name) {
    bool is_built = built->has_been_built(idx);
    str msg = str(test_name) + " - has_been_built " + (should_be_built ? "true" : "false");
    note_result (is_built == should_be_built, msg.c_str());
}

void verify_building_buildable_status (BuildableBuildings* buildable, u32 idx, bool should_be_buildable, cstr test_name) {
    bool can_build = buildable->can_build(idx);
    str msg = str(test_name) + " - can_build " + (should_be_buildable ? "true" : "false");
    note_result (can_build == should_be_buildable, msg.c_str());
}

void test_building_at_idx (BuiltBuildings* built, u32 idx, cstr prefix) {
    const BuildingTypeStats* data = BuildingData::get_building_data_array();
    const BuildingTypeStats& stats = data[idx];
    str test_name = str(prefix) + " - idx " + std::to_string(idx);
    verify_building_stats(stats, test_name.c_str());
    verify_building_built_status(built, idx, false, test_name.c_str());
}

void test_bounds_get_building_stats (BuiltBuildings* built, u32 count, cstr prefix) {
    test_building_at_idx(built, 0, prefix);
    if (count > 1) {
        test_building_at_idx(built, count / 2, prefix);
        test_building_at_idx(built, count - 1, prefix);
    }
}

void test_bounds_set_built (BuiltBuildings* built, u32 count, cstr prefix) {
    built->set_built(0);
    str msg1 = str(prefix) + " - set_built valid idx 0";
    note_result (built->has_been_built(0) == true, msg1.c_str());
    
    if (count > 1) {
        built->set_built(count - 1);
        str msg2 = str(prefix) + " - set_built valid idx (last)";
        note_result (built->has_been_built(count - 1) == true, msg2.c_str());
    }
    
    built->set_built(count);
    str msg3 = str(prefix) + " - set_built out of bounds (idx " + std::to_string(count) + ")";
    note_result (true, msg3.c_str(), " - no crash");
}

void test_bounds_clear_built (BuiltBuildings* built, u32 count, cstr prefix) {
    built->set_built(0);
    built->clear_built(0);
    str msg1 = str(prefix) + " - clear_built valid idx 0";
    note_result (built->has_been_built(0) == false, msg1.c_str());
    
    if (count > 1) {
        built->set_built(count - 1);
        built->clear_built(count - 1);
        str msg2 = str(prefix) + " - clear_built valid idx (last)";
        note_result (built->has_been_built(count - 1) == false, msg2.c_str());
    }
    
    built->clear_built(count);
    str msg3 = str(prefix) + " - clear_built out of bounds (idx " + std::to_string(count) + ")";
    note_result (true, msg3.c_str(), " - no crash");
}

void test_bounds_toggle_built (BuiltBuildings* built, u32 count, cstr prefix) {
    bool initial = built->has_been_built(0);
    if (initial) {
        built->clear_built(0);
    } else {
        built->set_built(0);
    }
    str msg1 = str(prefix) + " - toggle_built valid idx 0";
    note_result (built->has_been_built(0) != initial, msg1.c_str());
    
    if (count > 1) {
        bool initial_last = built->has_been_built(count - 1);
        if (initial_last) {
            built->clear_built(count - 1);
        } else {
            built->set_built(count - 1);
        }
        str msg2 = str(prefix) + " - toggle_built valid idx (last)";
        note_result (built->has_been_built(count - 1) != initial_last, msg2.c_str());
    }
    
    bool initial_oob = built->has_been_built(count);
    if (initial_oob) {
        built->clear_built(count);
    } else {
        built->set_built(count);
    }
    str msg3 = str(prefix) + " - toggle_built out of bounds (idx " + std::to_string(count) + ")";
    note_result (true, msg3.c_str(), " - no crash");
}

void test_multiple_buildings (BuiltBuildings* built, u32 count, cstr prefix) {
    test_building_at_idx(built, 0, (str(prefix) + " - building idx 0").c_str());
    
    if (count > 2) {
        u32 mid = count / 2;
        test_building_at_idx(built, mid, (str(prefix) + " - building idx " + std::to_string(mid)).c_str());
    }
    
    if (count > 1) {
        test_building_at_idx(built, count - 1, (str(prefix) + " - building idx " + std::to_string(count - 1)).c_str());
    }
}

void test_built_status_calc (BuiltBuildings* built, u32 idx, cstr prefix) {
    verify_building_built_status(built, idx, false, (str(prefix) + " - not built initially").c_str());
    
    built->set_built(idx);
    verify_building_built_status(built, idx, true, (str(prefix) + " - built after set_built").c_str());
    
    built->clear_built(idx);
    verify_building_built_status(built, idx, false, (str(prefix) + " - not built after clear_built").c_str());
}

void test_repeated_set_built (BuiltBuildings* built, u32 idx, cstr prefix) {
    built->set_built(idx);
    bool status1 = built->has_been_built(idx);
    verify_building_built_status(built, idx, true, (str(prefix) + " - first set_built").c_str());
    
    built->set_built(idx);
    bool status2 = built->has_been_built(idx);
    verify_building_built_status(built, idx, true, (str(prefix) + " - second set_built").c_str());
    
    built->set_built(idx);
    bool status3 = built->has_been_built(idx);
    verify_building_built_status(built, idx, true, (str(prefix) + " - third set_built").c_str());
    
    str msg = str(prefix) + " - repeated calls consistent";
    note_result (status1 == status2 && status2 == status3, msg.c_str());
}

void test_get_count_consistency (u32 data_count, u32 actual_count, cstr prefix) {
    i32 io_count = static_cast<i32>(data_count);
    i32 actual = static_cast<i32>(actual_count);
    str msg = str(prefix) + " - get_count matches validate_and_count";
    note_result (actual == io_count, msg.c_str());
}

void test_buildingio_idempotency (cstr filename, cstr prefix) {
    BuildingData::load_static_data(filename);
    u16 count1 = BuildingData::get_building_data_count();
    
    BuildingData::load_static_data(filename);
    u16 count2 = BuildingData::get_building_data_count();
    
    str msg = str(prefix) + " - parse_and_allocate idempotent";
    note_result (count1 == count2, msg.c_str());
}

void test_multiple_indices (BuiltBuildings* built, u32 count, cstr prefix) {
    test_building_at_idx(built, 0, (str(prefix) + " - first").c_str());
    
    if (count > 2) {
        test_building_at_idx(built, count / 2, (str(prefix) + " - middle").c_str());
    }
    
    if (count > 1) {
        test_building_at_idx(built, count - 1, (str(prefix) + " - last").c_str());
    }
}

void test_buildable_logic (BuildableBuildings* buildable, u32 idx, cstr prefix) {
    verify_building_buildable_status(buildable, idx, false, (str(prefix) + " - initial state").c_str());
    verify_building_buildable_status(buildable, idx, false, (str(prefix) + " - not set buildable").c_str());
}

void test_built_independent (BuiltBuildings* built, u32 count, cstr prefix) {
    u32 idx_a = 0;
    u32 idx_b = count / 2;
    u32 idx_c = count - 1;
    
    built->set_built(idx_a);
    built->set_built(idx_b);
    built->set_built(idx_c);
    
    verify_building_built_status(built, idx_a, true, (str(prefix) + " - building A").c_str());
    verify_building_built_status(built, idx_b, true, (str(prefix) + " - building B").c_str());
    verify_building_built_status(built, idx_c, true, (str(prefix) + " - building C").c_str());
    
    built->clear_built(idx_b);
    verify_building_built_status(built, idx_a, true, (str(prefix) + " - building A still built").c_str());
    verify_building_built_status(built, idx_b, false, (str(prefix) + " - building B cleared").c_str());
    verify_building_built_status(built, idx_c, true, (str(prefix) + " - building C still built").c_str());
}

void test_all_not_built_initially (BuiltBuildings* built, u32 count, cstr prefix) {
    u32 checked = 0;
    for (u32 i = 0; i < count && checked < 10; i++) {
        str msg = str(prefix) + " - idx " + std::to_string(i);
        verify_building_built_status(built, i, false, msg.c_str());
        checked++;
    }
}

void test_toggle_built_consistency (BuiltBuildings* built, u32 idx, cstr prefix) {
    bool initial = built->has_been_built(idx);
    if (initial) {
        built->clear_built(idx);
    } else {
        built->set_built(idx);
    }
    bool after_first = built->has_been_built(idx);
    note_result (after_first != initial, (str(prefix) + " - first toggle changes state").c_str());
    
    if (after_first) {
        built->clear_built(idx);
    } else {
        built->set_built(idx);
    }
    bool after_second = built->has_been_built(idx);
    note_result (after_second == initial, (str(prefix) + " - second toggle restores state").c_str());
    
    if (after_second) {
        built->clear_built(idx);
    } else {
        built->set_built(idx);
    }
    bool after_third = built->has_been_built(idx);
    note_result (after_third != initial, (str(prefix) + " - third toggle changes state again").c_str());
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_static_data_loaded_resource_data () {
    u16 count = ResourceData::get_resource_data_count();
    note_result (count > 2, "Static data validation: ResourceData count > 2");
    summarize_test_results();
}

void test_static_data_loaded_tech_data () {
    u16 count = TechData::get_tech_data_count();
    note_result (count > 2, "Static data validation: TechData count > 2");
    summarize_test_results();
}

void test_static_data_loaded_building_data () {
    u16 count = BuildingData::get_building_data_count();
    note_result (count > 2, "Static data validation: BuildingData count > 2");
    summarize_test_results();
}

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
    u32 data_count = static_cast<u32>(BuildingData::get_building_data_count());
    note_result (data_count == setup.count, "BuildingVec count");
    const BuildingTypeStats* data = BuildingData::get_building_data_array();
    const BuildingTypeStats& stats = data[0];
    verify_building_stats(stats, "BuildingVec basic");
    verify_building_built_status(setup.built, 0, false, "BuildingVec has_been_built (initial, not built)");
    summarize_test_results();
}

void test_building_vector_built () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    verify_building_built_status(setup.built, 0, false, "BuildingVec has_been_built (not built)");
    setup.built->set_built(0);
    verify_building_built_status(setup.built, 0, true, "BuildingVec has_been_built (built)");
    summarize_test_results();
}

void test_building_vector_bounds () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_bounds_get_building_stats(setup.built, setup.count, "BuildingVec bounds get_building_stats");
    test_bounds_set_built(setup.built, setup.count, "BuildingVec bounds set_built");
    test_bounds_clear_built(setup.built, setup.count, "BuildingVec bounds clear_built");
    test_bounds_toggle_built(setup.built, setup.count, "BuildingVec bounds toggle_built");
    summarize_test_results();
}

void test_building_vector_multiple_indices () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_multiple_indices(setup.built, setup.count, "BuildingVec multiple indices");
    summarize_test_results();
}

void test_building_vector_multiple_buildings () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_multiple_buildings(setup.built, setup.count, "BuildingVec multiple buildings");
    summarize_test_results();
}

void test_building_vector_built_status_calc () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_built_status_calc(setup.built, 0, "BuildingVec built status calc idx 0");
    if (setup.count > 1) {
        test_built_status_calc(setup.built, setup.count - 1, "BuildingVec built status calc idx last");
    }
    if (setup.count > 2) {
        test_built_status_calc(setup.built, setup.count / 2, "BuildingVec built status calc idx middle");
    }
    summarize_test_results();
}

void test_building_vector_repeated_set_built () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_repeated_set_built(setup.built, 0, "BuildingVec repeated set_built idx 0");
    if (setup.count > 1) {
        test_repeated_set_built(setup.built, setup.count - 1, "BuildingVec repeated set_built idx last");
    }
    summarize_test_results();
}

void test_building_vector_get_count_consistency () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    u32 data_count = static_cast<u32>(BuildingData::get_building_data_count());
    test_get_count_consistency(data_count, setup.count, "BuildingVec get_count consistency");
    summarize_test_results();
}

void test_building_vector_built_with_different_buildings () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    for (u32 i = 0; i < setup.count && i < 5; i++) {
        verify_building_built_status(setup.built, i, false, ("BuildingVec locked idx " + std::to_string(i)).c_str());
        
        setup.built->set_built(i);
        str msg = "BuildingVec built different buildings idx " + std::to_string(i) + " built";
        verify_building_built_status(setup.built, i, true, msg.c_str());
    }
    summarize_test_results();
}

void test_building_vector_all_not_built_initially () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_all_not_built_initially(setup.built, setup.count, "BuildingVec all not built initially");
    summarize_test_results();
}

void test_building_vector_built_independent () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    if (setup.count < 3) {
        note_result (false, "BuildingVec built independent test: need at least 3 buildings");
        summarize_test_results();
        return;
    }
    test_built_independent(setup.built, setup.count, "BuildingVec built independent");
    summarize_test_results();
}

void test_building_vector_toggle_consistency () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    test_toggle_built_consistency(setup.built, 0, "BuildingVec toggle consistency idx 0");
    if (setup.count > 1) {
        test_toggle_built_consistency(setup.built, setup.count - 1, "BuildingVec toggle consistency idx last");
    }
    summarize_test_results();
}

void test_building_vector_save_load () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    setup.built->set_built(0);
    if (setup.count > 1) {
        setup.built->set_built(setup.count - 1);
    }
    str filename = "building_vector_test.temp";
    setup.built->save(filename);
    
    BuiltBuildings built2;
    built2.load(filename);
    
    note_result (built2.has_been_built(0) == setup.built->has_been_built(0), "BuildingVec save/load consistency idx 0");
    if (setup.count > 1) {
        note_result (built2.has_been_built(setup.count - 1) == setup.built->has_been_built(setup.count - 1), "BuildingVec save/load consistency");
    }
    
    summarize_test_results();
}

void test_building_vector_buildable_logic () {
    BuildingVectorTestSetup setup = setup_building_vector_test();
    u32 idx = 0;
    verify_building_buildable_status(setup.buildable, idx, false, "BuildingVec buildable logic - initial");
    verify_building_buildable_status(setup.buildable, idx, false, "BuildingVec buildable logic - not set buildable");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    CityFlagData::load_static_data(city_flag_path);
    TechData::load_static_data(tech_path);
    ResourceData::load_static_data(res_path);
    BuildingData::load_static_data(bld_path);

    test_static_data_loaded_resource_data();
    test_static_data_loaded_tech_data();
    test_static_data_loaded_building_data();
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
