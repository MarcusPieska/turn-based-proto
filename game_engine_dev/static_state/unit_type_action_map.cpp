//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_type_action_map.h"

//================================================================================================================================
//=> - UnitTypeActionMap globals -
//================================================================================================================================

StaticBitBank* UnitTypeActionMap::m_bit_bank = nullptr;
u16 UnitTypeActionMap::m_unit_type_count = 0;
u16 UnitTypeActionMap::m_action_count = 0;

//================================================================================================================================
//=> - UnitTypeActionMap implementation -
//================================================================================================================================

void UnitTypeActionMap::set_map (StaticBitBank* bit_bank, u16 unit_type_count, u16 action_count) {
    m_bit_bank = bit_bank;
    m_unit_type_count = unit_type_count;
    m_action_count = action_count;
}

bool UnitTypeActionMap::unit_type_can_do (u16 unit_type_idx, u16 action_idx) {
    if (m_bit_bank == nullptr) {
        return false;
    }
    if (unit_type_idx >= m_unit_type_count) {
        return false;
    }
    if (action_idx >= m_action_count) {
        return false;
    }
    return m_bit_bank->is_flagged(unit_type_idx, action_idx);
}

u16 UnitTypeActionMap::get_unit_type_count () {
    return m_unit_type_count;
}

u16 UnitTypeActionMap::get_action_count () {
    return m_action_count;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
