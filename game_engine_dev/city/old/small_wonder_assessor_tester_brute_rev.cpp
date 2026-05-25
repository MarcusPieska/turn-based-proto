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
//=> - Result struct and globals -
//================================================================================================================================

const u16 MAX_TECHS = 1;
const u16 MAX_FLAGS = 4;
const u16 MAX_RESOURCES = 4;
const u16 MAX_BUILDINGS = 4;
const u32 MAX_WONDERS = 250;

struct WonderResult {
    u16 tech_indices[MAX_TECHS];
    u16 flag_indices[MAX_FLAGS];
    u16 resource_indices[MAX_RESOURCES];
    u16 building_indices[MAX_BUILDINGS];
    u16 tech_count;
    u16 flag_count;
    u16 resource_count;
    u16 building_count;

    WonderResult() {
        tech_count = 0;
        flag_count = 0;
        resource_count = 0;
        building_count = 0;
    }
};

static WonderResult g_wonder_results[MAX_WONDERS];
static u32 g_wonder_count = 0;

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

static void add_tech_prereq(WonderResult& wr, u16 tech_idx) {
    for (u16 i = 0; i < wr.tech_count; ++i) {
        if (wr.tech_indices[i] == tech_idx) {
            return;
        }
    }
    if (wr.tech_count < MAX_TECHS) {
        wr.tech_indices[wr.tech_count++] = tech_idx;
    }
}

static void add_flag_prereq(WonderResult& wr, u16 flag_idx) {
    for (u16 i = 0; i < wr.flag_count; ++i) {
        if (wr.flag_indices[i] == flag_idx) {
            return;
        }
    }
    if (wr.flag_count < MAX_FLAGS) {
        wr.flag_indices[wr.flag_count++] = flag_idx;
    }
}

static void add_resource_prereq(WonderResult& wr, u16 res_idx) {
    for (u16 i = 0; i < wr.resource_count; ++i) {
        if (wr.resource_indices[i] == res_idx) {
            return;
        }
    }
    if (wr.resource_count < MAX_RESOURCES) {
        wr.resource_indices[wr.resource_count++] = res_idx;
    }
}

static void add_building_prereq(WonderResult& wr, u16 bld_idx) {
    for (u16 i = 0; i < wr.building_count; ++i) {
        if (wr.building_indices[i] == bld_idx) {
            return;
        }
    }
    if (wr.building_count < MAX_BUILDINGS) {
        wr.building_indices[wr.building_count++] = bld_idx;
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
    for (u32 i = 0; i < building_count; ++i) {
        buildings->set_built(i);
    }
}

//================================================================================================================================
//=> - Brute force prerequisite collection -
//================================================================================================================================

static void brute_force_collect() {
    g_wonder_count = WonderData::get_wonder_data_count();
    u32 tech_count = TechData::get_tech_data_count();
    u32 res_count = ResourceData::get_resource_data_count();
    u32 flag_count = CityFlagData::get_flag_count();
    u32 building_count = BuildingData::get_building_data_count();

    if (g_wonder_count > MAX_WONDERS) {
        std::printf("Too many wonders (%u), increase MAX_WONDERS\n", g_wonder_count);
        std::exit(1);
    }

    BitArrayCL techs(tech_count);
    BitArrayCL resources(res_count);
    BitArrayCL flags(flag_count);
    BuiltBuildings buildings;

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableWonders* baseline = WonderAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 t = 0; t < tech_count; ++t) {
            techs.clear_bit(t);
            BuildableWonders* cur = WonderAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 w = 0; w < g_wonder_count; ++w) {
                if (baseline->can_build((u16)w) && !cur->can_build((u16)w)) {
                    add_tech_prereq(g_wonder_results[w], (u16)t);
                }
            }
            delete cur;
            techs.set_bit(t);
        }
        delete baseline;
    }

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableWonders* baseline = WonderAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 r = 0; r < res_count; ++r) {
            resources.clear_bit(r);
            BuildableWonders* cur = WonderAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 w = 0; w < g_wonder_count; ++w) {
                if (baseline->can_build((u16)w) && !cur->can_build((u16)w)) {
                    add_resource_prereq(g_wonder_results[w], (u16)r);
                }
            }
            delete cur;
            resources.set_bit(r);
        }
        delete baseline;
    }

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableWonders* baseline = WonderAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 f = 0; f < flag_count; ++f) {
            flags.clear_bit(f);
            BuildableWonders* cur = WonderAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 w = 0; w < g_wonder_count; ++w) {
                if (baseline->can_build((u16)w) && !cur->can_build((u16)w)) {
                    add_flag_prereq(g_wonder_results[w], (u16)f);
                }
            }
            delete cur;
            flags.set_bit(f);
        }
        delete baseline;
    }

    {
        set_all_bits(&techs, &resources, &flags, &buildings, tech_count, res_count, flag_count, building_count);
        BuildableWonders* baseline = WonderAssessor::assess(&techs, &buildings, &resources, &flags);
        for (u32 b = 0; b < building_count; ++b) {
            buildings.clear_built((u16)b);
            BuildableWonders* cur = WonderAssessor::assess(&techs, &buildings, &resources, &flags);
            for (u32 w = 0; w < g_wonder_count; ++w) {
                if (baseline->can_build((u16)w) && !cur->can_build((u16)w)) {
                    add_building_prereq(g_wonder_results[w], (u16)b);
                }
            }
            delete cur;
            buildings.set_built((u16)b);
        }
        delete baseline;
    }
}

//================================================================================================================================
//=> - Debug print of collected prerequisites -
//================================================================================================================================

static void print_results() {
    const WonderTypeStats* wonder_array = WonderData::get_wonder_data_array();
    const TechTypeStats* tech_array = TechData::get_tech_data_array();
    const ResourceTypeStats* res_array = ResourceData::get_resource_data_array();
    const BuildingTypeStats* bld_array = BuildingData::get_building_data_array();
    const CityFlagStats* flag_array = CityFlagData::get_flag_data_array();

    for (u32 w = 0; w < g_wonder_count; ++w) {
        const WonderResult& wr = g_wonder_results[w];
        if (wr.tech_count == 0 && wr.flag_count == 0 && wr.resource_count == 0 && wr.building_count == 0) {
            continue;
        }

        std::printf("Wonder: %s\n", wonder_array[w].name.c_str());

        if (wr.tech_count > 0) {
            std::printf("  Techs: ");
            for (u16 i = 0; i < wr.tech_count; ++i) {
                u16 idx = wr.tech_indices[i];
                std::printf("%s%s", tech_array[idx].name.c_str(), (i + 1 < wr.tech_count) ? ", " : "");
            }
            std::printf("\n");
        }

        if (wr.flag_count > 0) {
            std::printf("  Flags: ");
            for (u16 i = 0; i < wr.flag_count; ++i) {
                u16 idx = wr.flag_indices[i];
                std::printf("%s%s", flag_array[idx].name.c_str(), (i + 1 < wr.flag_count) ? ", " : "");
            }
            std::printf("\n");
        }

        if (wr.resource_count > 0) {
            std::printf("  Resources: ");
            for (u16 i = 0; i < wr.resource_count; ++i) {
                u16 idx = wr.resource_indices[i];
                std::printf("%s%s", res_array[idx].name.c_str(), (i + 1 < wr.resource_count) ? ", " : "");
            }
            std::printf("\n");
        }

        if (wr.building_count > 0) {
            std::printf("  Buildings: ");
            for (u16 i = 0; i < wr.building_count; ++i) {
                u16 idx = wr.building_indices[i];
                std::printf("%s%s", bld_array[idx].name.c_str(), (i + 1 < wr.building_count) ? ", " : "");
            }
            std::printf("\n");
        }

        std::printf("\n");
    }
}

//================================================================================================================================
//=> - Validation against game_config.wonders_small -
//================================================================================================================================

static void validate_against_wonders_file() {
    const WonderTypeStats* wonder_array = WonderData::get_wonder_data_array();
    const TechTypeStats* tech_array = TechData::get_tech_data_array();
    const ResourceTypeStats* res_array = ResourceData::get_resource_data_array();
    const BuildingTypeStats* bld_array = BuildingData::get_building_data_array();
    const CityFlagStats* flag_array = CityFlagData::get_flag_data_array();

    std::vector<std::string> lines;
    std::ifstream in("../game_config.wonders_small");
    if (!in) {
        std::printf("Could not open ../game_config.wonders_small for validation\n");
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    for (u32 w = 0; w < g_wonder_count; ++w) {
        const WonderResult& wr = g_wonder_results[w];
        const std::string& wname = wonder_array[w].name;
        std::size_t line_idx = std::string::npos;

        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].find(wname) != std::string::npos) {
                line_idx = i;
                break;
            }
        }

        if (line_idx == std::string::npos) {
            note_result(false, ("No line found for wonder " + wname).c_str());
            continue;
        }

        const std::string& wline = lines[line_idx];

        for (u16 i = 0; i < wr.tech_count; ++i) {
            u16 idx = wr.tech_indices[i];
            const std::string& tname = tech_array[idx].name;
            bool found = (wline.find(tname) != std::string::npos);
            note_result(found, ("Tech '" + tname + "' present for wonder " + wname).c_str());
        }
        for (u16 i = 0; i < wr.flag_count; ++i) {
            u16 idx = wr.flag_indices[i];
            const std::string& fname = flag_array[idx].name;
            bool found = (wline.find(fname) != std::string::npos);
            note_result(found, ("Flag '" + fname + "' present for wonder " + wname).c_str());
        }
        for (u16 i = 0; i < wr.resource_count; ++i) {
            u16 idx = wr.resource_indices[i];
            const std::string& rname = res_array[idx].name;
            bool found = (wline.find(rname) != std::string::npos);
            note_result(found, ("Resource '" + rname + "' present for wonder " + wname).c_str());
        }
        for (u16 i = 0; i < wr.building_count; ++i) {
            u16 idx = wr.building_indices[i];
            const std::string& bname = bld_array[idx].name;
            bool found = (wline.find(bname) != std::string::npos);
            note_result(found, ("Building '" + bname + "' present for wonder " + wname).c_str());
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
    WonderData::load_static_data("../game_config.wonders_small");
    BuiltWonders::allocate_static_array();

    brute_force_collect();
    if (print_level > 0) {
        print_results();
    }

    validate_against_wonders_file();

    std::printf("============================================================================\n");
    std::printf(" TESTING SMALL WONDER ASSESSOR (BRUTE REV): TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("============================================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================