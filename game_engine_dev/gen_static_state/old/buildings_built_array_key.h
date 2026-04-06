//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDINGS_BUILT_ARRAY_KEY_H
#define BUILDINGS_BUILT_ARRAY_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - BuildingsBuiltKey class -
//================================================================================================================================

class BuildingsBuiltKey {
    friend class BuildingsBuiltArray;
    friend class BuildingsBuiltAccessor;
public:
    constexpr BuildingsBuiltKey() : m_val(0) {}

    constexpr bool operator==(BuildingsBuiltKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(BuildingsBuiltKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr BuildingsBuiltKey None() { 
        return BuildingsBuiltKey(0); 
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr BuildingsBuiltKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // BUILDINGS_BUILT_ARRAY_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
