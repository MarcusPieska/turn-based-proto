//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include "building_vector.h"

//================================================================================================================================
//=> - BuiltBuildings implementation -
//================================================================================================================================

BuiltBuildings::BuiltBuildings () : 
    m_built(static_cast<u32>(BuildingData::get_building_data_count())) {
}

BuiltBuildings::~BuiltBuildings () {
}

void BuiltBuildings::set_built (u32 idx) {
    m_built.set_bit(idx);
}

void BuiltBuildings::clear_built (u32 idx) {
    m_built.clear_bit(idx);
}

bool BuiltBuildings::has_been_built (u32 idx) const {
    return m_built.get_bit(idx) == 1;
}

void BuiltBuildings::save (const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        m_built.serialize(file);
        file.close();
    }
}

void BuiltBuildings::load (const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        BitArrayCL* temp = BitArrayCL::deserialize(file);
        u32 count = temp->get_count();
        for (u32 i = 0; i < count; i++) {
            if (temp->get_bit(i) == 1) {
                m_built.set_bit(i);
            } else {
                m_built.clear_bit(i);
            }
        }
        delete temp;
        file.close();
    }
}

//================================================================================================================================
//=> - BuildableBuildings implementation -
//================================================================================================================================

BuildableBuildings::BuildableBuildings () : 
    m_buildable(static_cast<u32>(BuildingData::get_building_data_count())) {
}

BuildableBuildings::~BuildableBuildings () {
}

void BuildableBuildings::set_buildable (u32 idx) {
    m_buildable.set_bit(idx);
}

bool BuildableBuildings::can_build (u32 idx) const {
    return m_buildable.get_bit(idx) == 1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
