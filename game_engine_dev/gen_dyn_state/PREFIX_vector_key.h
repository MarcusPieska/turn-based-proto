//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef [MACRO_PREFIX]_VECTOR_KEY_H
#define [MACRO_PREFIX]_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - [CLASS_NAME_PREFIX]Key class -
//================================================================================================================================

class [CLASS_NAME_PREFIX]Key {
    friend class [CLASS_NAME_PREFIX]Vector;
public:
    constexpr [CLASS_NAME_PREFIX]Key() : m_val(0) {}

    constexpr bool operator==([CLASS_NAME_PREFIX]Key other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=([CLASS_NAME_PREFIX]Key other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr [CLASS_NAME_PREFIX]Key None() { 
        return [CLASS_NAME_PREFIX]Key(0); 
    }

    static constexpr [CLASS_NAME_PREFIX]Key from_raw(u16 val) {
        return [CLASS_NAME_PREFIX]Key(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr [CLASS_NAME_PREFIX]Key(u16 val) : 
        m_val(val) {
    }    
};

#endif // [MACRO_PREFIX]_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
