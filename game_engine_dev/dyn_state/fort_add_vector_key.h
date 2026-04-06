//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef FORT_ADD_VECTOR_KEY_H
#define FORT_ADD_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - FortAddKey class -
//================================================================================================================================

class FortAddKey {
    friend class FortAddVector;
public:
    constexpr FortAddKey() : m_val(0) {}

    constexpr bool operator==(FortAddKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(FortAddKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr FortAddKey None() { 
        return FortAddKey(0); 
    }

    static constexpr FortAddKey from_raw(u16 val) {
        return FortAddKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr FortAddKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // FORT_ADD_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
