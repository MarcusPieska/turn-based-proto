//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SMALL_WONDER_STATIC_KEY_H
#define SMALL_WONDER_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - SmallWonderStaticDataKey class -
//================================================================================================================================

class SmallWonderStaticDataKey {
    friend class SmallWonderStaticData;
public:
    constexpr SmallWonderStaticDataKey() : m_val(0) {}

    constexpr bool operator==(SmallWonderStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(SmallWonderStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr SmallWonderStaticDataKey None() { 
        return SmallWonderStaticDataKey(0); 
    }

    static constexpr SmallWonderStaticDataKey from_raw(u16 val) {
        return SmallWonderStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr SmallWonderStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // SMALL_WONDER_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
