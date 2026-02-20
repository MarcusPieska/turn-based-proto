//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "str_mng.h"
#include "bit_array.h"

#include "unit_vector.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static UnitTypeStats* unit_data_array = nullptr;
static uint32_t unit_data_count = 0;

const UnitTypeStats* UnitVector::get_unit_data_array () {
    return unit_data_array;
}

uint32_t UnitVector::get_unit_data_count () {
    return unit_data_count;
}

//================================================================================================================================
//=> - UnitIO implementation -
//================================================================================================================================

int UnitIO::validate_and_count () const {
    StringReader reader(m_filename);
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
        if (parts.size() == 5) {
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

void UnitIO::print_content () const {
    StringReader reader(m_filename);
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
        if (parts.size() == 5) {
            std::string name = trimmer.trim(parts[0]);
            std::string cost = trimmer.trim(parts[1]);
            std::string attack = trimmer.trim(parts[2]);
            std::string defense = trimmer.trim(parts[3]);
            std::string moves = trimmer.trim(parts[4]);
            if (!name.empty() && !cost.empty() && !attack.empty() && !defense.empty() && !moves.empty()) {
                printf("Unit: %s, Cost: %s, Attack: %s, Defense: %s, Moves: %s\n", 
                       name.c_str(), cost.c_str(), attack.c_str(), defense.c_str(), moves.c_str());
            }
        }
    }
}

void UnitIO::parse_and_allocate () const {
    if (unit_data_array != nullptr) {
        return;
    }
    int count = validate_and_count();
    if (count == 0) {
        return;
    }
    unit_data_count = static_cast<uint32_t>(count);
    unit_data_array = new UnitTypeStats[unit_data_count];
    StringReader reader(m_filename);
    std::string content = reader.read();
    if (content.empty()) {
        return;
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    uint32_t index = 0;
    for (size_t i = 0; i < lines.size() && index < unit_data_count; i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() == 5) {
            std::string name = trimmer.trim(parts[0]);
            std::string cost_str = trimmer.trim(parts[1]);
            std::string attack_str = trimmer.trim(parts[2]);
            std::string defense_str = trimmer.trim(parts[3]);
            std::string moves_str = trimmer.trim(parts[4]);
            if (!name.empty() && !cost_str.empty() && !attack_str.empty() && !defense_str.empty() && !moves_str.empty()) {
                unit_data_array[index].name = name;
                unit_data_array[index].cost = std::atoi(cost_str.c_str());
                unit_data_array[index].attack = std::atoi(attack_str.c_str());
                unit_data_array[index].defense = std::atoi(defense_str.c_str());
                unit_data_array[index].moves = std::atoi(moves_str.c_str());
                index++;
            }
        }
    }
}

//================================================================================================================================
//=> - UnitVector implementation -
//================================================================================================================================

UnitVector::UnitVector (const BitArrayCL* researched_units) : 
    m_researched_units (researched_units), 
    m_units_unlocked (new BitArrayCL(researched_units->get_count())) {
}

UnitVector::~UnitVector () {
    delete m_units_unlocked;
}

UnitData UnitVector::get_unit (uint32_t index) const {
    uint16_t strength = 0;
    if (is_trainable (index)) {
        strength = 1000;
    }
    UnitData result{unit_data_array[index], strength};
    return result;
}

bool UnitVector::is_trainable (uint32_t index) const {
    return m_researched_units->get_bit(index) == 1 && m_units_unlocked->get_bit(index) == 1;
}

uint32_t UnitVector::get_count () const {
    return m_units_unlocked->get_count();
}

void UnitVector::set_trainable (uint32_t index) {
    if (index >= m_units_unlocked->get_count()) {
        return;
    }
    m_units_unlocked->set_bit(index);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
