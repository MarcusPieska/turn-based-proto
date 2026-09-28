//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "world_unit_city_save_timeline.h"

//================================================================================================================================
//=> - WorldUnitCitySaveTimeline -
//================================================================================================================================

WorldUnitCitySaveTimeline::WorldUnitCitySaveTimeline ()
    : m_units(nullptr),
      m_cities(nullptr),
      m_save_n(0) {
}

WorldUnitCitySaveTimeline::~WorldUnitCitySaveTimeline () {
    clr();
}

void WorldUnitCitySaveTimeline::clr () {
    delete[] m_units;
    m_units = nullptr;
    delete[] m_cities;
    m_cities = nullptr;
    m_save_n = 0;
}

bool WorldUnitCitySaveTimeline::setup (u16 save_n) {
    clr();
    if (save_n == 0) {
        return false;
    }
    m_units = new u32[save_n];
    m_cities = new u32[save_n];
    if (m_units == nullptr || m_cities == nullptr) {
        clr();
        return false;
    }
    m_save_n = save_n;
    for (u16 i = 0; i < save_n; ++i) {
        m_units[i] = 0;
        m_cities[i] = 0;
    }
    return true;
}

bool WorldUnitCitySaveTimeline::set (u16 save_i, u32 units, u32 cities) {
    if (m_units == nullptr || m_cities == nullptr || save_i >= m_save_n) {
        return false;
    }
    m_units[save_i] = units;
    m_cities[save_i] = cities;
    return true;
}

u16 WorldUnitCitySaveTimeline::save_n () const {
    return m_save_n;
}

u32 WorldUnitCitySaveTimeline::units_at (u16 save_i) const {
    if (m_units == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_units[save_i];
}

u32 WorldUnitCitySaveTimeline::cities_at (u16 save_i) const {
    if (m_cities == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_cities[save_i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
