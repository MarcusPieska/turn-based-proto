//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdint>
#include <string>

#include "bit_array.h"
#include "wonder_data.h"
#include "wonder_vector.h"
#include "wonder_assessor.h"
#include "city_flags.h"
#include "building_data.h"
#include "resource_data.h"

#include "city.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef enum BuildType : u8 {
    BUILD_TYPE_BUILDING = 0,
    BUILD_TYPE_WONDER = 1,
    BUILD_TYPE_SMALL_WONDER = 2,
    BUILD_TYPE_UNIT = 3,
    ACCUMULATE_COMMERCE = 4
} BuildType;

//================================================================================================================================
//=> - City implementation -
//================================================================================================================================

City::City ()
    : m_accumulated_food(0),
      m_accumulated_shields(0),
      m_build_type(0),
      m_build_index(0),
      m_flags(CityFlagData::get_flag_count()),
      m_buildings(BuildingData::get_building_data_count()),
      m_resources(ResourceData::get_resource_data_count())
{
}

City::~City () {
}

BitArrayCL* City::get_buildable_buildings (BitArrayCL* techs) {
    return nullptr;
}

WonderBuildableVector* City::get_buildable_wonders (BitArrayCL* techs) {
    return WonderAssessor::assess(techs, &m_flags, &m_buildings, &m_resources);
}

BitArrayCL* City::get_buildable_small_wonders (BitArrayCL* techs) {
    return nullptr;
}

BitArrayCL* City::get_trainable_units (BitArrayCL* techs) {
    return nullptr;
}

void City::build_building (u16 building_idx) {
    m_build_type = 0;
    m_build_index = building_idx;
}

void City::build_wonder (u16 wonder_idx) {
    m_build_type = 1;
    m_build_index = wonder_idx;
}

void City::build_small_wonder (u16 small_wonder_idx) {
    m_build_type = 2;
    m_build_index = small_wonder_idx;
}

void City::build_unit (u16 unit_idx) {
    m_build_type = 3;
    m_build_index = unit_idx;
}

void City::add_food (u16 amount) {
    m_accumulated_food += amount;
}

void City::add_shields (u16 amount) {
    m_accumulated_shields += amount;
}

bool City::is_build_done () const {
    switch (m_build_type) {
        case BUILD_TYPE_WONDER: {
            return m_accumulated_shields >= WonderData::get_wonder_data_array()[m_build_index].cost;
        }
        case BUILD_TYPE_SMALL_WONDER: {
            return false;
        }
        case BUILD_TYPE_UNIT: {
            return false;
        }
        case ACCUMULATE_COMMERCE: {
            return false;
        }
        default: {
            return false;
        }
    }
}

bool City::is_ready_to_grow () const {
    return false;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================