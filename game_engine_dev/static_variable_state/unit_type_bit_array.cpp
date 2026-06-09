//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_type_bit_array.h"

//================================================================================================================================
//=> - UnitTypeBitArray implementation -
//================================================================================================================================

UnitTypeBitArray::UnitTypeBitArray (u16 count) :
    m_flags(count) {
}

UnitTypeBitArray::~UnitTypeBitArray () {
}

bool UnitTypeBitArray::is_flagged (UnitTypeStaticDataKey key) const {
    return m_flags.get_bit(key.value()) != 0;
}

void UnitTypeBitArray::set_flag (UnitTypeStaticDataKey key) {
    m_flags.set_bit(key.value());
}

void UnitTypeBitArray::clear_flag (UnitTypeStaticDataKey key) {
    m_flags.clear_bit(key.value());
}

bool UnitTypeBitArray::can_access (u16 idx, UnitTypeStaticDataKey& out_key) const {
    if (idx >= m_flags.get_count()) {
        return false;
    }
    if (m_flags.get_bit(idx) == 0) {
        return false;
    }
    out_key = UnitTypeStaticDataKey::from_raw(idx);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
