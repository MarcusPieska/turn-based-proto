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
#include "city_flags.h"
#include "wonder_data.h"
#include "wonder_vector.h"
#include "city.h"

typedef const char* cstr;
typedef std::string str;
typedef uint32_t u32;
typedef uint16_t u16;

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
        total_test_fails++;
        if (print_level > 0) {
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void note_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_result(cond, msg.c_str());
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
    test_pass = 0;
}

static u16 find_tech_index (const char* tech_name) {
    const TechTypeStats* tech_array = TechData::get_tech_data_array();
    u16 tech_count = TechData::get_tech_data_count();
    for (u16 i = 0; i < tech_count; ++i) {
        if (tech_array[i].name == tech_name) {
            return i;
        }
    }
    return static_cast<u16>(UINT16_MAX);
}

static u16 find_building_index (const char* building_name) {
    const BuildingTypeStats* building_array = BuildingData::get_building_data_array();
    u16 building_count = BuildingData::get_building_data_count();
    for (u16 i = 0; i < building_count; ++i) {
        if (building_array[i].name == building_name) {
            return i;
        }
    }
    return static_cast<u16>(UINT16_MAX);
}

static u16 find_resource_index (const char* resource_name) {
    const ResourceTypeStats* resource_array = ResourceData::get_resource_data_array();
    u16 resource_count = ResourceData::get_resource_data_count();
    for (u16 i = 0; i < resource_count; ++i) {
        if (resource_array[i].name == resource_name) {
            return i;
        }
    }
    return static_cast<u16>(UINT16_MAX);
}

static u16 find_flag_index (const char* flag_name) {
    const CityFlagStats* flag_array = CityFlagData::get_flag_data_array();
    u16 flag_count = CityFlagData::get_flag_count();
    for (u16 i = 0; i < flag_count; ++i) {
        if (flag_array[i].name == flag_name) {
            return i;
        }
    }
    return static_cast<u16>(UINT16_MAX);
}

static u16 find_wonder_index (const char* wonder_name) {
    const WonderTypeStats* wonders = WonderData::get_wonder_data_array();
    u16 wonder_count = WonderData::get_wonder_data_count();
    for (u16 i = 0; i < wonder_count; ++i) {
        if (wonders[i].name == wonder_name) {
            return i;
        }
    }
    return static_cast<u16>(UINT16_MAX);
}

//================================================================================================================================
//=> - Test setup -
//================================================================================================================================

struct WonderTestSetup {
    BitArrayCL* techs;
    BitArrayCL* buildings;
    BitArrayCL* resources;
    BitArrayCL* flags;
    u32 tech_count;
    u32 building_count;
    u32 resource_count;
    u32 flag_count;

    WonderTestSetup () :
        techs(nullptr),
        buildings(nullptr),
        resources(nullptr),
        flags(nullptr),
        tech_count(0),
        building_count(0),
        resource_count(0),
        flag_count(0) {
    }

    ~WonderTestSetup () {
        delete techs;
        delete buildings;
        delete resources;
        delete flags;
    }
};

static WonderTestSetup create_wonder_test_setup () {
    WonderTestSetup setup;
    setup.tech_count = TechData::get_tech_data_count();
    setup.building_count = BuildingData::get_building_data_count();
    setup.resource_count = ResourceData::get_resource_data_count();
    setup.flag_count = CityFlagData::get_flag_count();

    setup.techs = new BitArrayCL(setup.tech_count);
    setup.buildings = new BitArrayCL(setup.building_count);
    setup.resources = new BitArrayCL(setup.resource_count);
    setup.flags = new BitArrayCL(setup.flag_count);

    return setup;
}

static void clear_all_built_wonders () {
    u16 wonder_count = WonderData::get_wonder_data_count();
    for (u16 i = 0; i < wonder_count; ++i) {
        WondersBuiltVector::set_owning_city(i, 0);
    }
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_city_constructor () {
    City city;
    note_result(true, "City constructor");
    summarize_test_results();
}

void test_city_add_food () {
    City city;
    city.add_food(10);
    city.add_food(5);
    note_result(true, "City add_food");
    summarize_test_results();
}

void test_city_add_shields () {
    City city;
    city.add_shields(20);
    city.add_shields(15);
    note_result(true, "City add_shields");
    summarize_test_results();
}

void test_city_build_functions () {
    City city;
    u16 building_idx = find_building_index("Barracks");
    u16 wonder_idx = find_wonder_index("Pyramids");
    
    if (building_idx != UINT16_MAX) {
        city.build_building(building_idx);
        note_result(true, "City build_building");
    }
    
    if (wonder_idx != UINT16_MAX) {
        city.build_wonder(wonder_idx);
        note_result(true, "City build_wonder");
    }
    
    city.build_small_wonder(0);
    city.build_unit(0);
    note_result(true, "City build_small_wonder and build_unit");
    summarize_test_results();
}

void test_city_get_functions () {
    City city;
    BitArrayCL* techs = new BitArrayCL(TechData::get_tech_data_count());
    BitArrayCL* buildings = city.get_buildable_buildings(techs);
    WonderBuildableVector* wonders = city.get_buildable_wonders(techs);
    BitArrayCL* small_wonders = city.get_buildable_small_wonders(techs);
    BitArrayCL* units = city.get_trainable_units(techs);
    
    note_result(buildings == nullptr, "City get_buildable_buildings returns nullptr (not yet implemented)");
    note_result(wonders == nullptr, "City get_buildable_wonders returns nullptr (not yet implemented)");
    note_result(small_wonders == nullptr, "City get_buildable_small_wonders returns nullptr (not yet implemented)");
    note_result(units == nullptr, "City get_trainable_units returns nullptr (not yet implemented)");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main function -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    TechData::load_static_data("../game_config.techs");
    ResourceData::load_static_data("../game_config.resources");
    CityFlagData::load_static_data("../game_config.city_flags");
    BuildingData::load_static_data("../game_config.buildings");
    WonderData::load_static_data("../game_config.wonders");
    WondersBuiltVector::allocate_static_array();

    test_city_constructor();
    test_city_add_food();
    test_city_add_shields();
    test_city_build_functions();
    test_city_get_functions();

    std::printf("=======================================================\n");
    std::printf(" TESTING CITY: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
