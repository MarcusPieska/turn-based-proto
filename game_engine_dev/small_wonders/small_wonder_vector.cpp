//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include "small_wonder_vector.h"

//================================================================================================================================
//=> - BuiltSmallWonders implementation -
//================================================================================================================================

BuiltSmallWonders::BuiltSmallWonders () {
    u16 wonder_count = SmallWonderData::get_small_wonder_data_count();
    m_built = new u16[wonder_count];
    std::memset(m_built, 0, wonder_count * sizeof(u16));
}

BuiltSmallWonders::~BuiltSmallWonders () {
    delete[] m_built;
}

void BuiltSmallWonders::set_owning_city (u16 idx, u16 city_id) {
    m_built[idx] = city_id;
}

bool BuiltSmallWonders::has_been_built (u16 idx) const {
    return m_built[idx] > 0;
}

u16 BuiltSmallWonders::get_owning_city (u16 idx) const {
    return m_built[idx];
}

//================================================================================================================================
//=> - BuildableSmallWonders implementation -
//================================================================================================================================

BuildableSmallWonders::BuildableSmallWonders () : 
    m_buildable(SmallWonderData::get_small_wonder_data_count()) {
}

BuildableSmallWonders::~BuildableSmallWonders () {
}

void BuildableSmallWonders::set_buildable (u16 idx) {
    m_buildable.set_bit(idx);
}

bool BuildableSmallWonders::can_build (u16 idx) const {
    return m_buildable.get_bit(idx) == 1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
