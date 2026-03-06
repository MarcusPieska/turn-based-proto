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
#include "tech_data.h"

#include "resource_data.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static ResourceTypeStats* resource_data_array = nullptr;
static u32 resource_data_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================


void ResourceData::load_static_data (const std::string& filename) {
    if (resource_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void ResourceData::print_content () {
    if (resource_data_array == nullptr || resource_data_count == 0) {
        printf("Resource data not loaded.\n");
        return;
    }

    for (u32 i = 0; i < resource_data_count; ++i) {
        const ResourceTypeStats& stats = resource_data_array[i];
        printf("Resource: %s, Food: %u, Shields: %u, Commerce: %u, Tech prerequisite idx: %u\n",
               stats.name.c_str(),
               static_cast<u32>(stats.bonus_food),
               static_cast<u32>(stats.bonus_shields),
               static_cast<u32>(stats.bonus_commerce),
               static_cast<u32>(stats.tech_prereq_index));
    }
}

u16 ResourceData::find_resource_index (const std::string& resource_name) {
    if (resource_data_array == nullptr || resource_data_count == 0) {
        printf("ERROR: ResourceData not loaded when searching for resource '%s'\n", resource_name.c_str());
        exit(1);
    }
    
    for (u32 i = 0; i < resource_data_count; ++i) {
        if (resource_data_array[i].name == resource_name) {
            return static_cast<u16>(i);
        }
    }
    
    printf("ERROR: Resource '%s' not in ResourceData\n", resource_name.c_str());
    exit(1);
    return 0; // To make the compiler happy
}

u16 ResourceData::get_resource_data_count () {
    return static_cast<u16>(resource_data_count);
}

const ResourceTypeStats* ResourceData::get_resource_data_array () {
    return resource_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

u16 ResourceData::validate_and_count (const std::string& filename) {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: Resource data file '%s' is empty\n", filename.c_str());
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
        if (parts.size() >= 4) {
            bool valid = true;
            for (size_t j = 0; j < 4; j++) {
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

void ResourceData::parse_and_allocate (const std::string& filename) {
    int count = ResourceData::validate_and_count(filename);
    if (count == 0) {
        printf("ERROR: No resources found in data file '%s'\n", filename.c_str());
        exit(1);
    }
    resource_data_count = static_cast<u32>(count);
    resource_data_array = new ResourceTypeStats[resource_data_count];
    if (resource_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u resources\n", static_cast<u32>(count));
        exit(1);
    }
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: Resource data file '%s' is empty\n", filename.c_str());
        exit(1);
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");

    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < resource_data_count; i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 4) {
            std::string name = trimmer.trim(parts[0]);
            std::string food_str = trimmer.trim(parts[1]);
            std::string shields_str = trimmer.trim(parts[2]);
            std::string comm_str = trimmer.trim(parts[3]);
            std::string prereq_str;
            if (parts.size() >= 5) {
                prereq_str = trimmer.trim(parts[4]);
            }

            if (!name.empty() && !food_str.empty() && !shields_str.empty() && !comm_str.empty()) {
                resource_data_array[idx].name = name;
                resource_data_array[idx].bonus_food = static_cast<u16>(std::atoi(food_str.c_str()));
                resource_data_array[idx].bonus_shields = static_cast<u16>(std::atoi(shields_str.c_str()));
                resource_data_array[idx].bonus_commerce = static_cast<u16>(std::atoi(comm_str.c_str()));
                if (!prereq_str.empty()) {
                    resource_data_array[idx].tech_prereq_index = TechData::find_tech_index(prereq_str);
                } else {
                    resource_data_array[idx].tech_prereq_index = 0;
                }

                idx++;
            }
        }
    }

    if (idx != resource_data_count) {
        printf("ERROR: Did not parse %u resources from data file '%s'\n", resource_data_count, filename.c_str());
        exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
