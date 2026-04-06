//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WONDER_STATIC_KEY_H
#define WONDER_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - WonderStaticDataKey class -
//================================================================================================================================

class WonderStaticDataKey {
    friend class WonderStaticData;
public:
    constexpr WonderStaticDataKey() : m_val(0) {}

    constexpr bool operator==(WonderStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(WonderStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr WonderStaticDataKey None() { 
        return WonderStaticDataKey(0); 
    }

    static constexpr WonderStaticDataKey from_raw(u16 val) {
        return WonderStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr WonderStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // WONDER_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
