//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TECH_STATIC_KEY_H
#define TECH_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - TechStaticDataKey class -
//================================================================================================================================

class TechStaticDataKey {
    friend class TechStaticData;
public:
    constexpr TechStaticDataKey() : m_val(0) {}

    constexpr bool operator==(TechStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(TechStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr TechStaticDataKey None() { 
        return TechStaticDataKey(0); 
    }

    static constexpr TechStaticDataKey from_raw(u16 val) {
        return TechStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr TechStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // TECH_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
