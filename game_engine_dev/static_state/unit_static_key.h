//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_STATIC_KEY_H
#define UNIT_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - UnitStaticDataKey class -
//================================================================================================================================

class UnitStaticDataKey {
    friend class UnitStaticData;
public:
    constexpr UnitStaticDataKey() : m_val(0) {}

    constexpr bool operator==(UnitStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(UnitStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr UnitStaticDataKey None() { 
        return UnitStaticDataKey(0); 
    }

    static constexpr UnitStaticDataKey from_raw(u16 val) {
        return UnitStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr UnitStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // UNIT_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
