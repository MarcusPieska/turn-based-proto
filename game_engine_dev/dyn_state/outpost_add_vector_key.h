//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef OUTPOST_ADD_VECTOR_KEY_H
#define OUTPOST_ADD_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - OutpostAddKey class -
//================================================================================================================================

class OutpostAddKey {
    friend class OutpostAddVector;
public:
    constexpr OutpostAddKey() : m_val(0) {}

    constexpr bool operator==(OutpostAddKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(OutpostAddKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr OutpostAddKey None() { 
        return OutpostAddKey(0); 
    }

    static constexpr OutpostAddKey from_raw(u16 val) {
        return OutpostAddKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr OutpostAddKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // OUTPOST_ADD_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
