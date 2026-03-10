//================================================================================================================================
//=> - Includes, globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "bit_array.h"
#include "tech_data.h"
#include "resource_data.h"
#include "building_data.h"
#include "building_vector.h"
#include "city_flags.h"
#include "wonder_data.h"
#include "wonder_vector.h"
#include "wonder_assessor.h"

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
    WonderData::load_static_data("../game_config.wonders");
    BuiltWonders::allocate_static_array();
}

static u32 find_tech_index (const char* tech_name) {
    const TechTypeStats* tech_array = TechData::get_tech_data_array();
    u32 tech_count = TechData::get_tech_data_count();
    if (tech_array == nullptr) {
        return UINT32_MAX;
    }
    for (u32 i = 0; i < tech_count; i++) {
        if (tech_array[i].name == tech_name) {
            return i;
        }
    }
    return UINT32_MAX;
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

static u32 find_wonder_index (const char* wonder_name) {
    const WonderTypeStats* wonders = WonderData::get_wonder_data_array();
    u32 wonder_count = WonderData::get_wonder_data_count();
    if (wonders == nullptr) {
        return UINT32_MAX;
    }
    for (u32 i = 0; i < wonder_count; i++) {
        if (wonders[i].name == wonder_name) {
            return i;
        }
    }
    return UINT32_MAX;
}

struct TestSetup {
    BitArrayCL* techs;
    BuiltBuildings* buildings;
    BitArrayCL* resources;
    BitArrayCL* flags;
    BuildableWonders* buildable;
    
    TestSetup () : 
        techs(nullptr), 
        buildings(nullptr), 
        resources(nullptr), 
        flags(nullptr),
        buildable(nullptr) {
    }
    
    ~TestSetup () {
        delete buildable;
        delete flags;
        delete resources;
        delete buildings;
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
    setup.buildings = new BuiltBuildings();
    setup.resources = new BitArrayCL(resource_count);
    setup.flags = new BitArrayCL(flag_count);
    setup.buildable = nullptr; // Will be created by assess()
    
    return setup;
}

void clear_all_built_wonders () {
    u32 wonder_count = WonderData::get_wonder_data_count();
    for (u32 i = 0; i < wonder_count; i++) {
        BuiltWonders::set_owning_city(i, 0);
    }
}

void set_resource_available (BitArrayCL* resources, const char* resource_name) {
    u32 idx = find_resource_index(resource_name);
    if (idx != UINT32_MAX) {
        resources->set_bit(idx);
    }
}

void set_tech_researched (BitArrayCL* techs, const char* tech_name) {
    u32 idx = find_tech_index(tech_name);
    if (idx != UINT32_MAX) {
        techs->set_bit(idx);
    }
}

void set_wonder_tech_researched (BitArrayCL* techs, const char* wonder_name) {
    u32 wonder_idx = find_wonder_index(wonder_name);
    if (wonder_idx != UINT32_MAX) {
        const WonderTypeStats* wonders = WonderData::get_wonder_data_array();
        u32 tech_idx = wonders[wonder_idx].tech_prereq_idx.get_idx();
        techs->set_bit(tech_idx);
    }
}

void set_flag (BitArrayCL* flags, const char* flag_name) {
    u16 flag_idx = CityFlagData::find_flag_index(flag_name);
    flags->set_bit(flag_idx);
}

void set_building_built (BuiltBuildings* buildings, const char* building_name) {
    u32 building_idx = find_building_index(building_name);
    if (building_idx != UINT32_MAX) {
        buildings->set_built(building_idx);
    }
}

bool builds (BuildableWonders* buildable, const char* wonder_name) {
    if (buildable == nullptr) {
        return false;
    }
    u32 idx = find_wonder_index(wonder_name);
    if (idx == UINT32_MAX) {
        return false;
    }
    return buildable->can_build(idx);
}

void reassess_buildable (TestSetup& setup) {
    if (setup.buildable != nullptr) {
        delete setup.buildable;
    }
    setup.buildable = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_wonder_assessor_construction () {
    setup_static_data();
    note_result (true, "WonderAssessor static class");
    summarize_test_results();
}

void test_pyramids_requires_only_masonry () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Pyramids"), "Pyramids not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Pyramids");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Pyramids"), "Pyramids builds w Masonry");
    summarize_test_results();
}

void test_great_library_requires_literature_and_library () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Great Library");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Great Library"), "Great Library not buildable w/o Library");
    
    set_building_built(setup.buildings, "Library");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Great Library"), "Great Library builds w Literature, Library");
    summarize_test_results();
}

void test_hanging_gardens_requires_mathematics_and_temple () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Hanging Gardens");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Hanging Gardens"), "Hanging Gardens not buildable w/o Temple");
    
    set_building_built(setup.buildings, "Temple");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Hanging Gardens"), "Hanging Gardens builds w Mathematics, Temple");
    summarize_test_results();
}

void test_colossus_requires_bronze_working_and_iron () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Colossus");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Colossus"), "Colossus not buildable w/o Iron");
    
    set_resource_available(setup.resources, "Iron");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Colossus"), "Colossus builds w Bronze Working, Iron");
    summarize_test_results();
}

void test_great_lighthouse_requires_navigation_harbor_and_isCoastal () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Great Lighthouse");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Great Lighthouse"), "Great Lighthouse not buildable w/o requirements");
    
    set_building_built(setup.buildings, "Harbor");
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Great Lighthouse"), "Great Lighthouse not builds wout isCoastal flag");
    
    set_flag(setup.flags, "isCoastal");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Great Lighthouse"), "Great Lighthouse builds w Navigation, Harbor, isCoastal flag");
    summarize_test_results();
}

void test_oracle_requires_only_philosophy () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Oracle"), "Oracle not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Oracle");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Oracle"), "Oracle builds w Philosophy");
    summarize_test_results();
}

void test_great_wall_requires_only_masonry () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Great Wall"), "Great Wall not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Great Wall");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Great Wall"), "Great Wall builds w Masonry");
    summarize_test_results();
}

void test_statue_of_zeus_requires_mathematics_and_ivory () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Statue of Zeus");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Statue of Zeus"), "Statue of Zeus not buildable w/o Ivory");
    
    set_resource_available(setup.resources, "Ivory");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Statue of Zeus"), "Statue of Zeus builds w Mathematics, Ivory");
    summarize_test_results();
}

void test_sun_tzus_art_of_war_requires_only_feudalism () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Sun Tzu's Art of War"), "Sun Tzu's Art of War not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Sun Tzu's Art of War");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Sun Tzu's Art of War"), "Sun Tzu's Art of War builds w Feudalism");
    summarize_test_results();
}

void test_leonardos_workshop_requires_only_invention () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Leonardo's Workshop"), "Leonardo's Workshop not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Leonardo's Workshop");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Leonardo's Workshop"), "Leonardo's Workshop builds w Invention");
    summarize_test_results();
}

void test_knights_templar_requires_only_chivalry () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Knights Templar"), "Knights Templar not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Knights Templar");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Knights Templar"), "Knights Templar builds w Chivalry");
    summarize_test_results();
}

void test_magellans_voyage_requires_navigation_and_isCoastal () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Magellan's Voyage");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Magellan's Voyage"), "Magellan's Voyage not buildable w/o isCoastal flag");
    
    set_flag(setup.flags, "isCoastal");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Magellan's Voyage"), "Magellan's Voyage builds w Navigation, isCoastal flag");
    summarize_test_results();
}

void test_copernicus_observatory_requires_only_astronomy () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Copernicus' Observatory"), "Copernicus' Observatory not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Copernicus' Observatory");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Copernicus' Observatory"), "Copernicus' Observatory builds w Astronomy");
    summarize_test_results();
}

void test_shakespeares_theatre_requires_only_literature () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Shakespeare's Theatre"), "Shakespeare's Theatre not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Shakespeare's Theatre");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Shakespeare's Theatre"), "Shakespeare's Theatre builds w Literature");
    summarize_test_results();
}

void test_newtons_university_requires_only_theory_of_gravity () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Newton's University"), "Newton's University not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Newton's University");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Newton's University"), "Newton's University builds w Theory of Gravity");
    summarize_test_results();
}

void test_js_bachs_cathedral_requires_theology_and_cathedral () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "J.S. Bach's Cathedral");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "J.S. Bach's Cathedral"), "J.S. Bach's Cathedral not buildable w/o Cathedral");
    
    set_building_built(setup.buildings, "Cathedral");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "J.S. Bach's Cathedral"), "J.S. Bach's Cathedral builds w Theology, Cathedral");
    summarize_test_results();
}

void test_adam_smiths_trading_company_requires_economics_and_bank () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Adam Smith's Trading Company");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Adam Smith's Trading Company"), "Adam Smith's Trading Company not buildable w/o Bank");
    
    set_building_built(setup.buildings, "Bank");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Adam Smith's Trading Company"), "Adam Smith's Trading Company builds w Economics, Bank");
    summarize_test_results();
}

void test_darwins_voyage_requires_medicine_and_isCoastal () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Darwin's Voyage");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Darwin's Voyage"), "Darwin's Voyage not buildable w/o isCoastal flag");
    
    set_flag(setup.flags, "isCoastal");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Darwin's Voyage"), "Darwin's Voyage builds w Medicine, isCoastal flag");
    summarize_test_results();
}

void test_statue_of_liberty_requires_only_democracy () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Statue of Liberty"), "Statue of Liberty not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Statue of Liberty");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Statue of Liberty"), "Statue of Liberty builds w Democracy");
    summarize_test_results();
}

void test_eiffel_tower_requires_only_steel () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Eiffel Tower"), "Eiffel Tower not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Eiffel Tower");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Eiffel Tower"), "Eiffel Tower builds w Steel");
    summarize_test_results();
}

void test_hoover_dam_requires_electronics_and_factory () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Hoover Dam");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Hoover Dam"), "Hoover Dam not buildable w/o Factory");
    
    set_building_built(setup.buildings, "Factory");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Hoover Dam"), "Hoover Dam builds w Electronics, Factory");
    summarize_test_results();
}

void test_united_nations_requires_only_mass_production () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "United Nations"), "United Nations not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "United Nations");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "United Nations"), "United Nations builds w Mass Production");
    summarize_test_results();
}

void test_seti_program_requires_computers_and_research_lab () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "SETI Program");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "SETI Program"), "SETI Program not buildable w/o Research Lab");
    
    set_building_built(setup.buildings, "Research Lab");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "SETI Program"), "SETI Program builds w Computers, Research Lab");
    summarize_test_results();
}

void test_cure_for_cancer_requires_genetics_and_hospital () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Cure for Cancer");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Cure for Cancer"), "Cure for Cancer not buildable w/o Hospital");
    
    set_building_built(setup.buildings, "Hospital");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Cure for Cancer"), "Cure for Cancer builds w Genetics, Hospital");
    summarize_test_results();
}

void test_longevity_requires_genetics_and_hospital () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Longevity");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Longevity"), "Longevity not buildable w/o Hospital");
    
    set_building_built(setup.buildings, "Hospital");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Longevity"), "Longevity builds w Genetics, Hospital");
    summarize_test_results();
}

void test_manhattan_project_requires_nuclear_power_uranium_and_nuclear_plant () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Manhattan Project");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Manhattan Project"), "Manhattan Project not buildable w/o requirements");
    
    set_resource_available(setup.resources, "Uranium");
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Manhattan Project"), "Manhattan Project not builds wout Nuclear Plant");
    
    set_building_built(setup.buildings, "Nuclear Plant");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Manhattan Project"), "Manhattan Project builds w Nuclear Power, Uranium, Nuclear Plant");
    summarize_test_results();
}

void test_apollo_program_requires_space_flight_aluminum_and_research_lab () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Apollo Program");
    
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Apollo Program"), "Apollo Program not buildable w/o requirements");
    
    set_resource_available(setup.resources, "Aluminum");
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Apollo Program"), "Apollo Program not builds wout Research Lab");
    
    set_building_built(setup.buildings, "Research Lab");
    reassess_buildable(setup);
    note_result (builds(setup.buildable, "Apollo Program"), "Apollo Program builds w Space Flight, Aluminum, Research Lab");
    summarize_test_results();
}

void test_wonder_not_buildable_when_already_built () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Pyramids");
    
    reassess_buildable(setup);
    bool first = builds(setup.buildable, "Pyramids");
    note_result (first, "Pyramids buildable when not built");
    
    u32 pyramids_idx = find_wonder_index("Pyramids");
    BuiltWonders::set_owning_city(pyramids_idx, 1);
    reassess_buildable(setup);
    note_result (!builds(setup.buildable, "Pyramids"), "Pyramids not buildable when already built");
    summarize_test_results();
}

void test_wonder_assessor_idempotent () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Pyramids");
    
    reassess_buildable(setup);
    bool first = builds(setup.buildable, "Pyramids");
    
    reassess_buildable(setup);
    bool second = builds(setup.buildable, "Pyramids");
    
    note_result (first == second && first == true, "WonderAssessor assess idempotent");
    summarize_test_results();
}

void test_all_resources_all_wonders () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    u32 wonder_count = WonderData::get_wonder_data_count();
    
    const char* all_resources[] = {"Iron", "Coal", "Saltpeter", "Oil", "Uranium", "Rubber", "Aluminum", "Stone", "Ivory"};
    for (int i = 0; i < 9; i++) {
        set_resource_available(setup.resources, all_resources[i]);
    }
    
    set_flag(setup.flags, "isCoastal");
    
    // Set all techs
    u32 tech_count = TechData::get_tech_data_count();
    for (u32 i = 0; i < tech_count; i++) {
        setup.techs->set_bit(i);
    }
    
    // Set all buildings
    u32 building_count = BuildingData::get_building_data_count();
    for (u32 i = 0; i < building_count; i++) {
        setup.buildings->set_built(i);
    }
    
    reassess_buildable(setup);
    u32 buildable_count = 0;
    for (u32 i = 0; i < wonder_count; i++) {
        if (setup.buildable->can_build(i)) {
            buildable_count++;
        }
    }
    note_result (buildable_count > 0, "Some wonders builds w all resources");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    test_wonder_assessor_construction();
    
    // Tests for wonders with only tech requirements
    test_pyramids_requires_only_masonry();
    test_oracle_requires_only_philosophy();
    test_great_wall_requires_only_masonry();
    test_sun_tzus_art_of_war_requires_only_feudalism();
    test_leonardos_workshop_requires_only_invention();
    test_knights_templar_requires_only_chivalry();
    test_copernicus_observatory_requires_only_astronomy();
    test_shakespeares_theatre_requires_only_literature();
    test_newtons_university_requires_only_theory_of_gravity();
    test_statue_of_liberty_requires_only_democracy();
    test_eiffel_tower_requires_only_steel();
    test_united_nations_requires_only_mass_production();
    
    // Tests for wonders with building requirements
    test_great_library_requires_literature_and_library();
    test_hanging_gardens_requires_mathematics_and_temple();
    test_js_bachs_cathedral_requires_theology_and_cathedral();
    test_adam_smiths_trading_company_requires_economics_and_bank();
    test_hoover_dam_requires_electronics_and_factory();
    test_seti_program_requires_computers_and_research_lab();
    test_cure_for_cancer_requires_genetics_and_hospital();
    test_longevity_requires_genetics_and_hospital();
    
    // Tests for wonders with resource requirements
    test_colossus_requires_bronze_working_and_iron();
    test_statue_of_zeus_requires_mathematics_and_ivory();
    
    // Tests for wonders with flag requirements
    test_great_lighthouse_requires_navigation_harbor_and_isCoastal();
    test_magellans_voyage_requires_navigation_and_isCoastal();
    test_darwins_voyage_requires_medicine_and_isCoastal();
    
    // Tests for wonders with resource, building requirements
    test_manhattan_project_requires_nuclear_power_uranium_and_nuclear_plant();
    test_apollo_program_requires_space_flight_aluminum_and_research_lab();
    
    // Comprehensive tests
    test_wonder_not_buildable_when_already_built();
    test_wonder_assessor_idempotent();
    test_all_resources_all_wonders();

    printf("=======================================================\n");
    printf(" TESTING WONDER ASSESSOR: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
