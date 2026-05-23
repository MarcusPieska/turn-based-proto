//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "civ_bld_discount_map.h"

//================================================================================================================================
//=> - CivBldDiscountMap implementation -
//================================================================================================================================

void CivBldDiscountMap::set_map (StaticBitBank* bit_bank, u16 civ_trait_count, u16 building_count) {
    m_bit_bank = bit_bank;
    m_civ_trait_count = civ_trait_count;
    m_building_count = building_count;
}

void CivBldDiscountMap::take_ownership () {
    StaticBitBank* src = m_bit_bank;
    m_bit_bank = new StaticBitBank(m_civ_trait_count, m_building_count);
    for (u16 c = 0; c < m_civ_trait_count; ++c) {
        for (u16 b = 0; b < m_building_count; ++b) {
            if (src->is_flagged(c, b)) {
                m_bit_bank->set_flag(c, b);
            }
        }
    }
    delete src;
}

void CivBldDiscountMap::release_map () {
    delete m_bit_bank;
    m_bit_bank = nullptr;
    m_civ_trait_count = 0;
    m_building_count = 0;
}

bool CivBldDiscountMap::civ_trait_has_discount_for_bld (u16 civ_trait_idx, u16 building_idx) const {
    if (m_bit_bank == nullptr) {
        return false;
    }
    if (civ_trait_idx >= m_civ_trait_count) {
        return false;
    }
    if (building_idx >= m_building_count) {
        return false;
    }
    return m_bit_bank->is_flagged(civ_trait_idx, building_idx);
}

u16 CivBldDiscountMap::get_civ_trait_count () const {
    return m_civ_trait_count;
}

u16 CivBldDiscountMap::get_building_count () const {
    return m_building_count;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
