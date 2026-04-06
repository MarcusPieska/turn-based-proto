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

#include "unit_types.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static UnitTypeName* unit_type_data_array = nullptr;
static u16 unit_type_count = 0;

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================

void UnitTypeData::load_static_data (const std::string& filename) {
    if (unit_type_data_array != nullptr) {
        return;
    }
    parse_and_allocate(filename);
}

void UnitTypeData::print_content () {
    printf("Unit type data count: (total=%u)\n", unit_type_count);
    for (u16 i = 0; i < unit_type_count; ++i) {
        printf("UnitType[%u]: %s\n", i, unit_type_data_array[i].name.c_str());
    }
}

u16 UnitTypeData::find_unit_type_index (const std::string& unit_type_name) {
    for (u16 i = 0; i < unit_type_count; ++i) {
        if (unit_type_data_array[i].name == unit_type_name) {
            return i;
        }
    }
    printf("ERROR: Unit type %s not found\n", unit_type_name.c_str());
    print_content();
    exit(1);
    return 0; // To make the compiler happy
}

u16 UnitTypeData::get_unit_type_count () {
    return unit_type_count;
}

const UnitTypeName* UnitTypeData::get_unit_type_name_array () {
    return unit_type_data_array;
}

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

void UnitTypeData::parse_and_allocate (const std::string& filename) {
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
            ++unit_type_count;
        }
    }
    if (unit_type_count == 0) {
        printf("ERROR: No unit types found in data file '%s'\n", filename.c_str());
        exit(1);
    }
    unit_type_data_array = new UnitTypeName[unit_type_count];
    if (unit_type_data_array == nullptr) {
        printf("ERROR: Failed to allocate memory for %u unit types\n", unit_type_count);
        exit(1);
    }
    u32 idx = 0;
    for (size_t i = 0; i < lines.size() && idx < unit_type_count; ++i) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        unit_type_data_array[idx++].name = line;
    }
    if (idx != unit_type_count) {
        printf("ERROR: Did not parse %u unit types from data file '%s'\n", unit_type_count, filename.c_str());
        exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
