//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "str_mng.h"
#include "bit_array.h"

#include "building_vector.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

struct __BuildingData {
    std::string name;
    int cost;
    std::string effect;
};

static __BuildingData* building_data_array = nullptr;
static uint32_t building_data_count = 0;

//================================================================================================================================
//=> - BuildingIO implementation -
//================================================================================================================================

int BuildingIO::validate_and_count () const {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        return 0;
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    int count = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() == 3) {
            bool valid = true;
            for (size_t j = 0; j < parts.size(); j++) {
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

void BuildingIO::print_content () const {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        printf("File is empty or could not be read.\n");
        return;
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() == 3) {
            std::string name = trimmer.trim(parts[0]);
            std::string cost = trimmer.trim(parts[1]);
            std::string effect = trimmer.trim(parts[2]);
            if (!name.empty() && !cost.empty() && !effect.empty()) {
                printf("Building: %s, Cost: %s, Effect: %s\n", name.c_str(), cost.c_str(), effect.c_str());
            }
        }
    }
}

void BuildingIO::parse_and_allocate () const {
    if (building_data_array != nullptr) {
        return;
    }
    int count = validate_and_count();
    if (count == 0) {
        return;
    }
    building_data_count = static_cast<uint32_t>(count);
    building_data_array = new __BuildingData[building_data_count];
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        return;
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    uint32_t index = 0;
    for (size_t i = 0; i < lines.size() && index < building_data_count; i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() == 3) {
            std::string name = trimmer.trim(parts[0]);
            std::string cost_str = trimmer.trim(parts[1]);
            std::string effect = trimmer.trim(parts[2]);
            if (!name.empty() && !cost_str.empty() && !effect.empty()) {
                building_data_array[index].name = name;
                building_data_array[index].cost = std::atoi(cost_str.c_str());
                building_data_array[index].effect = effect;
                index++;
            }
        }
    }
}

//================================================================================================================================
//=> - BuildingVector implementation -
//================================================================================================================================

BuildingVector::BuildingVector (uint32_t num_buildings) : num_buildings(num_buildings) {
    built_flags = new BitArrayCL(num_buildings);
    buildings_available = new BitArrayCL(num_buildings);
}

BuildingVector::~BuildingVector () {
    delete built_flags;
    delete buildings_available;
}

BuildingData BuildingVector::get_building (int index) const {
    BuildingData result;
    if (building_data_array == nullptr || index < 0 || static_cast<uint32_t>(index) >= building_data_count) {
        result.name = "";
        result.cost = 0;
        result.effect = "";
        result.exists = false;
        return result;
    }
    result.name = building_data_array[index].name;
    result.cost = building_data_array[index].cost;
    result.effect = building_data_array[index].effect;
    result.exists = (built_flags->get_bit(index) == 1);
    return result;
}

int BuildingVector::get_count () const {
    return static_cast<int>(num_buildings);
}

void BuildingVector::save (const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        built_flags->serialize(file);
        file.close();
    }
}

void BuildingVector::load (const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        delete built_flags;
        built_flags = BitArrayCL::deserialize(file);
        file.close();
    }
}

void BuildingVector::toggle_built (int index) {
    if (index < 0 || static_cast<uint32_t>(index) >= num_buildings) {
        return;
    }
    if (buildings_available->get_bit(index) == 1) {
        if (built_flags->get_bit(index) == 1) {
            built_flags->clear_bit(index);
        } else {
            built_flags->set_bit(index);
        }
    }
}

void BuildingVector::set_available (int index) {
    if (index < 0 || static_cast<uint32_t>(index) >= num_buildings) {
        return;
    }
    buildings_available->set_bit(index);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
