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
static u32 flag_count = 0;

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
    if (flag_data_array == nullptr || flag_count == 0) {
        printf("City flag data not loaded.\n");
        return;
    }

    for (u32 i = 0; i < flag_count; ++i) {
        printf("Flag[%u]: %s\n", static_cast<u32>(i), flag_data_array[i].name.c_str());
    }
}

u16 CityFlagData::find_flag_index (const std::string& flag_name) {
    for (u32 i = 0; i < flag_count; ++i) {
        if (flag_data_array[i].name == flag_name) {
            return static_cast<u16>(i);
        }
    }

    printf("CRITICAL ERROR: Flag %s not found\n", flag_name.c_str());
    exit(1);
    return 0; // To make the compiler happy
}

u16 CityFlagData::get_flag_count () {
    return static_cast<u16>(flag_count);
}

const CityFlagStats* CityFlagData::get_flag_data_array () {
    return flag_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

void CityFlagData::parse_and_allocate (const std::string& filename) {
    if (flag_data_array != nullptr) {
        return;
    }

    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        return;
    }

    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");

    u32 count = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (!line.empty()) {
            ++count;
        }
    }

    if (count == 0) {
        return;
    }

    flag_count = count;
    flag_data_array = new CityFlagStats[flag_count];

    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < flag_count; ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        flag_data_array[idx++].name = line;
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
