//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>

#include "bit_array.h"
#include "unit_vector.h"
#include "resources_vector.h"
#include "city_flags.h"
#include "tech_data.h"
#include "resource_data.h"
#include "unit_data.h"
#include "building_data.h"
#include "building_vector.h"
#include "unit_assessor.h"

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
//=> - Result struct and globals -
//================================================================================================================================

const u16 MAX_TECHS = 1;
const u16 MAX_FLAGS = 4;
const u16 MAX_RESOURCES = 4;
const u16 MAX_BUILDINGS = 4;
const u32 MAX_UNITS = 128;

struct UnitResult {
    u16 tech_indices[MAX_TECHS];
    u16 flag_indices[MAX_FLAGS];
    u16 resource_indices[MAX_RESOURCES];
    u16 building_indices[MAX_BUILDINGS];
    u16 tech_count;
    u16 flag_count;
    u16 resource_count;
    u16 building_count;

    UnitResult() {
        tech_count = 0;
        flag_count = 0;
        resource_count = 0;
        building_count = 0;
    }
};

static UnitResult g_unit_results[MAX_UNITS];
static u32 g_unit_count = 0;

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
        std::printf("*** TEST FAILED: %s\n", msg);
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
        std::printf(" Test pass:  %d\n", test_pass);
        std::printf(" Test fail:  %d\n", test_count - test_pass);
        std::printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass  = 0;
}

static void add_tech_prereq (UnitResult& ur, u16 tech_idx) {
    for (u16 i = 0; i < ur.tech_count; ++i) {
        if (ur.tech_indices[i] == tech_idx) {
            return;
        }
    }
    if (ur.tech_count < MAX_TECHS) {
        ur.tech_indices[ur.tech_count++] = tech_idx;
    }
}

static void add_flag_prereq (UnitResult& ur, u16 flag_idx) {
    for (u16 i = 0; i < ur.flag_count; ++i) {
        if (ur.flag_indices[i] == flag_idx) {
            return;
        }
    }
    if (ur.flag_count < MAX_FLAGS) {
        ur.flag_indices[ur.flag_count++] = flag_idx;
    }
}

static void add_resource_prereq (UnitResult& ur, u16 res_idx) {
    for (u16 i = 0; i < ur.resource_count; ++i) {
        if (ur.resource_indices[i] == res_idx) {
            return;
        }
    }
    if (ur.resource_count < MAX_RESOURCES) {
        ur.resource_indices[ur.resource_count++] = res_idx;
    }
}

static void add_building_prereq (UnitResult& ur, u16 bld_idx) {
    for (u16 i = 0; i < ur.building_count; ++i) {
        if (ur.building_indices[i] == bld_idx) {
            return;
        }
    }
    if (ur.building_count < MAX_BUILDINGS) {
        ur.building_indices[ur.building_count++] = bld_idx;
    }
}

static void set_all_bits (
    BitArrayCL* techs,
    BitArrayCL* resources,
    BitArrayCL* flags,
    BuiltBuildings* buildings,
    u32 tech_count,
    u32 res_count,
    u32 flag_count,
    u32 building_count
) {
    for (u32 i = 0; i < tech_count; ++i) {
        techs->set_bit(i);
    }
    for (u32 i = 0; i < res_count; ++i) {
        resources->set_bit(i);
    }
    for (u32 i = 0; i < flag_count; ++i) {
        flags->set_bit(i);
    }
    for (u32 i = 0; i < building_count; ++i) {
        buildings->set_built(i);
    }
}

static void brute_force_collect () {
    g_unit_count = UnitData::get_unit_data_count();
    u32 tech_count = TechData::get_tech_data_count();
    u32 res_count = ResourceData::get_resource_data_count();
    u32 flag_count = CityFlagData::get_flag_count();
    u32 building_count = BuildingData::get_building_data_count();

    if (g_unit_count > MAX_UNITS) {
        std::printf("Too many units (%u), increase MAX_UNITS\n", g_unit_count);
        std::exit(1);
    }

    BitArrayCL techs(tech_count);
    BitArrayCL resources(res_count);
    BitArrayCL flags(flag_count);
    BuiltBuildings buildings;

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableUnits* baseline = UnitAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 t = 0; t < tech_count; ++t) {
            techs.clear_bit(t);
            BuildableUnits* cur = UnitAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 u = 0; u < g_unit_count; ++u) {
                if (baseline->can_build(u) && !cur->can_build(u)) {
                    add_tech_prereq(g_unit_results[u], (u16)t);
                }
            }
            delete cur;
            techs.set_bit(t);
        }
        delete baseline;
    }

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableUnits* baseline = UnitAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 r = 0; r < res_count; ++r) {
            resources.clear_bit(r);
            BuildableUnits* cur = UnitAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 u = 0; u < g_unit_count; ++u) {
                if (baseline->can_build(u) && !cur->can_build(u)) {
                    add_resource_prereq(g_unit_results[u], (u16)r);
                }
            }
            delete cur;
            resources.set_bit(r);
        }
        delete baseline;
    }

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableUnits* baseline = UnitAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 f = 0; f < flag_count; ++f) {
            flags.clear_bit(f);
            BuildableUnits* cur = UnitAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 u = 0; u < g_unit_count; ++u) {
                if (baseline->can_build(u) && !cur->can_build(u)) {
                    add_flag_prereq(g_unit_results[u], (u16)f);
                }
            }
            delete cur;
            flags.set_bit(f);
        }
        delete baseline;
    }

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableUnits* baseline = UnitAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 b = 0; b < building_count; ++b) {
            buildings.clear_built(b);
            BuildableUnits* cur = UnitAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 u = 0; u < g_unit_count; ++u) {
                if (baseline->can_build(u) && !cur->can_build(u)) {
                    add_building_prereq(g_unit_results[u], (u16)b);
                }
            }
            delete cur;
            buildings.set_built(b);
        }
        delete baseline;
    }
}

static void print_results () {
    const UnitTypeStats* unit_array = UnitData::get_unit_data_array();
    const TechTypeStats* tech_array = TechData::get_tech_data_array();
    const ResourceTypeStats* res_array = ResourceData::get_resource_data_array();
    const BuildingTypeStats* bld_array = BuildingData::get_building_data_array();
    const CityFlagStats* flag_array = CityFlagData::get_flag_data_array();

    for (u32 u = 0; u < g_unit_count; ++u) {
        const UnitResult& ur = g_unit_results[u];
        if (ur.tech_count == 0 &&
            ur.flag_count == 0 &&
            ur.resource_count == 0 &&
            ur.building_count == 0) {
            continue;
        }

        std::printf("Unit: %s\n", unit_array[u].name.c_str());

        if (ur.tech_count > 0) {
            std::printf("  Techs: ");
            for (u16 i = 0; i < ur.tech_count; ++i) {
                u16 idx = ur.tech_indices[i];
                std::printf("%s%s", tech_array[idx].name.c_str(), (i + 1 < ur.tech_count) ? ", " : "");
            }
            std::printf("\n");
        }

        if (ur.flag_count > 0) {
            std::printf("  Flags: ");
            for (u16 i = 0; i < ur.flag_count; ++i) {
                u16 idx = ur.flag_indices[i];
                std::printf("%s%s", flag_array[idx].name.c_str(), (i + 1 < ur.flag_count) ? ", " : "");
            }
            std::printf("\n");
        }

        if (ur.resource_count > 0) {
            std::printf("  Resources: ");
            for (u16 i = 0; i < ur.resource_count; ++i) {
                u16 idx = ur.resource_indices[i];
                std::printf("%s%s", res_array[idx].name.c_str(), (i + 1 < ur.resource_count) ? ", " : "");
            }
            std::printf("\n");
        }

        if (ur.building_count > 0) {
            std::printf("  Buildings: ");
            for (u16 i = 0; i < ur.building_count; ++i) {
                u16 idx = ur.building_indices[i];
                std::printf("%s%s", bld_array[idx].name.c_str(), (i + 1 < ur.building_count) ? ", " : "");
            }
            std::printf("\n");
        }

        std::printf("\n");
    }
}

static void validate_against_units_file () {
    const UnitTypeStats* unit_array = UnitData::get_unit_data_array();
    const TechTypeStats* tech_array = TechData::get_tech_data_array();
    const ResourceTypeStats* res_array = ResourceData::get_resource_data_array();
    const BuildingTypeStats* bld_array = BuildingData::get_building_data_array();
    const CityFlagStats* flag_array = CityFlagData::get_flag_data_array();

    std::vector<std::string> lines;
    std::ifstream in("../game_config.units");
    if (!in) {
        std::printf("Could not open ../game_config.units for validation\n");
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    for (u32 u = 0; u < g_unit_count; ++u) {
        const UnitResult&   ur    = g_unit_results[u];
        const std::string&  uname = unit_array[u].name;
        std::size_t line_idx = std::string::npos;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].find(uname) != std::string::npos) {
                line_idx = i;
                break;
            }
        }

        if (line_idx == std::string::npos) {
            note_result(false, ("No line found for unit " + uname).c_str());
            continue;
        }
        const std::string& uline = lines[line_idx];
        for (u16 i = 0; i < ur.tech_count; ++i) {
            u16 idx = ur.tech_indices[i];
            const std::string& tname = tech_array[idx].name;
            bool found = (uline.find(tname) != std::string::npos);
            note_result(found, ("Tech '" + tname + "' present for unit " + uname).c_str());
        }
        for (u16 i = 0; i < ur.flag_count; ++i) {
            u16 idx = ur.flag_indices[i];
            const std::string& fname = flag_array[idx].name;
            bool found = (uline.find(fname) != std::string::npos);
            note_result(found, ("Flag '" + fname + "' present for unit " + uname).c_str());
        }
        for (u16 i = 0; i < ur.resource_count; ++i) {
            u16 idx = ur.resource_indices[i];
            const std::string& rname = res_array[idx].name;
            bool found = (uline.find(rname) != std::string::npos);
            note_result(found, ("Resource '" + rname + "' present for unit " + uname).c_str());
        }
        for (u16 i = 0; i < ur.building_count; ++i) {
            u16 idx = ur.building_indices[i];
            const std::string& bname = bld_array[idx].name;
            bool found = (uline.find(bname) != std::string::npos);
            note_result(found, ("Building '" + bname + "' present for unit " + uname).c_str());
        }
    }
    summarize_test_results();
}


//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    CityFlagData::load_static_data("../game_config.city_flags");
    TechData::load_static_data("../game_config.techs");
    ResourceData::load_static_data("../game_config.resources");
    UnitData::load_static_data("../game_config.units");
    BuildingData::load_static_data("../game_config.buildings");

    brute_force_collect();
    if (print_level > 0) {
        print_results();
    }

    validate_against_units_file();

    std::printf("=======================================================\n");
    std::printf(" TESTING TRAINABLE ASSESSOR: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
