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

#include "city_flags.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static CityFlagStats* flag_data_array = nullptr;
static u16 flag_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================

void CityFlagData::load_static_data (const std::string& filename) {
    if (flag_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void CityFlagData::print_content () {
    printf("City flag data count: (total=%u)\n", flag_count);
    for (u16 i = 0; i < flag_count; ++i) {
        printf("Flag[%u]: %s\n", i, flag_data_array[i].name.c_str());
    }
}

u16 CityFlagData::find_flag_index (const std::string& flag_name) {
    for (u16 i = 0; i < flag_count; ++i) {
        if (flag_data_array[i].name == flag_name) {
            return i;
        }
    }
    printf("ERROR: Flag %s not found\n", flag_name.c_str());
    print_content();
    exit(1);
    return 0; // To make the compiler happy
}

u16 CityFlagData::get_flag_count () {
    return flag_count;
}

const CityFlagStats* CityFlagData::get_flag_data_array () {
    return flag_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

void CityFlagData::parse_and_allocate (const std::string& filename) {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("ERROR: No content found in file '%s'\n", filename.c_str());
        exit(1);
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (!line.empty()) {
            ++flag_count;
        }
    }
    if (flag_count == 0) {
        printf("ERROR: No flags found in data file '%s'\n", filename.c_str());
        exit(1);
    }
    flag_data_array = new CityFlagStats[flag_count];
    if (flag_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u flags\n", flag_count);
        exit(1);
    }
    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < flag_count; ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        flag_data_array[idx++].name = line;
    }
    if (idx != flag_count) {
        printf("ERROR: Did not parse %u flags from data file '%s'\n", flag_count, filename.c_str());
        exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
