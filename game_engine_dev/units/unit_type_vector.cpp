//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "unit_data.h"
#include "unit_types.h"

#include "unit_type_vector.h"

//================================================================================================================================
//=> - BestBuildableUnits implementation -
//================================================================================================================================

BestBuildableUnits::BestBuildableUnits() {
    m_buildable = new BestUnitTypeInstance[UnitTypeData::get_unit_type_count()];
    for (u16 i = 0; i < UnitTypeData::get_unit_type_count(); ++i) {
        m_buildable[i].active_type = 0;
        m_buildable[i].unit_idx = 0;
    }
}

BestBuildableUnits::~BestBuildableUnits() {
    delete[] m_buildable;
}

u16 BestBuildableUnits::get_unit_type_count() const {
    return UnitTypeData::get_unit_type_count();
}

u16 BestBuildableUnits::is_type_active(u16 unit_type_idx) const {
    return m_buildable[unit_type_idx].active_type;
}

u16 BestBuildableUnits::get_best_unit_idx_of_type(u16 unit_type_idx) const {
    return m_buildable[unit_type_idx].unit_idx;
}

void BestBuildableUnits::set_best_unit_idx_of_type(u16 unit_type_idx, u16 unit_idx) {
    m_buildable[unit_type_idx].active_type = 1;
    m_buildable[unit_type_idx].unit_idx = unit_idx;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
