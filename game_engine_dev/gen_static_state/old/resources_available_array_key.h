//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCES_AVAILABLE_ARRAY_KEY_H
#define RESOURCES_AVAILABLE_ARRAY_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - ResourcesAvailableKey class -
//================================================================================================================================

class ResourcesAvailableKey {
    friend class ResourcesAvailableArray;
    friend class ResourcesAvailableAccessor;
public:
    constexpr ResourcesAvailableKey() : m_val(0) {}

    constexpr bool operator==(ResourcesAvailableKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(ResourcesAvailableKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr ResourcesAvailableKey None() { 
        return ResourcesAvailableKey(0); 
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr ResourcesAvailableKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // RESOURCES_AVAILABLE_ARRAY_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
