//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_ACTION_STATIC_KEY_H
#define UNIT_ACTION_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - UnitActionStaticDataKey class -
//================================================================================================================================

class UnitActionStaticDataKey {
    friend class UnitActionStaticData;
public:
    constexpr UnitActionStaticDataKey() : m_val(0) {}

    constexpr bool operator==(UnitActionStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(UnitActionStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr UnitActionStaticDataKey None() { 
        return UnitActionStaticDataKey(0); 
    }

    static constexpr UnitActionStaticDataKey from_raw(u16 val) {
        return UnitActionStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr UnitActionStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // UNIT_ACTION_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
