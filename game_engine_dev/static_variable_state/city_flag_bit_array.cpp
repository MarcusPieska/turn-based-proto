//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "city_flag_bit_array.h"

//================================================================================================================================
//=> - CityFlagBitArray implementation -
//================================================================================================================================

CityFlagBitArray::CityFlagBitArray (u16 count) :
    m_flags(count) {
}

CityFlagBitArray::~CityFlagBitArray () {
}

bool CityFlagBitArray::is_flagged (CityFlagStaticDataKey key) const {
    return m_flags.get_bit(key.value()) != 0;
}

void CityFlagBitArray::set_flag (CityFlagStaticDataKey key) {
    m_flags.set_bit(key.value());
}

void CityFlagBitArray::clear_flag (CityFlagStaticDataKey key) {
    m_flags.clear_bit(key.value());
}

bool CityFlagBitArray::can_access (u16 idx, CityFlagStaticDataKey& out_key) const {
    if (idx >= m_flags.get_count()) {
        return false;
    }
    if (m_flags.get_bit(idx) == 0) {
        return false;
    }
    out_key = CityFlagStaticDataKey::from_raw(idx);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
