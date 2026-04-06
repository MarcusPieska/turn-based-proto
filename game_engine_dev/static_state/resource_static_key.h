//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCE_STATIC_KEY_H
#define RESOURCE_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - ResourceStaticDataKey class -
//================================================================================================================================

class ResourceStaticDataKey {
    friend class ResourceStaticData;
public:
    constexpr ResourceStaticDataKey() : m_val(0) {}

    constexpr bool operator==(ResourceStaticDataKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(ResourceStaticDataKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr ResourceStaticDataKey None() { 
        return ResourceStaticDataKey(0); 
    }

    static constexpr ResourceStaticDataKey from_raw(u16 val) {
        return ResourceStaticDataKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr ResourceStaticDataKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // RESOURCE_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
