//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "unit_type_action_map.h"

//================================================================================================================================
//=> - UnitTypeActionMap implementation -
//================================================================================================================================

void UnitTypeActionMap::set_map (StaticBitBank* bit_bank, u16 unit_type_count, u16 action_count) {
    m_bit_bank = bit_bank;
    m_unit_type_count = unit_type_count;
    m_action_count = action_count;
}

void UnitTypeActionMap::take_ownership () {
    StaticBitBank* src = m_bit_bank;
    m_bit_bank = new StaticBitBank(m_unit_type_count, m_action_count);
    for (u16 u = 0; u < m_unit_type_count; ++u) {
        for (u16 a = 0; a < m_action_count; ++a) {
            if (src->is_flagged(u, a)) {
                m_bit_bank->set_flag(u, a);
            }
        }
    }
    delete src;
}

void UnitTypeActionMap::release_map () {
    delete m_bit_bank;
    m_bit_bank = nullptr;
    m_unit_type_count = 0;
    m_action_count = 0;
}

bool UnitTypeActionMap::unit_type_can_do (u16 unit_type_idx, u16 action_idx) const {
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

u16 UnitTypeActionMap::get_unit_type_count () const {
    return m_unit_type_count;
}

u16 UnitTypeActionMap::get_action_count () const {
    return m_action_count;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
