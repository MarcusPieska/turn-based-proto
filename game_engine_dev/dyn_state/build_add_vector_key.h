//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILD_ADD_VECTOR_KEY_H
#define BUILD_ADD_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - BuildAddKey class -
//================================================================================================================================

class BuildAddKey {
    friend class BuildAddVector;
public:
    constexpr BuildAddKey() : m_val(0) {}

    constexpr bool operator==(BuildAddKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(BuildAddKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr BuildAddKey None() { 
        return BuildAddKey(0); 
    }

    static constexpr BuildAddKey from_raw(u16 val) {
        return BuildAddKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr BuildAddKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // BUILD_ADD_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
