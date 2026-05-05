//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "civ_bld_discount_map.h"

//================================================================================================================================
//=> - CivBldDiscountMap globals -
//================================================================================================================================

StaticBitBank* CivBldDiscountMap::m_bit_bank = nullptr;
u16 CivBldDiscountMap::m_civ_trait_count = 0;
u16 CivBldDiscountMap::m_building_count = 0;

//================================================================================================================================
//=> - CivBldDiscountMap implementation -
//================================================================================================================================

void CivBldDiscountMap::set_map (StaticBitBank* bit_bank, u16 civ_trait_count, u16 building_count) {
    m_bit_bank = bit_bank;
    m_civ_trait_count = civ_trait_count;
    m_building_count = building_count;
}

bool CivBldDiscountMap::civ_trait_has_discount_for_bld (u16 civ_trait_idx, u16 building_idx) {
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

u16 CivBldDiscountMap::get_civ_trait_count () {
    return m_civ_trait_count;
}

u16 CivBldDiscountMap::get_building_count () {
    return m_building_count;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
