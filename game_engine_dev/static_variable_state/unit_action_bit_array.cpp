//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_action_bit_array.h"

//================================================================================================================================
//=> - UnitActionBitArray implementation -
//================================================================================================================================

UnitActionBitArray::UnitActionBitArray (u16 count) :
    m_flags(count) {
}

UnitActionBitArray::~UnitActionBitArray () {
}

bool UnitActionBitArray::is_flagged (UnitActionStaticDataKey key) const {
    return m_flags.get_bit(key.value()) != 0;
}

void UnitActionBitArray::set_flag (UnitActionStaticDataKey key) {
    m_flags.set_bit(key.value());
}

void UnitActionBitArray::clear_flag (UnitActionStaticDataKey key) {
    m_flags.clear_bit(key.value());
}

bool UnitActionBitArray::can_access (u16 idx, UnitActionStaticDataKey& out_key) const {
    if (idx >= m_flags.get_count()) {
        return false;
    }
    if (m_flags.get_bit(idx) == 0) {
        return false;
    }
    out_key = UnitActionStaticDataKey::from_raw(idx);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
