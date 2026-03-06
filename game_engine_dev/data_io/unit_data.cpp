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

#include "unit_data.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static UnitTypeStats* unit_data_array = nullptr;
static u32 unit_data_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================

void UnitData::load_static_data (const std::string& filename) {
    if (unit_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void UnitData::print_content () {
    if (unit_data_array == nullptr || unit_data_count == 0) {
        printf("Unit data not loaded.\n");
        return;
    }

    for (u32 i = 0; i < unit_data_count; ++i) {
        const UnitTypeStats& stats = unit_data_array[i];
        printf("Unit: %s, Cost: %u, A/D/M: %u/%u/%u, Tech prerequisite idx: %u",
               stats.name.c_str(),
               stats.cost,
               static_cast<unsigned>(stats.attack),
               static_cast<unsigned>(stats.defense),
               static_cast<unsigned>(stats.movement_speed),
               static_cast<unsigned>(stats.tech_prereq_index));

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

u16 UnitData::find_unit_index (const std::string& unit_name) {
    for (u32 i = 0; i < unit_data_count; ++i) {
        if (unit_data_array[i].name == unit_name) {
            return static_cast<u16>(i);
        }
    }
    return 0; // Return 0 if not found (default)
}

u16 UnitData::get_unit_data_count () {
    return unit_data_count;
}

const UnitTypeStats* UnitData::get_unit_data_array () {
    return unit_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

u16 UnitData::validate_and_count (const std::string& filename) {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        return 0;
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
        if (parts.size() >= 6) {
            bool valid = true;
            for (size_t j = 0; j < 6; j++) {
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

void UnitData::parse_and_allocate (const std::string& filename) {
    if (unit_data_array != nullptr) {
        return;
    }

    int count = UnitData::validate_and_count(filename);
    if (count == 0) {
        return;
    }

    unit_data_count = static_cast<u32>(count);
    unit_data_array = new UnitTypeStats[unit_data_count];

    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        return;
    }

    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");

    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < unit_data_count; i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 6) {
            std::string name = trimmer.trim(parts[0]);
            std::string cost_str = trimmer.trim(parts[1]);
            std::string atk_str = trimmer.trim(parts[2]);
            std::string def_str = trimmer.trim(parts[3]);
            std::string move_str = trimmer.trim(parts[4]);
            std::string tech_prereq = trimmer.trim(parts[5]);

            if (!name.empty() && !cost_str.empty() &&
                !atk_str.empty() && !def_str.empty() &&
                !move_str.empty() && !tech_prereq.empty()) {

                unit_data_array[idx].name = name;
                unit_data_array[idx].cost = static_cast<u32>(std::atoi(cost_str.c_str()));
                unit_data_array[idx].attack = static_cast<u16>(std::atoi(atk_str.c_str()));
                unit_data_array[idx].defense = static_cast<u16>(std::atoi(def_str.c_str()));
                unit_data_array[idx].movement_speed = static_cast<u16>(std::atoi(move_str.c_str()));
                unit_data_array[idx].tech_prereq_index = TechData::find_tech_index(tech_prereq);

                for (u32 r = 0; r < MAX_RESOURCES_PER_ENTITY; ++r) {
                    unit_data_array[idx].resource_indices.indices[r] = 0;
                }
                u32 resource_count = 0;
                for (size_t j = 6; j < parts.size() && resource_count < MAX_RESOURCES_PER_ENTITY; j++) {
                    std::string resource = trimmer.trim(parts[j]);
                    if (!resource.empty()) {
                        i16 resource_index = ResourceData::find_resource_index(resource);
                        unit_data_array[idx].resource_indices.indices[resource_count++] =
                            static_cast<u16>(resource_index);
                    }
                }

                idx++;
            }
        }
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
