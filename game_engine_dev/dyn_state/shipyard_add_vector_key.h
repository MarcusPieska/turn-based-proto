//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SHIPYARD_ADD_VECTOR_KEY_H
#define SHIPYARD_ADD_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - ShipyardAddKey class -
//================================================================================================================================

class ShipyardAddKey {
    friend class ShipyardAddVector;
public:
    constexpr ShipyardAddKey() : m_val(0) {}

    constexpr bool operator==(ShipyardAddKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(ShipyardAddKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr ShipyardAddKey None() { 
        return ShipyardAddKey(0); 
    }

    static constexpr ShipyardAddKey from_raw(u16 val) {
        return ShipyardAddKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr ShipyardAddKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // SHIPYARD_ADD_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
