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
#include "wonder_assessor.h"

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

void test_wonder_assessor_basic () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    WonderBuildableVector* wb = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    u16 pyramids_idx = find_wonder_index("Pyramids");
    note_result(!wb->can_build(pyramids_idx), "Pyramids not buildable with no techs/resources/buildings/flags");
    delete wb;
    summarize_test_results();
}

void test_pyramids_requires_masonry () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 pyramids_idx = find_wonder_index("Pyramids");
    WonderBuildableVector* wb1 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(!wb1->can_build(pyramids_idx), "Pyramids not buildable without Masonry");
    delete wb1;

    u16 masonry_idx = find_tech_index("Masonry");
    setup.techs->set_bit(masonry_idx);
    WonderBuildableVector* wb2 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(wb2->can_build(pyramids_idx), "Pyramids buildable with Masonry");
    delete wb2;
    summarize_test_results();
}

void test_great_library_requires_literature_and_library () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 gl_idx = find_wonder_index("Great Library");
    u16 lit_idx = find_tech_index("Literature");
    u16 lib_idx = find_building_index("Library");

    setup.techs->set_bit(lit_idx);
    WonderBuildableVector* wb1 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(!wb1->can_build(gl_idx), "Great Library not buildable without Library");
    delete wb1;

    setup.buildings->set_bit(lib_idx);
    WonderBuildableVector* wb2 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(wb2->can_build(gl_idx), "Great Library buildable with Literature and Library");
    delete wb2;
    summarize_test_results();
}

void test_hanging_gardens_requires_mathematics_and_temple () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 hg_idx = find_wonder_index("Hanging Gardens");
    u16 math_idx = find_tech_index("Mathematics");
    u16 temple_idx = find_building_index("Temple");

    setup.techs->set_bit(math_idx);
    WonderBuildableVector* wb1 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(!wb1->can_build(hg_idx), "Hanging Gardens not buildable without Temple");
    delete wb1;

    setup.buildings->set_bit(temple_idx);
    WonderBuildableVector* wb2 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(wb2->can_build(hg_idx), "Hanging Gardens buildable with Mathematics and Temple");
    delete wb2;
    summarize_test_results();
}

void test_colossus_requires_iron () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 col_idx = find_wonder_index("Colossus");
    u16 bronze_idx = find_tech_index("Bronze Working");
    u16 iron_res_idx = find_resource_index("Iron");

    setup.techs->set_bit(bronze_idx);
    WonderBuildableVector* wb1 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(!wb1->can_build(col_idx), "Colossus not buildable without Iron");
    delete wb1;

    setup.resources->set_bit(iron_res_idx);
    WonderBuildableVector* wb2 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(wb2->can_build(col_idx), "Colossus buildable with Iron");
    delete wb2;
    summarize_test_results();
}

void test_great_lighthouse_requires_harbor_and_isCoastal () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 glh_idx = find_wonder_index("Great Lighthouse");
    u16 nav_idx = find_tech_index("Navigation");
    u16 harbor_idx = find_building_index("Harbor");
    u16 coastal_flag_idx = find_flag_index("isCoastal");

    setup.techs->set_bit(nav_idx);
    setup.buildings->set_bit(harbor_idx);
    WonderBuildableVector* wb1 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(!wb1->can_build(glh_idx), "Great Lighthouse not buildable without isCoastal");
    delete wb1;

    setup.flags->set_bit(coastal_flag_idx);
    WonderBuildableVector* wb2 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(wb2->can_build(glh_idx), "Great Lighthouse buildable with Harbor and isCoastal");
    delete wb2;
    summarize_test_results();
}

void test_statue_of_zeus_requires_ivory () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 soz_idx = find_wonder_index("Statue of Zeus");
    u16 math_idx = find_tech_index("Mathematics");
    u16 ivory_idx = find_resource_index("Ivory");

    setup.techs->set_bit(math_idx);
    WonderBuildableVector* wb1 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(!wb1->can_build(soz_idx), "Statue of Zeus not buildable without Ivory");
    delete wb1;

    setup.resources->set_bit(ivory_idx);
    WonderBuildableVector* wb2 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(wb2->can_build(soz_idx), "Statue of Zeus buildable with Ivory");
    delete wb2;
    summarize_test_results();
}

void test_manhattan_project_requires_uranium_and_nuclear_plant () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 man_idx = find_wonder_index("Manhattan Project");
    u16 nuclear_power_idx = find_tech_index("Nuclear Power");
    u16 uranium_idx = find_resource_index("Uranium");
    u16 np_building_idx = find_building_index("Nuclear Plant");

    setup.techs->set_bit(nuclear_power_idx);
    WonderBuildableVector* wb1 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(!wb1->can_build(man_idx), "Manhattan Project not buildable without Uranium and Nuclear Plant");
    delete wb1;

    setup.resources->set_bit(uranium_idx);
    setup.buildings->set_bit(np_building_idx);
    WonderBuildableVector* wb2 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(wb2->can_build(man_idx), "Manhattan Project buildable with Uranium and Nuclear Plant");
    delete wb2;
    summarize_test_results();
}

void test_apollo_program_requires_aluminum_and_research_lab () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 ap_idx = find_wonder_index("Apollo Program");
    u16 space_flight_idx = find_tech_index("Space Flight");
    u16 aluminum_idx = find_resource_index("Aluminum");
    u16 lab_idx = find_building_index("Research Lab");

    setup.techs->set_bit(space_flight_idx);
    WonderBuildableVector* wb1 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(!wb1->can_build(ap_idx), "Apollo Program not buildable without Aluminum and Research Lab");
    delete wb1;

    setup.resources->set_bit(aluminum_idx);
    setup.buildings->set_bit(lab_idx);
    WonderBuildableVector* wb2 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(wb2->can_build(ap_idx), "Apollo Program buildable with Aluminum and Research Lab");
    delete wb2;
    summarize_test_results();
}

void test_wonder_not_buildable_when_already_built () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 pyramids_idx = find_wonder_index("Pyramids");
    u16 masonry_idx = find_tech_index("Masonry");

    setup.techs->set_bit(masonry_idx);
    WondersBuiltVector::set_owning_city(pyramids_idx, 1);

    WonderBuildableVector* wb = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    note_result(!wb->can_build(pyramids_idx), "Pyramids not buildable when already built");
    delete wb;
    summarize_test_results();
}

void test_wonder_assessor_idempotent () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();
    u16 pyramids_idx = find_wonder_index("Pyramids");
    u16 masonry_idx = find_tech_index("Masonry");

    setup.techs->set_bit(masonry_idx);

    WonderBuildableVector* wb1 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    bool first = wb1->can_build(pyramids_idx);

    WonderBuildableVector* wb2 = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);
    bool second = wb2->can_build(pyramids_idx);

    note_result(first == second && first == true, "WonderAssessor assess idempotent");
    delete wb1;
    delete wb2;
    summarize_test_results();
}

void test_all_resources_all_wonders () {
    clear_all_built_wonders();
    WonderTestSetup setup = create_wonder_test_setup();

    u16 tech_count = TechData::get_tech_data_count();
    for (u16 i = 0; i < tech_count; ++i) {
        setup.techs->set_bit(i);
    }

    u16 building_count = BuildingData::get_building_data_count();
    for (u16 i = 0; i < building_count; ++i) {
        setup.buildings->set_bit(i);
    }

    u16 resource_count = ResourceData::get_resource_data_count();
    for (u16 i = 0; i < resource_count; ++i) {
        setup.resources->set_bit(i);
    }

    u16 flag_count = CityFlagData::get_flag_count();
    for (u16 i = 0; i < flag_count; ++i) {
        setup.flags->set_bit(i);
    }

    WonderBuildableVector* wb = WonderAssessor::assess(setup.techs, setup.buildings, setup.resources, setup.flags);

    u16 wonder_count = WonderData::get_wonder_data_count();
    u32 buildable_count = 0;
    for (u16 i = 0; i < wonder_count; ++i) {
        if (wb->can_build(i)) {
            buildable_count++;
        }
    }

    note_result(buildable_count > 0, "Some wonders buildable with all techs/buildings/resources/flags");
    delete wb;
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

    test_wonder_assessor_basic();
    test_pyramids_requires_masonry();
    test_great_library_requires_literature_and_library();
    test_hanging_gardens_requires_mathematics_and_temple();
    test_colossus_requires_iron();
    test_great_lighthouse_requires_harbor_and_isCoastal();
    test_statue_of_zeus_requires_ivory();
    test_manhattan_project_requires_uranium_and_nuclear_plant();
    test_apollo_program_requires_aluminum_and_research_lab();
    test_wonder_not_buildable_when_already_built();
    test_wonder_assessor_idempotent();
    test_all_resources_all_wonders();

    std::printf("=======================================================\n");
    std::printf(" TESTING WONDER ASSESSOR: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
