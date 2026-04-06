//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "resources_available_array.h"

//================================================================================================================================
//=> - ResourcesAvailableArray implementation -
//================================================================================================================================

ResourcesAvailableArray::ResourcesAvailableArray (u16 flag_count) :
    m_flags(flag_count) {
}

ResourcesAvailableArray::~ResourcesAvailableArray () {
}

bool ResourcesAvailableArray::is_flagged (ResourcesAvailableKey key) const {
    return m_flags.get_bit(key.value()) != 0;
}

void ResourcesAvailableArray::set_flag (ResourcesAvailableKey key) {
    m_flags.set_bit(key.value());
}

void ResourcesAvailableArray::clear_flag (ResourcesAvailableKey key) {
    m_flags.clear_bit(key.value());
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
