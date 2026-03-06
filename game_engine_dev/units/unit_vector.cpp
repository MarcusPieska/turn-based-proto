//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"

#include "unit_vector.h"

//================================================================================================================================
//=> - UnitVector implementation -
//================================================================================================================================

const UnitTypeStats* UnitVector::get_unit_data_array () {
    return UnitData::get_unit_data_array ();
}

uint32_t UnitVector::get_unit_data_count () {
    return static_cast<uint32_t>(UnitData::get_unit_data_count ());
}

UnitVector::UnitVector (const BitArrayCL* researched_units) : 
    m_researched_units (researched_units), 
    m_units_unlocked (new BitArrayCL(researched_units->get_count())) {
}

UnitVector::~UnitVector () {
    delete m_units_unlocked;
}

UnitInstance UnitVector::get_unit (uint32_t index) const {
    uint16_t strength = 0;
    if (is_trainable (index)) {
        strength = 1000;
    }
    const UnitTypeStats* unit_data_array = UnitVector::get_unit_data_array ();
    UnitInstance result{unit_data_array[index], strength};
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
