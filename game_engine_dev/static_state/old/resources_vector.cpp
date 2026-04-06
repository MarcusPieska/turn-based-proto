//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"

#include "resources_vector.h"

//================================================================================================================================
//=> - ResourceVector implementation -
//================================================================================================================================

ResourceVector::ResourceVector (uint32_t num_resources, BitArrayCL* resources_available) : 
    m_resources_available (resources_available) {
}

ResourceVector::~ResourceVector () {
}

ResourceTypeStats ResourceVector::get_resource (uint32_t index) const {
    const ResourceTypeStats* resource_data_array = ResourceData::get_resource_data_array();
    return resource_data_array[index];
}

bool ResourceVector::is_available (uint32_t index) const {
    return m_resources_available->get_bit(index);
}

uint32_t ResourceVector::get_count () const {
    return m_resources_available->get_count();
}

void ResourceVector::set_available (uint32_t index) {
    if (index >= m_resources_available->get_count()) {
        return;
    }
    m_resources_available->set_bit(index);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
