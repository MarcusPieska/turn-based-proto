//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TECHS_RESEARCHED_ARRAY_KEY_H
#define TECHS_RESEARCHED_ARRAY_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - TechsResearchedKey class -
//================================================================================================================================

class TechsResearchedKey {
    friend class TechsResearchedArray;
    friend class TechsResearchedAccessor;
public:
    constexpr TechsResearchedKey() : m_val(0) {}

    constexpr bool operator==(TechsResearchedKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(TechsResearchedKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr TechsResearchedKey None() { 
        return TechsResearchedKey(0); 
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr TechsResearchedKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // TECHS_RESEARCHED_ARRAY_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
