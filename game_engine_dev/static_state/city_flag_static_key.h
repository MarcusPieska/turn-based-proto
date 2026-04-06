//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_FLAG_STATIC_KEY_H
#define CITY_FLAG_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - CityFlagStaticDataKey class -
//================================================================================================================================

class CityFlagStaticDataKey {
    friend class CityFlagStaticData;
public:
    constexpr CityFlagStaticDataKey() : m_val(0) {}

    constexpr bool operator==(CityFlagStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(CityFlagStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr CityFlagStaticDataKey None() { 
        return CityFlagStaticDataKey(0); 
    }

    static constexpr CityFlagStaticDataKey from_raw(u16 val) {
        return CityFlagStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr CityFlagStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // CITY_FLAG_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
