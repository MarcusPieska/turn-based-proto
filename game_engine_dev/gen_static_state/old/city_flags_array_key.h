//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_FLAGS_ARRAY_KEY_H
#define CITY_FLAGS_ARRAY_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - CityFlagsKey class -
//================================================================================================================================

class CityFlagsKey {
    friend class CityFlagsArray;
    friend class CityFlagsAccessor;
public:
    constexpr CityFlagsKey() : m_val(0) {}

    constexpr bool operator==(CityFlagsKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(CityFlagsKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr CityFlagsKey None() { 
        return CityFlagsKey(0); 
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr CityFlagsKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // CITY_FLAGS_ARRAY_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
