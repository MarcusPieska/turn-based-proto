//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include "wonder_vector.h"

//================================================================================================================================
//=> - Static data -
//================================================================================================================================

static u16* s_wonder_owning_city_array = nullptr;

//================================================================================================================================
//=> - WondersBuiltVector implementation -
//================================================================================================================================

void WondersBuiltVector::allocate_static_array () {
    u16 wonder_count = WonderData::get_wonder_data_count();
    s_wonder_owning_city_array = new u16[wonder_count];
    std::memset(s_wonder_owning_city_array, 0, wonder_count * sizeof(u16));
}

void WondersBuiltVector::set_owning_city (u16 idx, u16 city_id) {
    s_wonder_owning_city_array[idx] = city_id;
}

bool WondersBuiltVector::has_been_built (u16 idx) {
    return s_wonder_owning_city_array[idx] > 0;
}

//================================================================================================================================
//=> - WonderBuildableVector implementation -
//================================================================================================================================

WonderBuildableVector::WonderBuildableVector () {
    u16 wonder_count = WonderData::get_wonder_data_count();
    m_buildable = new BitArrayCL(static_cast<uint32_t>(wonder_count));
}

WonderBuildableVector::~WonderBuildableVector () {
    delete m_buildable;
}

void WonderBuildableVector::set_buildable (u16 idx) {
    m_buildable->set_bit(static_cast<int>(idx));
}

bool WonderBuildableVector::can_build (u16 idx) const {
    return m_buildable->get_bit(static_cast<int>(idx)) == 1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
