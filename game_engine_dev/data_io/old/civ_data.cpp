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
#include "civ_trait_data.h"

#include "civ_data.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static CivStats* civ_data_array = nullptr;
static u16 civ_data_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================

void CivData::load_static_data (const std::string& filename) {
    if (civ_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void CivData::print_content () {
    printf("Civ data count: (total=%u)\n", civ_data_count);
    for (u16 i = 0; i < civ_data_count; ++i) {
        printf("Civ[%u]: %s\n", i, civ_data_array[i].name.c_str());
    }
}

u16 CivData::find_civ_index (const std::string& civ_name) {
    for (u16 i = 0; i < civ_data_count; ++i) {
        if (civ_data_array[i].name == civ_name) {
            return i;
        }
    }
    printf("ERROR: Civ %s not found\n", civ_name.c_str());
    print_content();
    exit(1);
    return 0; // To make the compiler happy
}

u16 CivData::get_civ_data_count () {
    return civ_data_count;
}

const CivStats* CivData::get_civ_data_array () {
    return civ_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

void CivData::parse_and_allocate (const std::string& filename) {
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

    civ_data_count = 1;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (!line.empty()) {
            std::vector<std::string> parts = colon_splitter.split(line);
            if (parts.size() >= 1) {
                std::string civ_name = trimmer.trim(parts[0]);
                if (!civ_name.empty()) {
                    ++civ_data_count;
                }
            }
        }
    }
    if (civ_data_count == 1) {
        printf("ERROR: No civs found in data file '%s'\n", filename.c_str());
        exit(1);
    }
    civ_data_array = static_cast<CivStats*>(::operator new(sizeof(CivStats) * civ_data_count));
    if (civ_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u civs\n", civ_data_count);
        exit(1);
    }
    new (&civ_data_array[0].name) std::string("CIV_NONE");
    for (u16 t = 0; t < MAX_TRAITS_PER_CIV; ++t) {
        civ_data_array[0].trait_indices.indices[t] = 0;
    }
    u32 idx = 1;
    for (size_t i = 0; i < lines.size() && idx < civ_data_count; ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() >= 1) {
            std::string civ_name = trimmer.trim(parts[0]);
            if (civ_name.empty()) {
                continue;
            }
            new (&civ_data_array[idx].name) std::string(civ_name);
            for (u16 t = 0; t < MAX_TRAITS_PER_CIV; ++t) {
                civ_data_array[idx].trait_indices.indices[t] = 0;
            }
            u16 trait_count = 0;
            for (size_t j = 1; j < parts.size() && trait_count < MAX_TRAITS_PER_CIV; ++j) {
                std::string trait_name = trimmer.trim(parts[j]);
                if (!trait_name.empty()) {
                    u16 trait_idx = CivTraitData::find_civ_trait_index(trait_name);
                    civ_data_array[idx].trait_indices.indices[trait_count++] = trait_idx;
                }
            }
            idx++;
        }
    }
    if (idx != civ_data_count) {
        printf("ERROR: Did not parse %u civs from data file '%s'\n", civ_data_count, filename.c_str());
        exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
