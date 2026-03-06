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
#include "building_data.h"

#include "building_vector.h"

//================================================================================================================================
//=> - BuildingVector implementation -
//================================================================================================================================

BuildingVector::BuildingVector (const BitArrayCL* researched_buildings) : 
    m_bld_researched (researched_buildings), 
    m_bld_unlocked (new BitArrayCL(researched_buildings->get_count())),
    m_bld_built (new BitArrayCL(researched_buildings->get_count())) {
}

BuildingVector::~BuildingVector () {
    delete m_bld_built;
    delete m_bld_unlocked;
}

const BuildingTypeStats& BuildingVector::get_building_stats (u32 idx) const {
    const BuildingTypeStats* data = BuildingData::get_building_data_array();
    return data[idx];
}

bool BuildingVector::is_buildable (u32 idx) const {
    return m_bld_researched->get_bit(idx) == 1 && m_bld_unlocked->get_bit(idx) == 1 &&  m_bld_built->get_bit(idx) == 0;
}

bool BuildingVector::is_built (u32 idx) const {
    return m_bld_built->get_bit(idx) == 1;
}

u32 BuildingVector::get_count () const {
    return m_bld_unlocked->get_count();
}

void BuildingVector::save (const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        m_bld_built->serialize(file);
        file.close();
    }
}

void BuildingVector::load (const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        delete m_bld_built;
        m_bld_built = BitArrayCL::deserialize(file);
        file.close();
    }
}

void BuildingVector::toggle_built (u32 idx) {
    if (m_bld_built->get_bit(idx) == 1) {
        m_bld_built->clear_bit(idx);
    } else {
        m_bld_built->set_bit(idx);
    }
}

void BuildingVector::set_built (u32 idx) {
    m_bld_built->set_bit(idx);
}

void BuildingVector::clear_built (u32 idx) {
    m_bld_built->clear_bit(idx);
}

void BuildingVector::set_buildable (u32 idx) {
    m_bld_unlocked->set_bit(idx);
}

//================================================================================================================================
//=> - Static data accessors -
//================================================================================================================================

const BuildingTypeStats* BuildingVector::get_building_data_array () {
    return BuildingData::get_building_data_array();
}

u32 BuildingVector::get_building_data_count () {
    return static_cast<u32>(BuildingData::get_building_data_count());
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
