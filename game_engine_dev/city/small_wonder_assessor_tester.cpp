//================================================================================================================================
//=> - Includes and globals -
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
        printf("*** TEST FAILED: %s\n", msg);
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
    WonderData::load_static_data("../game_config.wonders_small");
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

bool is_buildable (BuildableWonders* buildable, const char* wonder_name) {
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

void test_palace_requires_masonry_and_isCapital () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Palace");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Palace"), "Palace not buildable w/o isCapital flag");
    
    set_flag(setup.flags, "isCapital");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Palace"), "Palace buildable with Masonry and isCapital flag");
    summarize_test_results();
}

void test_forbidden_palace_requires_masonry_and_courthouse () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Forbidden Palace");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Forbidden Palace"), "Forbidden Palace not buildable w/o Courthouse");
    
    set_building_built(setup.buildings, "Courthouse");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Forbidden Palace"), "Forbidden Palace buildable with Masonry and Courthouse");
    summarize_test_results();
}

void test_military_academy_requires_military_tradition_and_victories () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Military Academy");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Military Academy"), "Military Academy not buildable w/o victories flag");
    
    set_flag(setup.flags, "victories");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Military Academy"), "Military Academy buildable with Military Tradition and victories flag");
    summarize_test_results();
}

void test_pentagon_requires_military_tradition_and_barracks () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Pentagon");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Pentagon"), "Pentagon not buildable w/o Barracks");
    
    set_building_built(setup.buildings, "Barracks");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Pentagon"), "Pentagon buildable with Military Tradition and Barracks");
    summarize_test_results();
}

void test_battlefield_medicine_requires_medicine_and_hospital () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Battlefield Medicine");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Battlefield Medicine"), "Battlefield Medicine not buildable w/o Hospital");
    
    set_building_built(setup.buildings, "Hospital");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Battlefield Medicine"), "Battlefield Medicine buildable with Medicine and Hospital");
    summarize_test_results();
}

void test_wall_street_requires_economics_and_bank () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Wall Street");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Wall Street"), "Wall Street not buildable w/o Bank");
    
    set_building_built(setup.buildings, "Bank");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Wall Street"), "Wall Street buildable with Economics and Bank");
    summarize_test_results();
}

void test_intelligence_agency_wonder_requires_espionage_and_police_station () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Intelligence Agency");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Intelligence Agency"), "Intelligence Agency not buildable w/o Police Station");
    
    set_building_built(setup.buildings, "Police Station");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Intelligence Agency"), "Intelligence Agency buildable with Espionage and Police Station");
    summarize_test_results();
}

void test_apollo_program_requires_space_flight_and_aluminum () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Apollo Program");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Apollo Program"), "Apollo Program not buildable w/o Aluminum");
    
    set_resource_available(setup.resources, "Aluminum");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Apollo Program"), "Apollo Program buildable with Space Flight and Aluminum");
    summarize_test_results();
}

void test_ironworks_requires_chemistry_iron_and_coal () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Ironworks");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Ironworks"), "Ironworks not buildable w/o resources");
    
    set_resource_available(setup.resources, "Iron");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Ironworks"), "Ironworks not buildable with only Iron");
    
    set_resource_available(setup.resources, "Coal");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Ironworks"), "Ironworks buildable with Chemistry, Iron and Coal");
    summarize_test_results();
}

void test_national_college_requires_philosophy_and_library () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "National College");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "National College"), "National College not buildable w/o Library");
    
    set_building_built(setup.buildings, "Library");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "National College"), "National College buildable with Philosophy and Library");
    summarize_test_results();
}

void test_national_epic_requires_literature_and_library () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "National Epic");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "National Epic"), "National Epic not buildable w/o Library");
    
    set_building_built(setup.buildings, "Library");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "National Epic"), "National Epic buildable with Literature and Library");
    summarize_test_results();
}

void test_heroic_epic_requires_military_tradition_victories_and_barracks () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Heroic Epic");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Heroic Epic"), "Heroic Epic not buildable w/o requirements");
    
    set_flag(setup.flags, "victories");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Heroic Epic"), "Heroic Epic not buildable without Barracks");
    
    set_building_built(setup.buildings, "Barracks");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Heroic Epic"), "Heroic Epic buildable with Military Tradition, victories flag and Barracks");
    summarize_test_results();
}

void test_oxford_university_requires_education_and_university () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Oxford University");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Oxford University"), "Oxford University not buildable w/o University");
    
    set_building_built(setup.buildings, "University");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Oxford University"), "Oxford University buildable with Education and University");
    summarize_test_results();
}

void test_hermitage_requires_only_nationalism () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Hermitage"), "Hermitage not buildable w/o tech");
    
    set_wonder_tech_researched(setup.techs, "Hermitage");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Hermitage"), "Hermitage buildable with Nationalism");
    summarize_test_results();
}

void test_circus_maximus_requires_mathematics_and_colosseum () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Circus Maximus");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Circus Maximus"), "Circus Maximus not buildable w/o Colosseum");
    
    set_building_built(setup.buildings, "Colosseum");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Circus Maximus"), "Circus Maximus buildable with Mathematics and Colosseum");
    summarize_test_results();
}

void test_grand_temple_requires_theology_and_temple () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Grand Temple");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Grand Temple"), "Grand Temple not buildable w/o Temple");
    
    set_building_built(setup.buildings, "Temple");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Grand Temple"), "Grand Temple buildable with Theology and Temple");
    summarize_test_results();
}

void test_east_india_company_requires_navigation_harbor_and_luxaries () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "East India Company");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "East India Company"), "East India Company not buildable w/o requirements");
    
    set_building_built(setup.buildings, "Harbor");
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "East India Company"), "East India Company not buildable without luxaries flag");
    
    set_flag(setup.flags, "luxaries");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "East India Company"), "East India Company buildable with Navigation, Harbor and luxaries flag");
    summarize_test_results();
}

void test_national_intelligence_agency_requires_radio_and_police_station () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "National Intelligence Agency");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "National Intelligence Agency"), "National Intelligence Agency not buildable w/o Police Station");
    
    set_building_built(setup.buildings, "Police Station");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "National Intelligence Agency"), "National Intelligence Agency buildable with Radio and Police Station");
    summarize_test_results();
}

void test_red_cross_requires_electricity_and_hospital () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Red Cross");
    
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Red Cross"), "Red Cross not buildable w/o Hospital");
    
    set_building_built(setup.buildings, "Hospital");
    reassess_buildable(setup);
    note_result (is_buildable(setup.buildable, "Red Cross"), "Red Cross buildable with Electricity and Hospital");
    summarize_test_results();
}

void test_wonder_not_buildable_when_already_built () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Hermitage");
    
    reassess_buildable(setup);
    bool first = is_buildable(setup.buildable, "Hermitage");
    note_result (first, "Hermitage buildable when not built");
    
    u32 hermitage_idx = find_wonder_index("Hermitage");
    BuiltWonders::set_owning_city(hermitage_idx, 1);
    reassess_buildable(setup);
    note_result (!is_buildable(setup.buildable, "Hermitage"), "Hermitage not buildable when already built");
    summarize_test_results();
}

void test_wonder_assessor_idempotent () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    set_wonder_tech_researched(setup.techs, "Hermitage");
    
    reassess_buildable(setup);
    bool first = is_buildable(setup.buildable, "Hermitage");
    
    reassess_buildable(setup);
    bool second = is_buildable(setup.buildable, "Hermitage");
    
    note_result (first == second && first == true, "WonderAssessor assess idempotent");
    summarize_test_results();
}

void test_all_resources_all_wonders () {
    clear_all_built_wonders();
    TestSetup setup = create_test_setup();
    u32 wonder_count = WonderData::get_wonder_data_count();
    
    const char* all_resources[] = {"Iron", "Coal", "Saltpeter", "Oil", "Uranium", "Rubber", "Aluminum", "Stone"};
    for (int i = 0; i < 8; i++) {
        set_resource_available(setup.resources, all_resources[i]);
    }
    
    set_flag(setup.flags, "isCoastal");
    set_flag(setup.flags, "isCapital");
    set_flag(setup.flags, "victories");
    set_flag(setup.flags, "luxaries");
    
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
    note_result (buildable_count > 0, "Some wonders buildable with all resources");
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
    
    // Tests for small wonders with flag requirements
    test_palace_requires_masonry_and_isCapital();
    
    // Tests for small wonders with building requirements
    test_forbidden_palace_requires_masonry_and_courthouse();
    test_pentagon_requires_military_tradition_and_barracks();
    test_battlefield_medicine_requires_medicine_and_hospital();
    test_wall_street_requires_economics_and_bank();
    test_intelligence_agency_wonder_requires_espionage_and_police_station();
    test_national_college_requires_philosophy_and_library();
    test_national_epic_requires_literature_and_library();
    test_oxford_university_requires_education_and_university();
    test_circus_maximus_requires_mathematics_and_colosseum();
    test_grand_temple_requires_theology_and_temple();
    test_national_intelligence_agency_requires_radio_and_police_station();
    test_red_cross_requires_electricity_and_hospital();
    
    // Tests for small wonders with flag and building requirements
    test_military_academy_requires_military_tradition_and_victories();
    test_heroic_epic_requires_military_tradition_victories_and_barracks();
    test_east_india_company_requires_navigation_harbor_and_luxaries();
    
    // Tests for small wonders with resource requirements
    test_apollo_program_requires_space_flight_and_aluminum();
    test_ironworks_requires_chemistry_iron_and_coal();
    
    // Tests for small wonders with only tech requirements
    test_hermitage_requires_only_nationalism();
    
    // Comprehensive tests
    test_wonder_not_buildable_when_already_built();
    test_wonder_assessor_idempotent();
    test_all_resources_all_wonders();

    printf("=======================================================\n");
    printf(" TESTING SMALL WONDER ASSESSOR: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
