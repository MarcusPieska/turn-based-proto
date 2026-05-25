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
#include "city_flags.h"
#include "tech_data.h"
#include "resource_data.h"
#include "building_data.h"
#include "building_vector.h"
#include "building_assessor.h"

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
const u32 MAX_BUILDING_TYPES = 250;

struct BuildingResult {
    u16 tech_indices[MAX_TECHS];
    u16 flag_indices[MAX_FLAGS];
    u16 resource_indices[MAX_RESOURCES];
    u16 building_indices[MAX_BUILDINGS];
    u16 tech_count;
    u16 flag_count;
    u16 resource_count;
    u16 building_count;

    BuildingResult() {
        tech_count = 0;
        flag_count = 0;
        resource_count = 0;
        building_count = 0;
    }
};

static BuildingResult g_building_results[MAX_BUILDING_TYPES];
static u32 g_building_count = 0;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

void note_result(bool cond, cstr msg) {
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

void note_result(bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_result(cond, msg.c_str());
}

void summarize_test_results() {
    if (print_level > 0) {
        std::printf("--------------------------------\n");
        std::printf(" Test count: %d\n", test_count);
        std::printf(" Test pass:  %d\n", test_pass);
        std::printf(" Test fail:  %d\n", test_count - test_pass);
        std::printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

static void add_tech_prereq(BuildingResult& br, u16 tech_idx) {
    for (u16 i = 0; i < br.tech_count; ++i) {
        if (br.tech_indices[i] == tech_idx) {
            return;
        }
    }
    if (br.tech_count < MAX_TECHS) {
        br.tech_indices[br.tech_count++] = tech_idx;
    }
}

static void add_flag_prereq(BuildingResult& br, u16 flag_idx) {
    for (u16 i = 0; i < br.flag_count; ++i) {
        if (br.flag_indices[i] == flag_idx) {
            return;
        }
    }
    if (br.flag_count < MAX_FLAGS) {
        br.flag_indices[br.flag_count++] = flag_idx;
    }
}

static void add_resource_prereq(BuildingResult& br, u16 res_idx) {
    for (u16 i = 0; i < br.resource_count; ++i) {
        if (br.resource_indices[i] == res_idx) {
            return;
        }
    }
    if (br.resource_count < MAX_RESOURCES) {
        br.resource_indices[br.resource_count++] = res_idx;
    }
}

static void add_building_prereq(BuildingResult& br, u16 bld_idx) {
    for (u16 i = 0; i < br.building_count; ++i) {
        if (br.building_indices[i] == bld_idx) {
            return;
        }
    }
    if (br.building_count < MAX_BUILDINGS) {
        br.building_indices[br.building_count++] = bld_idx;
    }
}

static void set_all_bits(
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
}

//================================================================================================================================
//=> - Brute force prerequisite collection -
//================================================================================================================================

static void brute_force_collect() {
    g_building_count = BuildingData::get_building_data_count();
    u32 tech_count = TechData::get_tech_data_count();
    u32 res_count = ResourceData::get_resource_data_count();
    u32 flag_count = CityFlagData::get_flag_count();
    u32 building_count = BuildingData::get_building_data_count();

    if (g_building_count > MAX_BUILDING_TYPES) {
        std::printf("Too many buildings (%u), increase MAX_BUILDING_TYPES\n", g_building_count);
        std::exit(1);
    }

    BitArrayCL techs(tech_count);
    BitArrayCL resources(res_count);
    BitArrayCL flags(flag_count);
    BuiltBuildings buildings;

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableBuildings* baseline = BuildingAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 t = 0; t < tech_count; ++t) {
            techs.clear_bit(t);
            BuildableBuildings* cur = BuildingAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 b = 0; b < g_building_count; ++b) {
                if (baseline->can_build(b) && !cur->can_build(b)) {
                    add_tech_prereq(g_building_results[b], (u16)t);
                }
            }
            delete cur;
            techs.set_bit(t);
        }
        delete baseline;
    }

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableBuildings* baseline = BuildingAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 r = 0; r < res_count; ++r) {
            resources.clear_bit(r);
            BuildableBuildings* cur = BuildingAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 b = 0; b < g_building_count; ++b) {
                if (baseline->can_build(b) && !cur->can_build(b)) {
                    add_resource_prereq(g_building_results[b], (u16)r);
                }
            }
            delete cur;
            resources.set_bit(r);
        }
        delete baseline;
    }

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableBuildings* baseline = BuildingAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 f = 0; f < flag_count; ++f) {
            flags.clear_bit(f);
            BuildableBuildings* cur = BuildingAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 b = 0; b < g_building_count; ++b) {
                if (baseline->can_build(b) && !cur->can_build(b)) {
                    add_flag_prereq(g_building_results[b], (u16)f);
                }
            }
            delete cur;
            flags.set_bit(f);
        }
        delete baseline;
    }

    {
        for (u32 b = 0; b < g_building_count; ++b) {
            set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
            for (u32 i = 0; i < building_count; ++i) {
                buildings.set_built((u16)i);
            }
            buildings.clear_built((u16)b);
            BuildableBuildings* baseline = BuildingAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 bb = 0; bb < building_count; ++bb) {
                if (bb == b) {
                    continue;
                }
                buildings.clear_built((u16)bb);
                BuildableBuildings* cur = BuildingAssessor::assess(&techs, &buildings, &resources, &flags);
                if (baseline->can_build(b) && !cur->can_build(b)) {
                    add_building_prereq(g_building_results[b], (u16)bb);
                }
                delete cur;
                buildings.set_built((u16)bb);
            }
            delete baseline;
        }
    }
}

//================================================================================================================================
//=> - Debug print of collected prerequisites -
//================================================================================================================================

static void print_results() {
    const BuildingTypeStats* building_array = BuildingData::get_building_data_array();
    const TechTypeStats* tech_array = TechData::get_tech_data_array();
    const ResourceTypeStats* res_array = ResourceData::get_resource_data_array();
    const CityFlagStats* flag_array = CityFlagData::get_flag_data_array();

    for (u32 b = 0; b < g_building_count; ++b) {
        const BuildingResult& br = g_building_results[b];
        if (br.tech_count == 0 && br.flag_count == 0 && br.resource_count == 0 && br.building_count == 0) {
            continue;
        }

        std::printf("Building: %s\n", building_array[b].name.c_str());

        if (br.tech_count > 0) {
            std::printf("  Techs: ");
            for (u16 i = 0; i < br.tech_count; ++i) {
                u16 idx = br.tech_indices[i];
                std::printf("%s%s", tech_array[idx].name.c_str(), (i + 1 < br.tech_count) ? ", " : "");
            }
            std::printf("\n");
        }

        if (br.flag_count > 0) {
            std::printf("  Flags: ");
            for (u16 i = 0; i < br.flag_count; ++i) {
                u16 idx = br.flag_indices[i];
                std::printf("%s%s", flag_array[idx].name.c_str(), (i + 1 < br.flag_count) ? ", " : "");
            }
            std::printf("\n");
        }

        if (br.resource_count > 0) {
            std::printf("  Resources: ");
            for (u16 i = 0; i < br.resource_count; ++i) {
                u16 idx = br.resource_indices[i];
                std::printf("%s%s", res_array[idx].name.c_str(), (i + 1 < br.resource_count) ? ", " : "");
            }
            std::printf("\n");
        }

        if (br.building_count > 0) {
            std::printf("  Buildings: ");
            for (u16 i = 0; i < br.building_count; ++i) {
                u16 idx = br.building_indices[i];
                std::printf("%s%s", building_array[idx].name.c_str(), (i + 1 < br.building_count) ? ", " : "");
            }
            std::printf("\n");
        }

        std::printf("\n");
    }
}

//================================================================================================================================
//=> - Validation against game_config.buildings -
//================================================================================================================================

static void validate_against_buildings_file() {
    const BuildingTypeStats* building_array = BuildingData::get_building_data_array();
    const TechTypeStats* tech_array = TechData::get_tech_data_array();
    const ResourceTypeStats* res_array = ResourceData::get_resource_data_array();
    const CityFlagStats* flag_array = CityFlagData::get_flag_data_array();

    std::vector<std::string> lines;
    std::ifstream in("../game_config.buildings");
    if (!in) {
        std::printf("Could not open ../game_config.buildings for validation\n");
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    for (u32 b = 0; b < g_building_count; ++b) {
        const BuildingResult& br = g_building_results[b];
        const std::string& bname = building_array[b].name;
        std::size_t line_idx = std::string::npos;

        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].find(bname) != std::string::npos) {
                line_idx = i;
                break;
            }
        }

        if (line_idx == std::string::npos) {
            note_result(false, ("No line found for building " + bname).c_str());
            continue;
        }

        const std::string& bline = lines[line_idx];

        for (u16 i = 0; i < br.tech_count; ++i) {
            u16 idx = br.tech_indices[i];
            const std::string& tname = tech_array[idx].name;
            bool found = (bline.find(tname) != std::string::npos);
            note_result(found, ("Tech '" + tname + "' present for building " + bname).c_str());
        }
        for (u16 i = 0; i < br.flag_count; ++i) {
            u16 idx = br.flag_indices[i];
            const std::string& fname = flag_array[idx].name;
            bool found = (bline.find(fname) != std::string::npos);
            note_result(found, ("Flag '" + fname + "' present for building " + bname).c_str());
        }
        for (u16 i = 0; i < br.resource_count; ++i) {
            u16 idx = br.resource_indices[i];
            const std::string& rname = res_array[idx].name;
            bool found = (bline.find(rname) != std::string::npos);
            note_result(found, ("Resource '" + rname + "' present for building " + bname).c_str());
        }
        for (u16 i = 0; i < br.building_count; ++i) {
            u16 idx = br.building_indices[i];
            const std::string& depname = building_array[idx].name;
            bool found = (bline.find(depname) != std::string::npos);
            note_result(found, ("Building '" + depname + "' present for building " + bname).c_str());
        }
    }

    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main(int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    CityFlagData::load_static_data("../game_config.city_flags");
    TechData::load_static_data("../game_config.techs");
    ResourceData::load_static_data("../game_config.resources");
    BuildingData::load_static_data("../game_config.buildings");

    brute_force_collect();
    if (print_level > 0) {
        print_results();
    }

    validate_against_buildings_file();

    std::printf("============================================================================\n");
    std::printf(" TESTING BUILDING ASSESSOR (BRUTE REV): TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("============================================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

