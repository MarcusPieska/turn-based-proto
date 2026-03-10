//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "building_vector.h"
#include "building_data.h"
#include "resource_data.h"
#include "tech_data.h"
#include "city_flags.h"
#include "building_assessor.h"
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
    CityFlagData::load_static_data("../game_config.city_flags");
    TechData::load_static_data("../game_config.techs");
    ResourceData::load_static_data("../game_config.resources");
    BuildingData::load_static_data("../game_config.buildings");
}

static u32 find_building_index (const char* building_name) {
    const BuildingTypeStats* building_array = BuildingData::get_building_data_array();
    u32 building_count = BuildingData::get_building_data_count();
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
    const ResourceTypeStats* resource_array = ResourceData::get_resource_data_array();
    u32 resource_count = ResourceData::get_resource_data_count();
    for (u32 i = 0; i < resource_count; i++) {
        if (resource_array[i].name == resource_name) {
            return i;
        }
    }
    return UINT32_MAX;
}

static u32 find_tech_index (const char* tech_name) {
    const TechTypeStats* tech_array = TechData::get_tech_data_array();
    u32 tech_count = TechData::get_tech_data_count();
    for (u32 i = 0; i < tech_count; i++) {
        if (tech_array[i].name == tech_name) {
            return i;
        }
    }
    return UINT32_MAX;
}

struct TestSetup {
    BitArrayCL* techs;
    BitArrayCL* resources;
    BitArrayCL* flags;
    BuiltBuildings* built;
    BuildableBuildings* buildable;
    
    TestSetup () : 
        techs(nullptr), 
        resources(nullptr), 
        flags(nullptr),
        built(nullptr), 
        buildable(nullptr) {
    }
    
    ~TestSetup () {
        delete buildable;
        delete built;
        delete flags;
        delete resources;
        delete techs;
    }
};

TestSetup create_test_setup () {
    TestSetup setup;
    setup_static_data();
    
    u32 building_count = BuildingData::get_building_data_count();
    u32 resource_count = ResourceData::get_resource_data_count();
    u32 tech_count = TechData::get_tech_data_count();
    u32 flag_count = CityFlagData::get_flag_count();
    
    setup.techs = new BitArrayCL(tech_count);
    setup.resources = new BitArrayCL(resource_count);
    setup.flags = new BitArrayCL(flag_count);
    setup.built = new BuiltBuildings();
    setup.buildable = nullptr; // Will be created by assess()
    
    return setup;
}

void set_resource_available (BitArrayCL* resources, const char* resource_name) {
    u32 idx = find_resource_index(resource_name);
    if (idx != UINT32_MAX) {
        resources->set_bit(idx);
    }
}

void set_resource_available_by_index (BitArrayCL* resources, u32 idx) {
    resources->set_bit(idx);
}

void set_tech_researched (BitArrayCL* techs, const char* tech_name) {
    u32 idx = find_tech_index(tech_name);
    if (idx != UINT32_MAX) {
        techs->set_bit(idx);
    }
}

void set_building_tech_researched (BitArrayCL* techs, const char* building_name) {
    u32 building_idx = find_building_index(building_name);
    if (building_idx != UINT32_MAX) {
        const BuildingTypeStats* buildings = BuildingData::get_building_data_array();
        u32 tech_idx = buildings[building_idx].tech_prereq_idx.get_idx();
        techs->set_bit(tech_idx);
    }
}

void set_flag (BitArrayCL* flags, const char* flag_name) {
    u16 flag_idx = CityFlagData::find_flag_index(flag_name);
    flags->set_bit(flag_idx);
}

void set_building_built (BuiltBuildings* built, const char* building_name) {
    u32 building_idx = find_building_index(building_name);
    if (building_idx != UINT32_MAX) {
        built->set_built(building_idx);
    }
}

bool is_buildable (BuildableBuildings* buildable, const char* building_name) {
    if (buildable == nullptr) {
        return false;
    }
    u32 idx = find_building_index(building_name);
    if (idx == UINT32_MAX) {
        return false;
    }
    return buildable->can_build(idx);
}

void reassess_buildable (TestSetup& setup) {
    if (setup.buildable != nullptr) {
        delete setup.buildable;
    }
    setup.buildable = BuildingAssessor::assess(setup.techs, setup.built, setup.resources, setup.flags);
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_buildable_assessor_construction () {
    setup_static_data();
    note_result (true, "BuildingAssessor static class");
    summarize_test_results();
}

void test_factory_requires_iron_and_coal () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Factory");

    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Factory"), "Factory not buildable w/o resources");
    
    set_resource_available(setup.resources, "Iron");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Factory"), "Factory not buildable with only Iron");
    
    set_resource_available(setup.resources, "Coal");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Factory"), "Factory buildable with Iron and Coal");
    summarize_test_results();
}

void test_coastal_fortress_requires_saltpeter_iron_and_coastal_flag () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Coastal Fortress");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Coastal Fortress"), "Coastal Fortress not buildable w/o requirements");
    
    set_resource_available(setup.resources, "Saltpeter");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Coastal Fortress"), "Coastal Fortress not buildable with only Saltpeter");
    
    set_resource_available(setup.resources, "Iron");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Coastal Fortress"), "Coastal Fortress not buildable without coastal flag");
    
    set_flag(setup.flags, "isCoastal");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Coastal Fortress"), "Coastal Fortress buildable with Saltpeter, Iron, and coastal flag");
    summarize_test_results();
}

void test_airport_requires_oil_and_rubber () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Airport");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Airport"), "Airport not buildable w/o resources");
    
    set_resource_available(setup.resources, "Oil");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Airport"), "Airport not buildable with only Oil");
    
    set_resource_available(setup.resources, "Rubber");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Airport"), "Airport buildable with Oil and Rubber");
    summarize_test_results();
}

void test_mass_transit_requires_oil_and_rubber () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Mass Transit");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Mass Transit"), "Mass Transit not buildable w/o resources");
    
    set_resource_available(setup.resources, "Oil");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Mass Transit"), "Mass Transit not buildable with only Oil");
    
    set_resource_available(setup.resources, "Rubber");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Mass Transit"), "Mass Transit buildable with Oil and Rubber");
    summarize_test_results();
}

void test_coal_plant_requires_coal () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Coal Plant");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Coal Plant"), "Coal Plant not buildable w/o Coal");
    
    set_resource_available(setup.resources, "Coal");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Coal Plant"), "Coal Plant buildable with Coal");
    summarize_test_results();
}

void test_nuclear_plant_requires_uranium () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Nuclear Plant");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Nuclear Plant"), "Nuclear Plant not buildable w/o Uranium");
    
    set_resource_available(setup.resources, "Uranium");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Nuclear Plant"), "Nuclear Plant buildable with Uranium");
    summarize_test_results();
}

void test_manufacturing_plant_requires_iron_and_factory () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Manufacturing Plant");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Manufacturing Plant"), "Manufacturing Plant not buildable w/o requirements");
    
    set_resource_available(setup.resources, "Iron");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Manufacturing Plant"), "Manufacturing Plant not buildable without Factory");
    
    set_building_tech_researched(setup.techs, "Factory");
    set_resource_available(setup.resources, "Coal");
    set_building_built(setup.built, "Factory");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Manufacturing Plant"), "Manufacturing Plant buildable with Iron and Factory");
    summarize_test_results();
}

void test_superhighways_requires_oil_and_rubber () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Superhighways");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Superhighways"), "Superhighways not buildable w/o resources");
    
    set_resource_available(setup.resources, "Oil");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Superhighways"), "Superhighways not buildable with only Oil");
    
    set_resource_available(setup.resources, "Rubber");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Superhighways"), "Superhighways buildable with Oil and Rubber");
    summarize_test_results();
}

void test_sam_missile_battery_requires_aluminum () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "SAM Missile Battery");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "SAM Missile Battery"), "SAM Missile Battery not buildable w/o Aluminum");
    
    set_resource_available(setup.resources, "Aluminum");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "SAM Missile Battery"), "SAM Missile Battery buildable with Aluminum");
    summarize_test_results();
}

void test_offshore_platform_requires_oil_and_coastal_flag () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Offshore Platform");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Offshore Platform"), "Offshore Platform not buildable w/o requirements");
    
    set_resource_available(setup.resources, "Oil");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Offshore Platform"), "Offshore Platform not buildable without coastal flag");
    
    set_flag(setup.flags, "isCoastal");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Offshore Platform"), "Offshore Platform buildable with Oil and coastal flag");
    summarize_test_results();
}

void test_wall_requires_stone () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Wall");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Wall"), "Wall not buildable w/o Stone");
    
    set_resource_available(setup.resources, "Stone");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Wall"), "Wall buildable with Stone");
    summarize_test_results();
}

void test_solar_plant_requires_aluminum () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Solar Plant");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Solar Plant"), "Solar Plant not buildable w/o Aluminum");
    
    set_resource_available(setup.resources, "Aluminum");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Solar Plant"), "Solar Plant buildable with Aluminum");
    summarize_test_results();
}

void test_barracks_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Barracks"), "Barracks not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Barracks");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Barracks"), "Barracks buildable with tech");
    summarize_test_results();
}

void test_granary_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Granary"), "Granary not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Granary");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Granary"), "Granary buildable with tech");
    summarize_test_results();
}

void test_library_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Library"), "Library not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Library");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Library"), "Library buildable with tech");
    summarize_test_results();
}

void test_temple_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Temple"), "Temple not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Temple");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Temple"), "Temple buildable with tech");
    summarize_test_results();
}

void test_marketplace_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Marketplace"), "Marketplace not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Marketplace");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Marketplace"), "Marketplace buildable with tech");
    summarize_test_results();
}

void test_aqueduct_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Aqueduct"), "Aqueduct not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Aqueduct");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Aqueduct"), "Aqueduct buildable with tech");
    summarize_test_results();
}

void test_courthouse_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Courthouse"), "Courthouse not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Courthouse");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Courthouse"), "Courthouse buildable with tech");
    summarize_test_results();
}

void test_harbor_requires_tech_and_coastal_flag () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Harbor"), "Harbor not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Harbor");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Harbor"), "Harbor not buildable without coastal flag");
    
    set_flag(setup.flags, "isCoastal");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Harbor"), "Harbor buildable with tech and coastal flag");
    summarize_test_results();
}

void test_bank_requires_marketplace () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Bank"), "Bank not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Bank");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Bank"), "Bank not buildable without Marketplace");
    
    set_building_tech_researched(setup.techs, "Marketplace");
    set_building_built(setup.built, "Marketplace");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Bank"), "Bank buildable with tech and Marketplace");
    summarize_test_results();
}

void test_cathedral_requires_temple () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Cathedral"), "Cathedral not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Cathedral");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Cathedral"), "Cathedral not buildable without Temple");
    
    set_building_tech_researched(setup.techs, "Temple");
    set_building_built(setup.built, "Temple");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Cathedral"), "Cathedral buildable with tech and Temple");
    summarize_test_results();
}

void test_university_requires_library () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "University"), "University not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "University");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "University"), "University not buildable without Library");
    
    set_building_tech_researched(setup.techs, "Library");
    set_building_built(setup.built, "Library");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "University"), "University buildable with tech and Library");
    summarize_test_results();
}

void test_colosseum_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Colosseum"), "Colosseum not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Colosseum");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Colosseum"), "Colosseum buildable with tech");
    summarize_test_results();
}

void test_police_station_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Police Station"), "Police Station not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Police Station");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Police Station"), "Police Station buildable with tech");
    summarize_test_results();
}

void test_commercial_dock_requires_harbor () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Commercial Dock"), "Commercial Dock not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Commercial Dock");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Commercial Dock"), "Commercial Dock not buildable without Harbor");
    
    set_building_tech_researched(setup.techs, "Harbor");
    set_flag(setup.flags, "isCoastal");
    set_building_built(setup.built, "Harbor");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Commercial Dock"), "Commercial Dock buildable with tech and Harbor");
    summarize_test_results();
}

void test_hospital_requires_aqueduct () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Hospital"), "Hospital not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Hospital");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Hospital"), "Hospital not buildable without Aqueduct");
    
    set_building_tech_researched(setup.techs, "Aqueduct");
    set_building_built(setup.built, "Aqueduct");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Hospital"), "Hospital buildable with tech and Aqueduct");
    summarize_test_results();
}

void test_hydro_plant_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Hydro Plant"), "Hydro Plant not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Hydro Plant");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Hydro Plant"), "Hydro Plant buildable with tech");
    summarize_test_results();
}

void test_research_lab_requires_university () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Research Lab"), "Research Lab not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Research Lab");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Research Lab"), "Research Lab not buildable without University");
    
    set_building_tech_researched(setup.techs, "University");
    set_building_tech_researched(setup.techs, "Library");
    set_building_built(setup.built, "Library");
    set_building_built(setup.built, "University");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Research Lab"), "Research Lab buildable with tech and University");
    summarize_test_results();
}

void test_stock_exchange_requires_bank () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Stock Exchange"), "Stock Exchange not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Stock Exchange");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Stock Exchange"), "Stock Exchange not buildable without Bank");
    
    set_building_tech_researched(setup.techs, "Bank");
    set_building_tech_researched(setup.techs, "Marketplace");
    set_building_built(setup.built, "Marketplace");
    set_building_built(setup.built, "Bank");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Stock Exchange"), "Stock Exchange buildable with tech and Bank");
    summarize_test_results();
}

void test_recycling_center_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Recycling Center"), "Recycling Center not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Recycling Center");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Recycling Center"), "Recycling Center buildable with tech");
    summarize_test_results();
}

void test_civil_defense_requires_only_tech () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Civil Defense"), "Civil Defense not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Civil Defense");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Civil Defense"), "Civil Defense buildable with tech");
    summarize_test_results();
}

void test_intelligence_agency_requires_police_station () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Intelligence Agency"), "Intelligence Agency not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Intelligence Agency");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Intelligence Agency"), "Intelligence Agency not buildable without Police Station");
    
    set_building_tech_researched(setup.techs, "Police Station");
    set_building_built(setup.built, "Police Station");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Intelligence Agency"), "Intelligence Agency buildable with tech and Police Station");
    summarize_test_results();
}

void test_capitalization_requires_stock_exchange () {
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Capitalization"), "Capitalization not buildable w/o tech");
    
    set_building_tech_researched(setup.techs, "Capitalization");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Capitalization"), "Capitalization not buildable without Stock Exchange");
    
    set_building_tech_researched(setup.techs, "Stock Exchange");
    set_building_tech_researched(setup.techs, "Bank");
    set_building_tech_researched(setup.techs, "Marketplace");
    set_building_built(setup.built, "Marketplace");
    set_building_built(setup.built, "Bank");
    set_building_built(setup.built, "Stock Exchange");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Capitalization"), "Capitalization buildable with tech and Stock Exchange");
    summarize_test_results();
}

void test_all_oil_rubber_buildings () {
    TestSetup setup = create_test_setup();
    const char* oil_rubber_buildings[] = {"Airport", "Mass Transit", "Superhighways"};
    
    for (int i = 0; i < 3; i++) {
        set_building_tech_researched(setup.techs, oil_rubber_buildings[i]);
    }
    
    set_resource_available(setup.resources, "Oil");
    reassess_buildable(setup);
    for (int i = 0; i < 3; i++) {
        str msg = str("Oil+Rubber building ") + str(oil_rubber_buildings[i]) + " not buildable with only Oil";
        note_result (!is_buildable(setup.buildable, oil_rubber_buildings[i]), msg.c_str());
    }
    
    set_resource_available(setup.resources, "Rubber");
    reassess_buildable(setup);
    for (int i = 0; i < 3; i++) {
        str msg = str("Oil+Rubber building ") + str(oil_rubber_buildings[i]) + " buildable with Oil and Rubber";
        note_result (is_buildable(setup.buildable, oil_rubber_buildings[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_all_aluminum_buildings () {
    TestSetup setup = create_test_setup();
    const char* aluminum_buildings[] = {"SAM Missile Battery", "Solar Plant"};
    
    for (int i = 0; i < 2; i++) {
        set_building_tech_researched(setup.techs, aluminum_buildings[i]);
    }
    
    reassess_buildable(setup);
    for (int i = 0; i < 2; i++) {
        str msg = str("Aluminum building ") + str(aluminum_buildings[i]) + " not buildable w/o Aluminum";
        note_result (!is_buildable(setup.buildable, aluminum_buildings[i]), msg.c_str());
    }
    
    set_resource_available(setup.resources, "Aluminum");
    reassess_buildable(setup);
    for (int i = 0; i < 2; i++) {
        str msg = str("Aluminum building ") + str(aluminum_buildings[i]) + " buildable with Aluminum";
        note_result (is_buildable(setup.buildable, aluminum_buildings[i]), msg.c_str());
    }
    summarize_test_results();
}

void test_mixed_resource_requirements () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Factory");
    set_building_tech_researched(setup.techs, "Coal Plant");
    set_building_tech_researched(setup.techs, "Nuclear Plant");
    set_building_tech_researched(setup.techs, "Wall");
    
    set_resource_available(setup.resources, "Iron");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Factory"), "Factory needs Coal");
    note_result (!is_buildable(setup.buildable, "Coal Plant"), "Coal Plant needs Coal");
    note_result (!is_buildable(setup.buildable, "Nuclear Plant"), "Nuclear Plant needs Uranium");
    note_result (!is_buildable(setup.buildable, "Wall"), "Wall needs Stone");
    
    set_resource_available(setup.resources, "Coal");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Factory"), "Factory buildable with Iron and Coal");
    note_result (is_buildable(setup.buildable, "Coal Plant"), "Coal Plant buildable with Coal");
    
    set_resource_available(setup.resources, "Uranium");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Nuclear Plant"), "Nuclear Plant buildable with Uranium");
    
    set_resource_available(setup.resources, "Stone");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Wall"), "Wall buildable with Stone");
    summarize_test_results();
}

void test_building_not_researched_not_buildable () {
    TestSetup setup = create_test_setup();
    set_resource_available(setup.resources, "Iron");
    set_resource_available(setup.resources, "Coal");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Factory"), "Factory not buildable if not researched");
    
    set_building_tech_researched(setup.techs, "Factory");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Factory"), "Factory buildable when researched");
    summarize_test_results();
}

void test_determine_buildable_buildings_idempotent () {
    TestSetup setup = create_test_setup();
    set_building_tech_researched(setup.techs, "Factory");
    set_resource_available(setup.resources, "Iron");
    set_resource_available(setup.resources, "Coal");
    
    reassess_buildable(setup);
    bool first = is_buildable(setup.buildable, "Factory");
    
    reassess_buildable(setup);
    bool second = is_buildable(setup.buildable, "Factory");
    
    note_result (first == second && first == true, "determine_buildable_buildings idempotent");
    summarize_test_results();
}

void test_all_resources_all_buildings () {
    TestSetup setup = create_test_setup();
    u32 building_count = BuildingData::get_building_data_count();
    
    const char* all_resources[] = {"Iron", "Coal", "Saltpeter", "Oil", "Uranium", "Rubber", "Aluminum", "Stone"};
    for (int i = 0; i < 8; i++) {
        set_resource_available(setup.resources, all_resources[i]);
    }
    
    set_flag(setup.flags, "isCoastal");
    
    const BuildingTypeStats* buildings = BuildingData::get_building_data_array();
    for (u32 i = 0; i < building_count && i < 35; i++) {
        u32 tech_idx = buildings[i].tech_prereq_idx.get_idx();
        setup.techs->set_bit(tech_idx);
        
        // Build prerequisite buildings
        for (u32 r = 0; r < MAX_BUILDING_REQS; r++) {
            const BuildingRequirement& req = buildings[i].requirements[r];
            if (req.type == BUILDING_REQ_BUILDING) {
                u32 prereq_idx = req.data.building_req.building_idx;
                setup.built->set_built(prereq_idx);
            }
        }
    }
    
    reassess_buildable(setup);
    u32 buildable_count = 0;
    for (u32 i = 0; i < building_count && i < 35; i++) {
        if (setup.buildable->can_build(i)) {
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
    
    // Tests for buildings with resource requirements
    test_factory_requires_iron_and_coal();
    test_coastal_fortress_requires_saltpeter_iron_and_coastal_flag();
    test_airport_requires_oil_and_rubber();
    test_mass_transit_requires_oil_and_rubber();
    test_coal_plant_requires_coal();
    test_nuclear_plant_requires_uranium();
    test_manufacturing_plant_requires_iron_and_factory();
    test_superhighways_requires_oil_and_rubber();
    test_sam_missile_battery_requires_aluminum();
    test_offshore_platform_requires_oil_and_coastal_flag();
    test_wall_requires_stone();
    test_solar_plant_requires_aluminum();
    
    // Tests for buildings with only tech requirements
    test_barracks_requires_only_tech();
    test_granary_requires_only_tech();
    test_library_requires_only_tech();
    test_temple_requires_only_tech();
    test_marketplace_requires_only_tech();
    test_aqueduct_requires_only_tech();
    test_courthouse_requires_only_tech();
    test_colosseum_requires_only_tech();
    test_police_station_requires_only_tech();
    test_hydro_plant_requires_only_tech();
    test_recycling_center_requires_only_tech();
    test_civil_defense_requires_only_tech();
    
    // Tests for buildings with building prerequisites
    test_bank_requires_marketplace();
    test_cathedral_requires_temple();
    test_university_requires_library();
    test_commercial_dock_requires_harbor();
    test_hospital_requires_aqueduct();
    test_research_lab_requires_university();
    test_stock_exchange_requires_bank();
    test_intelligence_agency_requires_police_station();
    test_capitalization_requires_stock_exchange();
    
    // Tests for buildings with flag prerequisites
    test_harbor_requires_tech_and_coastal_flag();
    
    // Tests for buildings with resource and building prerequisites
    test_manufacturing_plant_requires_iron_and_factory();
    
    // Tests for buildings with resource and flag prerequisites
    test_offshore_platform_requires_oil_and_coastal_flag();
    
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
