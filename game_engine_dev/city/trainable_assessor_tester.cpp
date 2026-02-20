//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "unit_vector.h"
#include "resources_vector.h"
#include "trainable_assessor.h"
#include "bit_array.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;
typedef std::string str;
typedef uint32_t u32;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

void note_test_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            printf("*** TEST PASSED: %s\n", msg);
        }
    } else if (print_level > 0) {
        total_test_fails++;
        printf("*** TEST FAILED: %s\n", msg);
    }
}

void note_test_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_test_result (cond, msg.c_str());
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

void setup_static_data () {
    UnitIO unit_io("../game_config_test.units");
    unit_io.parse_and_allocate();
    
    ResourceIO resource_io("../game_config_test.resources");
    resource_io.parse_and_allocate();
}

static u32 find_unit_index (const char* unit_name) {
    const UnitTypeStats* unit_array = UnitVector::get_unit_data_array();
    u32 unit_count = UnitVector::get_unit_data_count();
    for (u32 i = 0; i < unit_count; i++) {
        if (unit_array[i].name == unit_name) {
            return i;
        }
    }
    return UINT32_MAX;
}

static u32 find_resource_index (const char* resource_name) {
    const ResourceTypeStats* resource_array = ResourceVector::get_resource_data_array();
    u32 resource_count = ResourceVector::get_resource_data_count();
    for (u32 i = 0; i < resource_count; i++) {
        if (resource_array[i].name == resource_name) {
            return i;
        }
    }
    return UINT32_MAX;
}

struct TestSetup {
    BitArrayCL* techs;
    BitArrayCL* resources;
    UnitVector* units;
    ResourceVector* resource_vec;
    TrainableAssessor* assessor;
    
    TestSetup () : techs(nullptr), resources(nullptr), units(nullptr), resource_vec(nullptr), assessor(nullptr) {}
    
    ~TestSetup () {
        delete assessor;
        delete resource_vec;
        delete units;
        delete resources;
        delete techs;
    }
};

TestSetup create_test_setup () {
    TestSetup setup;
    setup_static_data();
    
    u32 unit_count = UnitVector::get_unit_data_count();
    u32 resource_count = ResourceVector::get_resource_data_count();
    
    setup.techs = new BitArrayCL(unit_count);
    setup.resources = new BitArrayCL(resource_count);
    setup.units = new UnitVector(setup.techs);
    setup.resource_vec = new ResourceVector(resource_count, setup.resources);
    setup.assessor = new TrainableAssessor();
    setup.assessor->load_unit_resource_costs("../game_config_test.unit_res_cost");
    
    return setup;
}

void set_resource_available (ResourceVector* rv, const char* resource_name) {
    u32 idx = find_resource_index(resource_name);
    if (idx != UINT32_MAX) {
        rv->set_available(idx);
    }
}

void set_resource_available_by_index (ResourceVector* rv, u32 idx) {
    rv->set_available(idx);
}

void set_unit_researched (BitArrayCL* techs, const char* unit_name) {
    u32 idx = find_unit_index(unit_name);
    if (idx != UINT32_MAX) {
        techs->set_bit(idx);
    }
}

bool is_unit_trainable (UnitVector* uv, const char* unit_name) {
    u32 idx = find_unit_index(unit_name);
    if (idx != UINT32_MAX) {
        return uv->is_trainable(idx);
    }
    return false;
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_trainable_assessor_construction () {
    setup_static_data();
    TrainableAssessor assessor;
    note_test_result (true, "TrainableAssessor construction");
    summarize_test_results();
}

void test_trainable_assessor_load_and_print () {
    setup_static_data();
    TrainableAssessor assessor;
    assessor.load_unit_resource_costs("../game_config_test.unit_res_cost");
    if (print_level > 0) {
        printf("Unit Resource Costs:\n");
        assessor.print_unit_resource_costs();
    }
    note_test_result (true, "TrainableAssessor load and print");
    summarize_test_results();
}

void test_swordsman_requires_iron () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Swordsman");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Swordsman"), "Swordsman not trainable w/o Iron");
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Swordsman"), "Swordsman trainable with Iron");
    summarize_test_results();
}

void test_legion_requires_iron () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Legion");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Legion"), "Legion not trainable w/o Iron");
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Legion"), "Legion trainable with Iron");
    summarize_test_results();
}

void test_immortal_requires_iron () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Immortal");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Immortal"), "Immortal not trainable w/o Iron");
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Immortal"), "Immortal trainable with Iron");
    summarize_test_results();
}

void test_chariot_requires_horses () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Chariot");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Chariot"), "Chariot not trainable w/o Horses");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Chariot"), "Chariot trainable with Horses");
    summarize_test_results();
}

void test_horseman_requires_horses () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Horseman");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Horseman"), "Horseman not trainable w/o Horses");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Horseman"), "Horseman trainable with Horses");
    summarize_test_results();
}

void test_knight_requires_horses_and_iron () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Knight");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Knight"), "Knight not trainable w/o resources");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Knight"), "Knight not trainable with only Horses");
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Knight"), "Knight trainable with Horses and Iron");
    summarize_test_results();
}

void test_cavalry_requires_horses () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Cavalry");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Cavalry"), "Cavalry not trainable w/o Horses");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Cavalry"), "Cavalry trainable with Horses");
    summarize_test_results();
}

void test_musketman_requires_saltpeter () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Musketman");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Musketman"), "Musketman not trainable w/o Saltpeter");
    
    set_resource_available(setup.resource_vec, "Saltpeter");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Musketman"), "Musketman trainable with Saltpeter");
    summarize_test_results();
}

void test_rifleman_requires_saltpeter () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Rifleman");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Rifleman"), "Rifleman not trainable w/o Saltpeter");
    
    set_resource_available(setup.resource_vec, "Saltpeter");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Rifleman"), "Rifleman trainable with Saltpeter");
    summarize_test_results();
}

void test_cannon_requires_saltpeter () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Cannon");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Cannon"), "Cannon not trainable w/o Saltpeter");
    
    set_resource_available(setup.resource_vec, "Saltpeter");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Cannon"), "Cannon trainable with Saltpeter");
    summarize_test_results();
}

void test_artillery_requires_saltpeter () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Artillery");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Artillery"), "Artillery not trainable w/o Saltpeter");
    
    set_resource_available(setup.resource_vec, "Saltpeter");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Artillery"), "Artillery trainable with Saltpeter");
    summarize_test_results();
}

void test_tank_requires_oil () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Tank");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Tank"), "Tank not trainable w/o Oil");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Tank"), "Tank trainable with Oil");
    summarize_test_results();
}

void test_modern_armor_requires_oil_and_aluminum () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Modern Armor");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Modern Armor"), "Modern Armor not trainable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Modern Armor"), "Modern Armor not trainable with only Oil");
    
    set_resource_available(setup.resource_vec, "Aluminum");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Modern Armor"), "Modern Armor trainable with Oil and Aluminum");
    summarize_test_results();
}

void test_fighter_requires_oil_rubber_aluminum () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Fighter");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Fighter"), "Fighter not trainable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Fighter"), "Fighter not trainable with only Oil");
    
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Fighter"), "Fighter not trainable with Oil and Rubber");
    
    set_resource_available(setup.resource_vec, "Aluminum");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Fighter"), "Fighter trainable with Oil, Rubber, and Aluminum");
    summarize_test_results();
}

void test_bomber_requires_oil_rubber () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Bomber");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Bomber"), "Bomber not trainable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Bomber"), "Bomber not trainable with only Oil");
    
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Bomber"), "Bomber trainable with Oil and Rubber");
    summarize_test_results();
}

void test_stealth_fighter_requires_oil_rubber_aluminum () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Stealth Fighter");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Stealth Fighter"), "Stealth Fighter not trainable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Stealth Fighter"), "Stealth Fighter not trainable w/o Aluminum");
    
    set_resource_available(setup.resource_vec, "Aluminum");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Stealth Fighter"), "Stealth Fighter trainable with all resources");
    summarize_test_results();
}

void test_nuclear_submarine_requires_uranium () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Nuclear Submarine");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Nuclear Submarine"), "Nuclear Submarine not trainable w/o Uranium");
    
    set_resource_available(setup.resource_vec, "Uranium");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Nuclear Submarine"), "Nuclear Submarine trainable with Uranium");
    summarize_test_results();
}

void test_tactical_nuke_requires_uranium () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Tactical Nuke");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Tactical Nuke"), "Tactical Nuke not trainable w/o Uranium");
    
    set_resource_available(setup.resource_vec, "Uranium");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Tactical Nuke"), "Tactical Nuke trainable with Uranium");
    summarize_test_results();
}

void test_nuclear_missile_requires_uranium () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Nuclear Missile");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Nuclear Missile"), "Nuclear Missile not trainable w/o Uranium");
    
    set_resource_available(setup.resource_vec, "Uranium");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Nuclear Missile"), "Nuclear Missile trainable with Uranium");
    summarize_test_results();
}

void test_unit_without_resources_needs_City () {
    TestSetup setup = create_test_setup();
    u32 settler_idx = find_unit_index("Settler");
    if (settler_idx != UINT32_MAX) {
        set_unit_researched(setup.techs, "Settler");
        
        setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
        note_test_result (!is_unit_trainable(setup.units, "Settler"), "Settler not trainable w/o City");
        
        set_resource_available_by_index(setup.resource_vec, 0);
        setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
        note_test_result (is_unit_trainable(setup.units, "Settler"), "Settler trainable with City");
    }
    summarize_test_results();
}

void test_multiple_units_same_resource () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Swordsman");
    set_unit_researched(setup.techs, "Legion");
    set_unit_researched(setup.techs, "Immortal");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Swordsman"), "Multiple units not trainable w/o Iron");
    note_test_result (!is_unit_trainable(setup.units, "Legion"), "Multiple units not trainable w/o Iron");
    note_test_result (!is_unit_trainable(setup.units, "Immortal"), "Multiple units not trainable w/o Iron");
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Swordsman"), "Swordsman trainable with Iron");
    note_test_result (is_unit_trainable(setup.units, "Legion"), "Legion trainable with Iron");
    note_test_result (is_unit_trainable(setup.units, "Immortal"), "Immortal trainable with Iron");
    summarize_test_results();
}

void test_unit_not_researched_not_trainable () {
    TestSetup setup = create_test_setup();
    set_resource_available_by_index(setup.resource_vec, 0);
    set_resource_available(setup.resource_vec, "Iron");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Swordsman"), "Swordsman not trainable if not researched");
    
    set_unit_researched(setup.techs, "Swordsman");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Swordsman"), "Swordsman trainable when researched");
    summarize_test_results();
}

void test_all_horse_units () {
    TestSetup setup = create_test_setup();
    const char* horse_units[] = {"Chariot", "Horseman", "Cavalry", "Ansar Warrior", "Conquistador", "Rider", "Cossack", "War Elephant"};
    set_resource_available_by_index(setup.resource_vec, 0);
    
    for (int i = 0; i < 8; i++) {
        set_unit_researched(setup.techs, horse_units[i]);
    }
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 8; i++) {
        str msg = str("Horse unit ") + str(horse_units[i]) + " not trainable w/o Horses";
        note_test_result (!is_unit_trainable(setup.units, horse_units[i]), msg.c_str());
    }
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 8; i++) {
        str msg = str("Horse unit ") + str(horse_units[i]) + " trainable with Horses";
        note_test_result (is_unit_trainable(setup.units, horse_units[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_all_oil_units () {
    TestSetup setup = create_test_setup();
    const char* oil_units[] = {"Tank", "Panzer", "Destroyer", "Battleship", "Cruiser", "AEGIS Cruiser"};
    set_resource_available_by_index(setup.resource_vec, 0);
    
    for (int i = 0; i < 6; i++) {
        set_unit_researched(setup.techs, oil_units[i]);
    }
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 6; i++) {
        str msg = str("Oil unit ") + str(oil_units[i]) + " not trainable w/o Oil";
        note_test_result (!is_unit_trainable(setup.units, oil_units[i]), msg.c_str());
    }
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 6; i++) {
        str msg = str("Oil unit ") + str(oil_units[i]) + " trainable with Oil";
        note_test_result (is_unit_trainable(setup.units, oil_units[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_f15_requires_oil_rubber_aluminum () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "F-15");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "F-15"), "F-15 not trainable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "F-15"), "F-15 not trainable w/o Aluminum");
    
    set_resource_available(setup.resource_vec, "Aluminum");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "F-15"), "F-15 trainable with all resources");
    summarize_test_results();
}

void test_helicopter_requires_oil_rubber () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Helicopter");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Helicopter"), "Helicopter not trainable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Helicopter"), "Helicopter not trainable with only Oil");
    
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Helicopter"), "Helicopter trainable with Oil and Rubber");
    summarize_test_results();
}

void test_mech_infantry_requires_oil_rubber () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Mech Infantry");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Mech Infantry"), "Mech Infantry not trainable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Mech Infantry"), "Mech Infantry trainable with Oil and Rubber");
    summarize_test_results();
}

void test_stealth_bomber_requires_oil_rubber () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Stealth Bomber");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Stealth Bomber"), "Stealth Bomber not trainable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Stealth Bomber"), "Stealth Bomber trainable with Oil and Rubber");
    summarize_test_results();
}

void test_radar_artillery_requires_saltpeter () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Radar Artillery");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Radar Artillery"), "Radar Artillery not trainable w/o Saltpeter");
    
    set_resource_available(setup.resource_vec, "Saltpeter");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Radar Artillery"), "Radar Artillery trainable with Saltpeter");
    summarize_test_results();
}

void test_musketeer_requires_saltpeter () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Musketeer");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Musketeer"), "Musketeer not trainable w/o Saltpeter");
    
    set_resource_available(setup.resource_vec, "Saltpeter");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Musketeer"), "Musketeer trainable with Saltpeter");
    summarize_test_results();
}

void test_all_saltpeter_units () {
    TestSetup setup = create_test_setup();
    const char* saltpeter_units[] = {"Musketman", "Rifleman", "Musketeer", "Cannon", "Artillery", "Radar Artillery"};
    set_resource_available_by_index(setup.resource_vec, 0);
    
    for (int i = 0; i < 6; i++) {
        set_unit_researched(setup.techs, saltpeter_units[i]);
    }
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 6; i++) {
        str msg = str("Saltpeter unit ") + str(saltpeter_units[i]) + " not trainable w/o Saltpeter";
        note_test_result (!is_unit_trainable(setup.units, saltpeter_units[i]), msg.c_str());
    }
    
    set_resource_available(setup.resource_vec, "Saltpeter");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 6; i++) {
        str msg = str("Saltpeter unit ") + str(saltpeter_units[i]) + " trainable with Saltpeter";
        note_test_result (is_unit_trainable(setup.units, saltpeter_units[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_all_uranium_units () {
    TestSetup setup = create_test_setup();
    const char* uranium_units[] = {"Nuclear Submarine", "Tactical Nuke", "Nuclear Missile"};
    set_resource_available_by_index(setup.resource_vec, 0);
    
    for (int i = 0; i < 3; i++) {
        set_unit_researched(setup.techs, uranium_units[i]);
    }
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 3; i++) {
        str msg = str("Uranium unit ") + str(uranium_units[i]) + " not trainable w/o Uranium";
        note_test_result (!is_unit_trainable(setup.units, uranium_units[i]), msg.c_str());
    }
    
    set_resource_available(setup.resource_vec, "Uranium");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 3; i++) {
        str msg = str("Uranium unit ") + str(uranium_units[i]) + " trainable with Uranium";
        note_test_result (is_unit_trainable(setup.units, uranium_units[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_all_iron_units () {
    TestSetup setup = create_test_setup();
    const char* iron_units[] = {"Swordsman", "Legion", "Immortal"};
    set_resource_available_by_index(setup.resource_vec, 0);
    
    for (int i = 0; i < 3; i++) {
        set_unit_researched(setup.techs, iron_units[i]);
    }
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 3; i++) {
        str msg = str("Iron unit ") + str(iron_units[i]) + " not trainable w/o Iron";
        note_test_result (!is_unit_trainable(setup.units, iron_units[i]), msg.c_str());
    }
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 3; i++) {
        str msg = str("Iron unit ") + str(iron_units[i]) + " trainable with Iron";
        note_test_result (is_unit_trainable(setup.units, iron_units[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_mixed_resource_requirements () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Swordsman");
    set_unit_researched(setup.techs, "Chariot");
    set_unit_researched(setup.techs, "Knight");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Swordsman"), "Swordsman trainable with Iron");
    note_test_result (!is_unit_trainable(setup.units, "Chariot"), "Chariot not trainable w/o Horses");
    note_test_result (!is_unit_trainable(setup.units, "Knight"), "Knight not trainable w/o Horses");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Swordsman"), "Swordsman still trainable");
    note_test_result (is_unit_trainable(setup.units, "Chariot"), "Chariot trainable with Horses");
    note_test_result (is_unit_trainable(setup.units, "Knight"), "Knight trainable with Horses and Iron");
    summarize_test_results();
}

void test_oil_rubber_units () {
    TestSetup setup = create_test_setup();
    const char* oil_rubber_units[] = {"Bomber", "Stealth Bomber", "Helicopter", "Mech Infantry"};
    set_resource_available_by_index(setup.resource_vec, 0);
    
    for (int i = 0; i < 4; i++) {
        set_unit_researched(setup.techs, oil_rubber_units[i]);
    }
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 4; i++) {
        str msg = str("Oil+Rubber unit ") + str(oil_rubber_units[i]) + " not trainable with only Oil";
        note_test_result (!is_unit_trainable(setup.units, oil_rubber_units[i]), msg.c_str());
    }
    
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 4; i++) {
        str msg = str("Oil+Rubber unit ") + str(oil_rubber_units[i]) + " trainable with Oil and Rubber";
        note_test_result (is_unit_trainable(setup.units, oil_rubber_units[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_oil_rubber_aluminum_units () {
    TestSetup setup = create_test_setup();
    const char* oil_rubber_aluminum_units[] = {"Fighter", "Stealth Fighter", "F-15"};
    set_resource_available_by_index(setup.resource_vec, 0);
    
    for (int i = 0; i < 3; i++) {
        set_unit_researched(setup.techs, oil_rubber_aluminum_units[i]);
    }
    
    set_resource_available(setup.resource_vec, "Oil");
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 3; i++) {
        str msg = str("Oil+Rubber+Aluminum unit ") + str(oil_rubber_aluminum_units[i]) + " not trainable w/o Aluminum";
        note_test_result (!is_unit_trainable(setup.units, oil_rubber_aluminum_units[i]), msg.c_str());
    }
    
    set_resource_available(setup.resource_vec, "Aluminum");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    for (int i = 0; i < 3; i++) {
        str msg = str("Oil+Rubber+Aluminum unit ") + str(oil_rubber_aluminum_units[i]) + " trainable with all resources";
        note_test_result (is_unit_trainable(setup.units, oil_rubber_aluminum_units[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_worker_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Worker");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Worker"), "Worker not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Worker"), "Worker trainable with City");
    summarize_test_results();
}

void test_scout_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Scout");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Scout"), "Scout not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Scout"), "Scout trainable with City");
    summarize_test_results();
}

void test_warrior_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Warrior");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Warrior"), "Warrior not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Warrior"), "Warrior trainable with City");
    summarize_test_results();
}

void test_spearman_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Spearman");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Spearman"), "Spearman not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Spearman"), "Spearman trainable with City");
    summarize_test_results();
}

void test_archer_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Archer");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Archer"), "Archer not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Archer"), "Archer trainable with City");
    summarize_test_results();
}

void test_pikeman_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Pikeman");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Pikeman"), "Pikeman not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Pikeman"), "Pikeman trainable with City");
    summarize_test_results();
}

void test_infantry_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Infantry");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Infantry"), "Infantry not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Infantry"), "Infantry trainable with City");
    summarize_test_results();
}

void test_catapult_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Catapult");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Catapult"), "Catapult not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Catapult"), "Catapult trainable with City");
    summarize_test_results();
}

void test_trebuchet_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Trebuchet");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Trebuchet"), "Trebuchet not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Trebuchet"), "Trebuchet trainable with City");
    summarize_test_results();
}

void test_galley_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Galley");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Galley"), "Galley not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Galley"), "Galley trainable with City");
    summarize_test_results();
}

void test_caravel_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Caravel");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Caravel"), "Caravel not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Caravel"), "Caravel trainable with City");
    summarize_test_results();
}

void test_frigate_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Frigate");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Frigate"), "Frigate not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Frigate"), "Frigate trainable with City");
    summarize_test_results();
}

void test_ironclad_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Ironclad");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Ironclad"), "Ironclad not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Ironclad"), "Ironclad trainable with City");
    summarize_test_results();
}

void test_submarine_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Submarine");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Submarine"), "Submarine not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Submarine"), "Submarine trainable with City");
    summarize_test_results();
}

void test_transport_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Transport");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Transport"), "Transport not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Transport"), "Transport trainable with City");
    summarize_test_results();
}

void test_carrier_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Carrier");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Carrier"), "Carrier not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Carrier"), "Carrier trainable with City");
    summarize_test_results();
}

void test_marine_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Marine");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Marine"), "Marine not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Marine"), "Marine trainable with City");
    summarize_test_results();
}

void test_paratrooper_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Paratrooper");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Paratrooper"), "Paratrooper not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Paratrooper"), "Paratrooper trainable with City");
    summarize_test_results();
}

void test_mobile_sam_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Mobile SAM");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Mobile SAM"), "Mobile SAM not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Mobile SAM"), "Mobile SAM trainable with City");
    summarize_test_results();
}

void test_guided_missile_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Guided Missile");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Guided Missile"), "Guided Missile not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Guided Missile"), "Guided Missile trainable with City");
    summarize_test_results();
}

void test_longbowman_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Longbowman");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Longbowman"), "Longbowman not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Longbowman"), "Longbowman trainable with City");
    summarize_test_results();
}

void test_explorer_no_resources () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Explorer");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Explorer"), "Explorer not trainable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Explorer"), "Explorer trainable with City");
    summarize_test_results();
}

void test_panzer_requires_oil () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Panzer");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Panzer"), "Panzer not trainable w/o Oil");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Panzer"), "Panzer trainable with Oil");
    summarize_test_results();
}

void test_destroyer_requires_oil () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Destroyer");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Destroyer"), "Destroyer not trainable w/o Oil");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Destroyer"), "Destroyer trainable with Oil");
    summarize_test_results();
}

void test_battleship_requires_oil () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Battleship");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Battleship"), "Battleship not trainable w/o Oil");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Battleship"), "Battleship trainable with Oil");
    summarize_test_results();
}

void test_cruiser_requires_oil () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Cruiser");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Cruiser"), "Cruiser not trainable w/o Oil");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Cruiser"), "Cruiser trainable with Oil");
    summarize_test_results();
}

void test_aegis_cruiser_requires_oil () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "AEGIS Cruiser");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "AEGIS Cruiser"), "AEGIS Cruiser not trainable w/o Oil");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "AEGIS Cruiser"), "AEGIS Cruiser trainable with Oil");
    summarize_test_results();
}

void test_ansar_warrior_requires_horses () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Ansar Warrior");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Ansar Warrior"), "Ansar Warrior not trainable w/o Horses");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Ansar Warrior"), "Ansar Warrior trainable with Horses");
    summarize_test_results();
}

void test_conquistador_requires_horses () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Conquistador");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Conquistador"), "Conquistador not trainable w/o Horses");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Conquistador"), "Conquistador trainable with Horses");
    summarize_test_results();
}

void test_rider_requires_horses () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Rider");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Rider"), "Rider not trainable w/o Horses");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Rider"), "Rider trainable with Horses");
    summarize_test_results();
}

void test_cossack_requires_horses () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Cossack");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "Cossack"), "Cossack not trainable w/o Horses");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Cossack"), "Cossack trainable with Horses");
    summarize_test_results();
}

void test_war_elephant_requires_horses () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "War Elephant");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (!is_unit_trainable(setup.units, "War Elephant"), "War Elephant not trainable w/o Horses");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "War Elephant"), "War Elephant trainable with Horses");
    summarize_test_results();
}

void test_comprehensive_resource_combinations () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Swordsman");
    set_unit_researched(setup.techs, "Knight");
    set_unit_researched(setup.techs, "Fighter");
    set_unit_researched(setup.techs, "Modern Armor");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Swordsman"), "Swordsman trainable with Iron");
    note_test_result (!is_unit_trainable(setup.units, "Knight"), "Knight needs Horses");
    note_test_result (!is_unit_trainable(setup.units, "Fighter"), "Fighter needs multiple resources");
    note_test_result (!is_unit_trainable(setup.units, "Modern Armor"), "Modern Armor needs Oil and Aluminum");
    
    set_resource_available(setup.resource_vec, "Horses");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Knight"), "Knight trainable with Horses and Iron");
    
    set_resource_available(setup.resource_vec, "Oil");
    set_resource_available(setup.resource_vec, "Rubber");
    set_resource_available(setup.resource_vec, "Aluminum");
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    note_test_result (is_unit_trainable(setup.units, "Fighter"), "Fighter trainable with Oil, Rubber, Aluminum");
    note_test_result (is_unit_trainable(setup.units, "Modern Armor"), "Modern Armor trainable with Oil and Aluminum");
    summarize_test_results();
}

void test_determine_trainable_units_idempotent () {
    TestSetup setup = create_test_setup();
    set_unit_researched(setup.techs, "Swordsman");
    set_resource_available_by_index(setup.resource_vec, 0);
    set_resource_available(setup.resource_vec, "Iron");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    bool first = is_unit_trainable(setup.units, "Swordsman");
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    bool second = is_unit_trainable(setup.units, "Swordsman");
    
    note_test_result (first == second && first == true, "determine_trainable_units idempotent");
    summarize_test_results();
}

void test_all_resources_all_units () {
    TestSetup setup = create_test_setup();
    u32 unit_count = UnitVector::get_unit_data_count();
    set_resource_available_by_index(setup.resource_vec, 0);
    
    const char* all_resources[] = {"Iron", "Horses", "Saltpeter", "Oil", "Uranium", "Rubber", "Aluminum"};
    for (int i = 0; i < 7; i++) {
        set_resource_available(setup.resource_vec, all_resources[i]);
    }
    
    for (u32 i = 0; i < unit_count && i < 20; i++) {
        setup.techs->set_bit(i);
    }
    
    setup.assessor->determine_trainable_units(setup.units, *setup.resource_vec);
    u32 trainable_count = 0;
    for (u32 i = 0; i < unit_count && i < 20; i++) {
        if (setup.units->is_trainable(i)) {
            trainable_count++;
        }
    }
    note_test_result (trainable_count > 0, "Some units trainable with all resources");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    test_trainable_assessor_construction();
    test_trainable_assessor_load_and_print();
    test_swordsman_requires_iron();
    test_legion_requires_iron();
    test_immortal_requires_iron();
    test_chariot_requires_horses();
    test_horseman_requires_horses();
    test_knight_requires_horses_and_iron();
    test_cavalry_requires_horses();
    test_musketman_requires_saltpeter();
    test_rifleman_requires_saltpeter();
    test_cannon_requires_saltpeter();
    test_artillery_requires_saltpeter();
    test_tank_requires_oil();
    test_modern_armor_requires_oil_and_aluminum();
    test_fighter_requires_oil_rubber_aluminum();
    test_bomber_requires_oil_rubber();
    test_stealth_fighter_requires_oil_rubber_aluminum();
    test_nuclear_submarine_requires_uranium();
    test_tactical_nuke_requires_uranium();
    test_nuclear_missile_requires_uranium();
    test_unit_without_resources_needs_City();
    test_multiple_units_same_resource();
    test_unit_not_researched_not_trainable();
    test_all_horse_units();
    test_all_oil_units();
    test_f15_requires_oil_rubber_aluminum();
    test_helicopter_requires_oil_rubber();
    test_mech_infantry_requires_oil_rubber();
    test_stealth_bomber_requires_oil_rubber();
    test_radar_artillery_requires_saltpeter();
    test_musketeer_requires_saltpeter();
    test_all_saltpeter_units();
    test_all_uranium_units();
    test_all_iron_units();
    test_mixed_resource_requirements();
    test_oil_rubber_units();
    test_oil_rubber_aluminum_units();
    test_worker_no_resources();
    test_scout_no_resources();
    test_warrior_no_resources();
    test_spearman_no_resources();
    test_archer_no_resources();
    test_pikeman_no_resources();
    test_infantry_no_resources();
    test_catapult_no_resources();
    test_trebuchet_no_resources();
    test_galley_no_resources();
    test_caravel_no_resources();
    test_frigate_no_resources();
    test_ironclad_no_resources();
    test_submarine_no_resources();
    test_transport_no_resources();
    test_carrier_no_resources();
    test_marine_no_resources();
    test_paratrooper_no_resources();
    test_mobile_sam_no_resources();
    test_guided_missile_no_resources();
    test_longbowman_no_resources();
    test_explorer_no_resources();
    test_panzer_requires_oil();
    test_destroyer_requires_oil();
    test_battleship_requires_oil();
    test_cruiser_requires_oil();
    test_aegis_cruiser_requires_oil();
    test_ansar_warrior_requires_horses();
    test_conquistador_requires_horses();
    test_rider_requires_horses();
    test_cossack_requires_horses();
    test_war_elephant_requires_horses();
    test_comprehensive_resource_combinations();
    test_determine_trainable_units_idempotent();
    test_all_resources_all_units();

    printf("\n=======================================================\n");
    printf(" TESTING TRAINABLE ASSESSOR: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("======================================================\n\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
