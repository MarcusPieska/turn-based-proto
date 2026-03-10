//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdint>
#include <string>

#include "bit_array.h"

#include "building_data.h"
#include "building_vector.h"
#include "building_assessor.h"

#include "wonder_data.h"
#include "wonder_vector.h"
#include "wonder_assessor.h"

#include "small_wonder_data.h"
#include "small_wonder_vector.h"
#include "small_wonder_assessor.h"

#include "unit_data.h"
#include "unit_vector.h"
#include "unit_assessor.h"

#include "city_flags.h"
#include "resource_data.h"
#include "unit_data.h"

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
      m_bld_idx(0),
      m_flags(CityFlagData::get_flag_count()),
      m_buildings(),
      m_resources(ResourceData::get_resource_data_count())
{
}

City::~City () {
}

BuildableBuildings* City::get_buildable_buildings (BitArrayCL* techs) {
    return BuildingAssessor::assess(techs, &m_buildings, &m_resources, &m_flags);
}

BuildableWonders* City::get_buildable_wonders (BitArrayCL* techs) {
    return WonderAssessor::assess(techs, &m_buildings, &m_resources, &m_flags);
}

BuildableSmallWonders* City::get_buildable_small_wonders (BitArrayCL* techs, BuiltSmallWonders* built) {
    return SmallWonderAssessor::assess(techs, &m_buildings, &m_resources, &m_flags, built);
}

BuildableUnits* City::get_trainable_units (BitArrayCL* techs) {
    return UnitAssessor::assess(techs, &m_buildings, &m_resources, &m_flags);
}

void City::build_building (u16 building_idx) {
    m_build_type = BUILD_TYPE_BUILDING;
    m_bld_idx = building_idx;
}

void City::build_wonder (u16 wonder_idx) {
    m_build_type = BUILD_TYPE_WONDER;
    m_bld_idx = wonder_idx;
}

void City::build_small_wonder (u16 small_wonder_idx) {
    m_build_type = BUILD_TYPE_SMALL_WONDER;
    m_bld_idx = small_wonder_idx;
}

void City::build_unit (u16 unit_idx) {
    m_build_type = BUILD_TYPE_UNIT;
    m_bld_idx = unit_idx;
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
            return m_accumulated_shields >= WonderData::get_wonder_data_array()[m_bld_idx].cost;
        }
        case BUILD_TYPE_SMALL_WONDER: {
            return m_accumulated_shields >= SmallWonderData::get_small_wonder_data_array()[m_bld_idx].cost;
        }
        case BUILD_TYPE_UNIT: {
            return m_accumulated_shields >= UnitData::get_unit_data_array()[m_bld_idx].cost;
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