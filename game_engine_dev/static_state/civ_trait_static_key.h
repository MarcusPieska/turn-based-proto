//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_TRAIT_STATIC_KEY_H
#define CIV_TRAIT_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - CivTraitStaticDataKey class -
//================================================================================================================================

class CivTraitStaticDataKey {
    friend class CivTraitStaticData;
public:
    constexpr CivTraitStaticDataKey() : m_val(0) {}

    constexpr bool operator==(CivTraitStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(CivTraitStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr CivTraitStaticDataKey None() { 
        return CivTraitStaticDataKey(0); 
    }

    static constexpr CivTraitStaticDataKey from_raw(u16 val) {
        return CivTraitStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr CivTraitStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // CIV_TRAIT_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
