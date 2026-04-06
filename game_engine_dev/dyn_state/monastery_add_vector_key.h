//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MONASTERY_ADD_VECTOR_KEY_H
#define MONASTERY_ADD_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - MonasteryAddKey class -
//================================================================================================================================

class MonasteryAddKey {
    friend class MonasteryAddVector;
public:
    constexpr MonasteryAddKey() : m_val(0) {}

    constexpr bool operator==(MonasteryAddKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(MonasteryAddKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr MonasteryAddKey None() { 
        return MonasteryAddKey(0); 
    }

    static constexpr MonasteryAddKey from_raw(u16 val) {
        return MonasteryAddKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr MonasteryAddKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // MONASTERY_ADD_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
