//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "techs_researched_array.h"

//================================================================================================================================
//=> - TechsResearchedArray implementation -
//================================================================================================================================

TechsResearchedArray::TechsResearchedArray (u16 flag_count) :
    m_flags(flag_count) {
}

TechsResearchedArray::~TechsResearchedArray () {
}

bool TechsResearchedArray::is_flagged (TechsResearchedKey key) const {
    return m_flags.get_bit(key.value()) != 0;
}

void TechsResearchedArray::set_flag (TechsResearchedKey key) {
    m_flags.set_bit(key.value());
}

void TechsResearchedArray::clear_flag (TechsResearchedKey key) {
    m_flags.clear_bit(key.value());
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
