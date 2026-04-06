//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_STATIC_KEY_H
#define BUILDING_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - BuildingStaticDataKey class -
//================================================================================================================================

class BuildingStaticDataKey {
    friend class BuildingStaticData;
public:
    constexpr BuildingStaticDataKey() : m_val(0) {}

    constexpr bool operator==(BuildingStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(BuildingStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr BuildingStaticDataKey None() { 
        return BuildingStaticDataKey(0); 
    }

    static constexpr BuildingStaticDataKey from_raw(u16 val) {
        return BuildingStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr BuildingStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // BUILDING_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
