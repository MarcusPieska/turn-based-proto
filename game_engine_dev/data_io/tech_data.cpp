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

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static TechTypeStats* tech_data_array = nullptr;
static u32 tech_data_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================


void TechData::load_static_data (const std::string& filename) {
    if (tech_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void TechData::print_content () {
    printf("Tech Data (total=%u):\n", tech_data_count);
    for (u32 i = 0; i < tech_data_count; ++i) {
        const TechTypeStats& stats = tech_data_array[i];
        printf("Tech: %s, Cost: %u", stats.name.c_str(), stats.cost);
        bool first = true;
        for (u32 t = 0; t < MAX_TECHS_PER_ENTITY; ++t) {
            TechIdx idx = stats.tech_indices.indices[t];
            if (idx.get_idx() != 0) {
                if (first) {
                    printf(", Prereqs: ");
                    first = false;
                } else {
                    printf(" : ");
                }
                printf("%u", idx.get_idx());
            } else {
                break;
            }
        }
        printf("\n");
    }
}

TechIdx TechData::find_tech_index (const std::string& tech_name) {
    for (u32 i = 0; i < tech_data_count; ++i) {
        if (tech_data_array[i].name == tech_name) {
            return TechIdx(static_cast<u16>(i));
        }
    }
    printf("ERROR: Tech %s not found\n", tech_name.c_str());
    print_content();
    exit(1);
    return TechIdx(0); // To make the compiler happy
}

u16 TechData::get_tech_data_count () {
    return static_cast<u16>(tech_data_count);
}

const TechTypeStats* TechData::get_tech_data_array () {
    return tech_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

u16 TechData::validate_and_count (const std::string& filename) {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: No content found in %s\n", filename.c_str());
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
        if (parts.size() >= 2) {
            bool valid = true;
            for (size_t j = 0; j < 2; j++) {
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

void TechData::parse_and_allocate (const std::string& filename) {
    tech_data_count = TechData::validate_and_count(filename);
    if (tech_data_count == 0) {
        printf("ERROR: No tech data found in %s\n", filename.c_str());
        exit(1);
    }
    tech_data_array = new TechTypeStats[tech_data_count];
    if (tech_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u tech data\n", tech_data_count);
        exit(1);
    }
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: No content found in %s\n", filename.c_str());
        exit(1);
    }

    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < tech_data_count; i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 2) {
            std::string name = trimmer.trim(parts[0]);
            std::string cost_str = trimmer.trim(parts[1]);
            if (!name.empty() && !cost_str.empty()) {
                tech_data_array[idx].name = name;
                tech_data_array[idx].cost = static_cast<u32>(std::atoi(cost_str.c_str()));
                for (u32 t = 0; t < MAX_TECHS_PER_ENTITY; ++t) {
                    tech_data_array[idx].tech_indices.indices[t] = 0;
                }

                idx++;
            }
        }
    }

    idx = 0;
    for (size_t i = 0; i < lines.size() && idx < tech_data_count; i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 2) {
            std::string name = trimmer.trim(parts[0]);
            std::string cost_str = trimmer.trim(parts[1]);
            if (!name.empty() && !cost_str.empty()) {
                u32 prereq_count = 0;
                for (size_t j = 2; j < parts.size() && prereq_count < MAX_TECHS_PER_ENTITY; j++) {
                    std::string prereq_name = trimmer.trim(parts[j]);
                    if (!prereq_name.empty()) {
                        TechIdx prereq_idx = TechData::find_tech_index(prereq_name);
                        tech_data_array[idx].tech_indices.indices[prereq_count++] = prereq_idx;
                    }
                }

                idx++;
            }
        }
    }
    if (idx != tech_data_count) {
        printf("ERROR: Expected %u tech data, got %u\n", tech_data_count, idx);
        exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
