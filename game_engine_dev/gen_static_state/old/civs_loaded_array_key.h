//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIVS_LOADED_ARRAY_KEY_H
#define CIVS_LOADED_ARRAY_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - CivsLoadedKey class -
//================================================================================================================================

class CivsLoadedKey {
    friend class CivsLoadedArray;
    friend class CivsLoadedAccessor;
public:
    constexpr CivsLoadedKey() : m_val(0) {}

    constexpr bool operator==(CivsLoadedKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(CivsLoadedKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr CivsLoadedKey None() { 
        return CivsLoadedKey(0); 
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr CivsLoadedKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // CIVS_LOADED_ARRAY_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
