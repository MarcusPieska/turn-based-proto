//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "str_mng.h"
#include "bit_array.h"
#include "building_data.h"

#include "civ_trait_data.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static CivTraitStats* civ_trait_data_array = nullptr;
static u16 civ_trait_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================

void CivTraitData::load_static_data (const std::string& filename) {
    if (civ_trait_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void CivTraitData::print_content () {
    printf("Civ trait data count: (total=%u)\n", civ_trait_count);
    for (u16 i = 0; i < civ_trait_count; ++i) {
        printf("CivTrait[%u]: %s\n", i, civ_trait_data_array[i].name.c_str());
    }
}

u16 CivTraitData::find_civ_trait_index (const std::string& trait_name) {
    for (u16 i = 0; i < civ_trait_count; ++i) {
        if (civ_trait_data_array[i].name == trait_name) {
            return i;
        }
    }
    printf("ERROR: Civ trait %s not found\n", trait_name.c_str());
    print_content();
    exit(1);
    return 0; // To make the compiler happy
}

u16 CivTraitData::get_civ_trait_count () {
    return civ_trait_count;
}

const CivTraitStats* CivTraitData::get_civ_trait_data_array () {
    return civ_trait_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

void CivTraitData::parse_and_allocate (const std::string& filename) {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: No content found in file '%s'\n", filename.c_str());
        exit(1);
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");

    civ_trait_count = 1;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (!line.empty()) {
            std::vector<std::string> parts = colon_splitter.split(line);
            if (parts.size() >= 1) {
                std::string trait_name = trimmer.trim(parts[0]);
                if (!trait_name.empty()) {
                    ++civ_trait_count;
                }
            }
        }
    }
    if (civ_trait_count == 1) {
        printf("ERROR: No civ traits found in data file '%s'\n", filename.c_str());
        exit(1);
    }
    u16 building_count = BuildingData::get_building_data_count();
    civ_trait_data_array = static_cast<CivTraitStats*>(::operator new(sizeof(CivTraitStats) * civ_trait_count));
    if (civ_trait_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u civ traits\n", civ_trait_count);
        exit(1);
    }
    new (&civ_trait_data_array[0].name) std::string("CIV_TRAIT_NONE");
    new (&civ_trait_data_array[0].buildings) BitArrayCL(building_count);
    u32 idx = 1;
    for (size_t i = 0; i < lines.size() && idx < civ_trait_count; ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 1) {
            std::string trait_name = trimmer.trim(parts[0]);
            if (trait_name.empty()) {
                continue;
            }
            new (&civ_trait_data_array[idx].name) std::string(trait_name);
            new (&civ_trait_data_array[idx].buildings) BitArrayCL(building_count);
            for (size_t j = 1; j < parts.size(); ++j) {
                std::string building_name = trimmer.trim(parts[j]);
                if (!building_name.empty()) {
                    u16 building_idx = BuildingData::find_building_index(building_name);
                    civ_trait_data_array[idx].buildings.set_bit(building_idx);
                }
            }
            idx++;
        }
    }
    if (idx != civ_trait_count) {
        printf("ERROR: Did not parse %u civ traits from data file '%s'\n", civ_trait_count, filename.c_str());
        exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
