//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "building_vector.h"
#include "resources_vector.h"
#include "buildable_assessor.h"
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

void setup_static_data () {
    BuildingIO building_io("../game_config_test.buildings");
    building_io.parse_and_allocate();
    
    ResourceIO resource_io("../game_config_test.resources");
    resource_io.parse_and_allocate();
}

static u32 find_building_index (const char* building_name) {
    const BuildingTypeStats* building_array = BuildingVector::get_building_data_array();
    u32 building_count = BuildingVector::get_building_data_count();
    if (building_array == nullptr) {
        return UINT32_MAX;
    }
    for (u32 i = 0; i < building_count; i++) {
        if (building_array[i].name == building_name) {
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
    BitArrayCL* researched_buildings;
    BitArrayCL* resources;
    BuildingVector* building_vec;
    ResourceVector* resource_vec;
    BuildableAssessor* assessor;
    
    
    TestSetup () : 
        researched_buildings(nullptr), 
        resources(nullptr), 
        building_vec(nullptr), 
        resource_vec(nullptr), 
        assessor(nullptr) {
    }
    
    ~TestSetup () {
        delete assessor;
        delete building_vec;
        delete resource_vec;
        delete resources;
        delete researched_buildings;
    }
};

TestSetup create_test_setup () {
    TestSetup setup;
    setup_static_data();
    
    u32 building_count = BuildingVector::get_building_data_count();
    u32 resource_count = ResourceVector::get_resource_data_count();
    
    setup.researched_buildings = new BitArrayCL(building_count);
    setup.resources = new BitArrayCL(resource_count);
    setup.building_vec = new BuildingVector(setup.researched_buildings);
    setup.resource_vec = new ResourceVector(resource_count, setup.resources);
    setup.assessor = new BuildableAssessor();
    setup.assessor->load_building_resource_costs("../game_config_test.building_res_cost");
    
    return setup;
}

void set_resource_available (ResourceVector* rv, const char* resource_name) {
    rv->set_available(find_resource_index(resource_name));
}

void set_resource_available_by_index (ResourceVector* rv, u32 idx) {
    rv->set_available(idx);
}

void set_building_researched (BitArrayCL* researched, const char* building_name) {
    researched->set_bit(find_building_index(building_name));
}

bool is_buildable (BuildingVector* bv, const char* building_name) {
    return bv->is_buildable(find_building_index(building_name));
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_buildable_assessor_construction () {
    setup_static_data();
    BuildableAssessor assessor;
    note_result (true, "BuildableAssessor construction");
    summarize_test_results();
}

void test_buildable_assessor_load_and_print () {
    setup_static_data();
    BuildableAssessor assessor;
    assessor.load_building_resource_costs("../game_config_test.building_res_cost");
    if (print_level > 0) {
        printf("Building Resource Costs:\n");
        assessor.print_building_resource_costs();
    }
    note_result (true, "BuildableAssessor load and print");
    summarize_test_results();
}

void test_factory_requires_iron_and_coal () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Factory");
    set_resource_available_by_index(setup.resource_vec, 0);

    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Factory"), "Factory not buildable w/o resources");
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Factory"), "Factory not buildable with only Iron");
    
    set_resource_available(setup.resource_vec, "Coal");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Factory"), "Factory buildable with Iron and Coal");
    summarize_test_results();
}

void test_coastal_fortress_requires_saltpeter_and_iron () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Coastal Fortress");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Coastal Fortress"), "Coastal Fortress not buildable w/o resources");
    
    set_resource_available(setup.resource_vec, "Saltpeter");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Coastal Fortress"), "Coastal Fortress not buildable with only Saltpeter");
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Coastal Fortress"), "Coastal Fortress buildable with Saltpeter and Iron");
    summarize_test_results();
}

void test_airport_requires_oil_and_rubber () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Airport");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Airport"), "Airport not buildable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Airport"), "Airport not buildable with only Oil");
    
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Airport"), "Airport buildable with Oil and Rubber");
    summarize_test_results();
}

void test_mass_transit_requires_oil_and_rubber () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Mass Transit");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Mass Transit"), "Mass Transit not buildable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Mass Transit"), "Mass Transit not buildable with only Oil");
    
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Mass Transit"), "Mass Transit buildable with Oil and Rubber");
    summarize_test_results();
}

void test_coal_plant_requires_coal () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Coal Plant");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Coal Plant"), "Coal Plant not buildable w/o Coal");
    
    set_resource_available(setup.resource_vec, "Coal");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Coal Plant"), "Coal Plant buildable with Coal");
    summarize_test_results();
}

void test_nuclear_plant_requires_uranium () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Nuclear Plant");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Nuclear Plant"), "Nuclear Plant not buildable w/o Uranium");
    
    set_resource_available(setup.resource_vec, "Uranium");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Nuclear Plant"), "Nuclear Plant buildable with Uranium");
    summarize_test_results();
}

void test_manufacturing_plant_requires_iron () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Manufacturing Plant");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Manufacturing Plant"), "Manufacturing Plant not buildable w/o Iron");
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Manufacturing Plant"), "Manufacturing Plant buildable with Iron");
    summarize_test_results();
}

void test_superhighways_requires_oil_and_rubber () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Superhighways");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Superhighways"), "Superhighways not buildable w/o resources");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Superhighways"), "Superhighways not buildable with only Oil");
    
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Superhighways"), "Superhighways buildable with Oil and Rubber");
    summarize_test_results();
}

void test_sam_missile_battery_requires_aluminum () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "SAM Missile Battery");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "SAM Missile Battery"), "SAM Missile Battery not buildable w/o Aluminum");
    
    set_resource_available(setup.resource_vec, "Aluminum");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "SAM Missile Battery"), "SAM Missile Battery buildable with Aluminum");
    summarize_test_results();
}

void test_offshore_platform_requires_oil () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Offshore Platform");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Offshore Platform"), "Offshore Platform not buildable w/o Oil");
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Offshore Platform"), "Offshore Platform buildable with Oil");
    summarize_test_results();
}

void test_wall_requires_stone () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Wall");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Wall"), "Wall not buildable w/o Stone");
    
    set_resource_available(setup.resource_vec, "Stone");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Wall"), "Wall buildable with Stone");
    summarize_test_results();
}

void test_solar_plant_requires_aluminum () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Solar Plant");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Solar Plant"), "Solar Plant not buildable w/o Aluminum");
    
    set_resource_available(setup.resource_vec, "Aluminum");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Solar Plant"), "Solar Plant buildable with Aluminum");
    summarize_test_results();
}

void test_barracks_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Barracks");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Barracks"), "Barracks not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Barracks"), "Barracks buildable with City");
    summarize_test_results();
}

void test_granary_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Granary");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Granary"), "Granary not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Granary"), "Granary buildable with City");
    summarize_test_results();
}

void test_library_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Library");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Library"), "Library not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Library"), "Library buildable with City");
    summarize_test_results();
}

void test_temple_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Temple");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Temple"), "Temple not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Temple"), "Temple buildable with City");
    summarize_test_results();
}

void test_marketplace_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Marketplace");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Marketplace"), "Marketplace not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Marketplace"), "Marketplace buildable with City");
    summarize_test_results();
}

void test_aqueduct_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Aqueduct");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Aqueduct"), "Aqueduct not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Aqueduct"), "Aqueduct buildable with City");
    summarize_test_results();
}

void test_courthouse_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Courthouse");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Courthouse"), "Courthouse not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Courthouse"), "Courthouse buildable with City");
    summarize_test_results();
}

void test_harbor_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Harbor");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Harbor"), "Harbor not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Harbor"), "Harbor buildable with City");
    summarize_test_results();
}

void test_bank_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Bank");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Bank"), "Bank not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Bank"), "Bank buildable with City");
    summarize_test_results();
}

void test_cathedral_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Cathedral");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Cathedral"), "Cathedral not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Cathedral"), "Cathedral buildable with City");
    summarize_test_results();
}

void test_university_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "University");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "University"), "University not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "University"), "University buildable with City");
    summarize_test_results();
}

void test_colosseum_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Colosseum");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Colosseum"), "Colosseum not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Colosseum"), "Colosseum buildable with City");
    summarize_test_results();
}

void test_police_station_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Police Station");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Police Station"), "Police Station not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Police Station"), "Police Station buildable with City");
    summarize_test_results();
}

void test_commercial_dock_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Commercial Dock");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Commercial Dock"), "Commercial Dock not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Commercial Dock"), "Commercial Dock buildable with City");
    summarize_test_results();
}

void test_hospital_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Hospital");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Hospital"), "Hospital not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Hospital"), "Hospital buildable with City");
    summarize_test_results();
}

void test_hydro_plant_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Hydro Plant");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Hydro Plant"), "Hydro Plant not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Hydro Plant"), "Hydro Plant buildable with City");
    summarize_test_results();
}

void test_research_lab_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Research Lab");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Research Lab"), "Research Lab not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Research Lab"), "Research Lab buildable with City");
    summarize_test_results();
}

void test_stock_exchange_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Stock Exchange");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Stock Exchange"), "Stock Exchange not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Stock Exchange"), "Stock Exchange buildable with City");
    summarize_test_results();
}

void test_recycling_center_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Recycling Center");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Recycling Center"), "Recycling Center not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Recycling Center"), "Recycling Center buildable with City");
    summarize_test_results();
}

void test_civil_defense_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Civil Defense");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Civil Defense"), "Civil Defense not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Civil Defense"), "Civil Defense buildable with City");
    summarize_test_results();
}

void test_intelligence_agency_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Intelligence Agency");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Intelligence Agency"), "Intelligence Agency not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Intelligence Agency"), "Intelligence Agency buildable with City");
    summarize_test_results();
}

void test_capitalization_no_resources () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Capitalization");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Capitalization"), "Capitalization not buildable w/o City");
    
    set_resource_available_by_index(setup.resource_vec, 0);
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Capitalization"), "Capitalization buildable with City");
    summarize_test_results();
}

void test_all_oil_rubber_buildings () {
    TestSetup setup = create_test_setup();
    const char* oil_rubber_buildings[] = {"Airport", "Mass Transit", "Superhighways"};
    set_resource_available_by_index(setup.resource_vec, 0);
    
    for (int i = 0; i < 3; i++) {
        set_building_researched(setup.researched_buildings, oil_rubber_buildings[i]);
    }
    
    set_resource_available(setup.resource_vec, "Oil");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    for (int i = 0; i < 3; i++) {
        str msg = str("Oil+Rubber building ") + str(oil_rubber_buildings[i]) + " not buildable with only Oil";
        note_result (!is_buildable(setup.building_vec, oil_rubber_buildings[i]), msg.c_str());
    }
    
    set_resource_available(setup.resource_vec, "Rubber");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    for (int i = 0; i < 3; i++) {
        str msg = str("Oil+Rubber building ") + str(oil_rubber_buildings[i]) + " buildable with Oil and Rubber";
        note_result (is_buildable(setup.building_vec, oil_rubber_buildings[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_all_aluminum_buildings () {
    TestSetup setup = create_test_setup();
    const char* aluminum_buildings[] = {"SAM Missile Battery", "Solar Plant"};
    set_resource_available_by_index(setup.resource_vec, 0);
    
    for (int i = 0; i < 2; i++) {
        set_building_researched(setup.researched_buildings, aluminum_buildings[i]);
    }
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    for (int i = 0; i < 2; i++) {
        str msg = str("Aluminum building ") + str(aluminum_buildings[i]) + " not buildable w/o Aluminum";
        note_result (!is_buildable(setup.building_vec, aluminum_buildings[i]), msg.c_str());
    }
    
    set_resource_available(setup.resource_vec, "Aluminum");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    for (int i = 0; i < 2; i++) {
        str msg = str("Aluminum building ") + str(aluminum_buildings[i]) + " buildable with Aluminum";
        note_result (is_buildable(setup.building_vec, aluminum_buildings[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_mixed_resource_requirements () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Factory");
    set_building_researched(setup.researched_buildings, "Coal Plant");
    set_building_researched(setup.researched_buildings, "Nuclear Plant");
    set_building_researched(setup.researched_buildings, "Wall");
    set_resource_available_by_index(setup.resource_vec, 0);
    
    set_resource_available(setup.resource_vec, "Iron");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Factory"), "Factory needs Coal");
    note_result (!is_buildable(setup.building_vec, "Coal Plant"), "Coal Plant needs Coal");
    note_result (!is_buildable(setup.building_vec, "Nuclear Plant"), "Nuclear Plant needs Uranium");
    note_result (!is_buildable(setup.building_vec, "Wall"), "Wall needs Stone");
    
    set_resource_available(setup.resource_vec, "Coal");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Factory"), "Factory buildable with Iron and Coal");
    note_result (is_buildable(setup.building_vec, "Coal Plant"), "Coal Plant buildable with Coal");
    
    set_resource_available(setup.resource_vec, "Uranium");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Nuclear Plant"), "Nuclear Plant buildable with Uranium");
    
    set_resource_available(setup.resource_vec, "Stone");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Wall"), "Wall buildable with Stone");
    summarize_test_results();
}

void test_building_not_researched_not_buildable () {
    TestSetup setup = create_test_setup();
    set_resource_available_by_index(setup.resource_vec, 0);
    set_resource_available(setup.resource_vec, "Iron");
    set_resource_available(setup.resource_vec, "Coal");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (!is_buildable(setup.building_vec, "Factory"), "Factory not buildable if not researched");
    
    set_building_researched(setup.researched_buildings, "Factory");
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    note_result (is_buildable(setup.building_vec, "Factory"), "Factory buildable when researched");
    summarize_test_results();
}

void test_determine_buildable_buildings_idempotent () {
    TestSetup setup = create_test_setup();
    set_building_researched(setup.researched_buildings, "Factory");
    set_resource_available_by_index(setup.resource_vec, 0);
    set_resource_available(setup.resource_vec, "Iron");
    set_resource_available(setup.resource_vec, "Coal");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    bool first = is_buildable(setup.building_vec, "Factory");
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    bool second = is_buildable(setup.building_vec, "Factory");
    
    note_result (first == second && first == true, "determine_buildable_buildings idempotent");
    summarize_test_results();
}

void test_all_resources_all_buildings () {
    TestSetup setup = create_test_setup();
    u32 building_count = BuildingVector::get_building_data_count();
    set_resource_available_by_index(setup.resource_vec, 0);
    
    const char* all_resources[] = {"Iron", "Coal", "Saltpeter", "Oil", "Uranium", "Rubber", "Aluminum", "Stone"};
    for (int i = 0; i < 8; i++) {
        set_resource_available(setup.resource_vec, all_resources[i]);
    }
    
    for (u32 i = 0; i < building_count && i < 35; i++) {
        setup.researched_buildings->set_bit(i);
    }
    
    setup.assessor->determine_buildable_buildings(setup.building_vec, *setup.resource_vec);
    u32 buildable_count = 0;
    for (u32 i = 0; i < building_count && i < 35; i++) {
        if (setup.building_vec->is_buildable(i)) {
            buildable_count++;
        }
    }
    note_result (buildable_count > 0, "Some buildings buildable with all resources");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    test_buildable_assessor_construction();
    test_buildable_assessor_load_and_print();
    
    // Tests for buildings with resource requirements
    test_factory_requires_iron_and_coal();
    test_coastal_fortress_requires_saltpeter_and_iron();
    test_airport_requires_oil_and_rubber();
    test_mass_transit_requires_oil_and_rubber();
    test_coal_plant_requires_coal();
    test_nuclear_plant_requires_uranium();
    test_manufacturing_plant_requires_iron();
    test_superhighways_requires_oil_and_rubber();
    test_sam_missile_battery_requires_aluminum();
    test_offshore_platform_requires_oil();
    test_wall_requires_stone();
    test_solar_plant_requires_aluminum();
    
    // Tests for buildings without resource requirements
    test_barracks_no_resources();
    test_granary_no_resources();
    test_library_no_resources();
    test_temple_no_resources();
    test_marketplace_no_resources();
    test_aqueduct_no_resources();
    test_courthouse_no_resources();
    test_harbor_no_resources();
    test_bank_no_resources();
    test_cathedral_no_resources();
    test_university_no_resources();
    test_colosseum_no_resources();
    test_police_station_no_resources();
    test_commercial_dock_no_resources();
    test_hospital_no_resources();
    test_hydro_plant_no_resources();
    test_research_lab_no_resources();
    test_stock_exchange_no_resources();
    test_recycling_center_no_resources();
    test_civil_defense_no_resources();
    test_intelligence_agency_no_resources();
    test_capitalization_no_resources();
    
    // Comprehensive tests
    test_all_oil_rubber_buildings();
    test_all_aluminum_buildings();
    test_mixed_resource_requirements();
    test_building_not_researched_not_buildable();
    test_determine_buildable_buildings_idempotent();
    test_all_resources_all_buildings();

    printf("=======================================================\n");
    printf(" TESTING BUILDABLE ASSESSOR: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
