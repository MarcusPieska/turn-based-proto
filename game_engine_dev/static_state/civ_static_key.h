//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_STATIC_KEY_H
#define CIV_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - CivStaticDataKey class -
//================================================================================================================================

class CivStaticDataKey {
    friend class CivStaticData;
public:
    constexpr CivStaticDataKey() : m_val(0) {}

    constexpr bool operator==(CivStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(CivStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr CivStaticDataKey None() { 
        return CivStaticDataKey(0); 
    }

    static constexpr CivStaticDataKey from_raw(u16 val) {
        return CivStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr CivStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // CIV_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
