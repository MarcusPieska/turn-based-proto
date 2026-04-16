//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_TYPE_STATIC_KEY_H
#define UNIT_TYPE_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - UnitTypeStaticDataKey class -
//================================================================================================================================

class UnitTypeStaticDataKey {
    friend class UnitTypeStaticData;
public:
    constexpr UnitTypeStaticDataKey() : m_val(0) {}

    constexpr bool operator==(UnitTypeStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(UnitTypeStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr UnitTypeStaticDataKey None() { 
        return UnitTypeStaticDataKey(0); 
    }

    static constexpr UnitTypeStaticDataKey from_raw(u16 val) {
        return UnitTypeStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr UnitTypeStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // UNIT_TYPE_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
