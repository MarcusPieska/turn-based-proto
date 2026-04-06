//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "buildings_built_array.h"

//================================================================================================================================
//=> - BuildingsBuiltArray implementation -
//================================================================================================================================

BuildingsBuiltArray::BuildingsBuiltArray (u16 flag_count) :
    m_flags(flag_count) {
}

BuildingsBuiltArray::~BuildingsBuiltArray () {
}

bool BuildingsBuiltArray::is_flagged (BuildingsBuiltKey key) const {
    return m_flags.get_bit(key.value()) != 0;
}

void BuildingsBuiltArray::set_flag (BuildingsBuiltKey key) {
    m_flags.set_bit(key.value());
}

void BuildingsBuiltArray::clear_flag (BuildingsBuiltKey key) {
    m_flags.clear_bit(key.value());
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
