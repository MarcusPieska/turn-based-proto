//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "str_mng.h"
#include "bit_array.h"
#include "resource_data.h"
#include "tech_data.h"

#include "building_data.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static BuildingTypeStats* building_data_array = nullptr;
static u32 building_data_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================


void BuildingData::load_static_data (const std::string& filename) {
    if (building_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void BuildingData::print_content () {
    if (building_data_array == nullptr || building_data_count == 0) {
        printf("Building data not loaded.\n");
        return;
    }
    for (u32 i = 0; i < building_data_count; ++i) {
        const BuildingTypeStats& stats = building_data_array[i];
        printf("Building: %s, Cost: %u, Tech prerequisite idx: %u", stats.name.c_str(), stats.cost, stats.tech_prereq_index);
        bool first_res = true;
        for (u32 r = 0; r < MAX_RESOURCES_PER_ENTITY; ++r) {
            u16 res_idx = stats.resource_indices.indices[r];
            if (res_idx != 0) {
                if (first_res) {
                    printf(", Resources: ");
                    first_res = false;
                } else {
                    printf(" : ");
                }
                printf("%u", res_idx);
            }
        }

        printf("\n");
    }
}

u16 BuildingData::find_building_index (const std::string& building_name) {
    if (building_data_array == nullptr || building_data_count == 0) {
        printf("ERROR: BuildingData not loaded when searching for building '%s'\n", building_name.c_str());
        exit(1);
    }
    
    for (u32 i = 0; i < building_data_count; ++i) {
        if (building_data_array[i].name == building_name) {
            return static_cast<u16>(i);
        }
    }
    
    printf("ERROR: Building '%s' not found in BuildingData\n", building_name.c_str());
    exit(1);
    return 0; // To make the compiler happy
}

u16 BuildingData::get_building_data_count () {
    return building_data_count;
}

const BuildingTypeStats* BuildingData::get_building_data_array () {
    return building_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

u16 BuildingData::validate_and_count (const std::string& filename) {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: Building data file '%s' is empty\n", filename.c_str());
        exit(1);
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    u16 count = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 3) {
            bool valid = true;
            for (size_t j = 0; j < 3; j++) {
                std::string part = trimmer.trim(parts[j]);
                if (part.empty()) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                count++;
            }
        }
    }
    return count;
}

void BuildingData::parse_and_allocate (const std::string& filename) {
    if (building_data_array != nullptr) {
        return;
    }

    int count = BuildingData::validate_and_count(filename);
    if (count == 0) {
        printf("ERROR: No buildings found in data file '%s'\n", filename.c_str());
        exit(1);
    }
    building_data_count = static_cast<u32>(count);
    building_data_array = new BuildingTypeStats[building_data_count];
    if (building_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u buildings\n", static_cast<u32>(count));
        exit(1);
    }
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: Building data file '%s' is empty\n", filename.c_str());
        exit(1);
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < building_data_count; i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 3) {
            std::string name = trimmer.trim(parts[0]);
            std::string cost_str = trimmer.trim(parts[1]);
            std::string tech_prereq = trimmer.trim(parts[2]);
            if (!name.empty() && !cost_str.empty() && !tech_prereq.empty()) {
                building_data_array[idx].name = name;
                building_data_array[idx].cost = static_cast<u32>(std::atoi(cost_str.c_str()));
                building_data_array[idx].tech_prereq_index = TechData::find_tech_index(tech_prereq);
                
                building_data_array[idx].resource_indices.indices[0] = 0;
                building_data_array[idx].resource_indices.indices[1] = 0;
                building_data_array[idx].resource_indices.indices[2] = 0;
                building_data_array[idx].resource_indices.indices[3] = 0;
                building_data_array[idx].effect_indices.indices[0] = 0;
                building_data_array[idx].effect_indices.indices[1] = 0;
                building_data_array[idx].effect_indices.indices[2] = 0;
                building_data_array[idx].effect_indices.indices[3] = 0;
                
                u32 resource_count = 0;
                for (size_t j = 3; j < parts.size() && resource_count < MAX_RESOURCES_PER_ENTITY; j++) {
                    std::string resource = trimmer.trim(parts[j]);
                    if (!resource.empty()) {
                        i16 resource_index = ResourceData::find_resource_index(resource);
                        building_data_array[idx].resource_indices.indices[resource_count++] = resource_index;
                    }
                }
                
                idx++;
            }
        }
    }
    
    if (idx != building_data_count) {
        printf("ERROR: Did not parse %u buildings from data file '%s'\n", building_data_count, filename.c_str());
        exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
