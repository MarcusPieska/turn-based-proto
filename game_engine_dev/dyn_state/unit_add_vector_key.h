//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_ADD_VECTOR_KEY_H
#define UNIT_ADD_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - UnitAddKey class -
//================================================================================================================================

class UnitAddKey {
    friend class UnitAddVector;
public:
    constexpr UnitAddKey() : m_val(0) {}

    constexpr bool operator==(UnitAddKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(UnitAddKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr UnitAddKey None() { 
        return UnitAddKey(0); 
    }

    static constexpr UnitAddKey from_raw(u16 val) {
        return UnitAddKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr UnitAddKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // UNIT_ADD_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
