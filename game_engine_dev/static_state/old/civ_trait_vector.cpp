//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "civ_trait_data.h"

#include "civ_trait_vector.h"

//================================================================================================================================
//=> - CivTraitVector implementation -
//================================================================================================================================

CivTraitVector::CivTraitVector()
    : m_traits(CivTraitData::get_civ_trait_count()) {
}

CivTraitVector::~CivTraitVector() {
}

bool CivTraitVector::has_trait(u16 trait_idx) const {
    return m_traits.get_bit(trait_idx) == 1;
}

const CivTraitStats& CivTraitVector::get_trait_data(u16 trait_idx) const {
    const CivTraitStats* trait_array = CivTraitData::get_civ_trait_data_array();
    return trait_array[trait_idx];
}

void CivTraitVector::set_trait(u16 trait_idx) {
    m_traits.set_bit(trait_idx);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
