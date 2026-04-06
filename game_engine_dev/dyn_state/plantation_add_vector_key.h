//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLANTATION_ADD_VECTOR_KEY_H
#define PLANTATION_ADD_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - PlantationAddKey class -
//================================================================================================================================

class PlantationAddKey {
    friend class PlantationAddVector;
public:
    constexpr PlantationAddKey() : m_val(0) {}

    constexpr bool operator==(PlantationAddKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(PlantationAddKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr PlantationAddKey None() { 
        return PlantationAddKey(0); 
    }

    static constexpr PlantationAddKey from_raw(u16 val) {
        return PlantationAddKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr PlantationAddKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // PLANTATION_ADD_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
