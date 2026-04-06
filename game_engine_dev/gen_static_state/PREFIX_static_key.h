//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef [MACRO_PREFIX]_STATIC_KEY_H
#define [MACRO_PREFIX]_STATIC_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - [DATA_KEY] class -
//================================================================================================================================

class [DATA_KEY] {
    friend class [CLASS_NAME];
public:
    constexpr [DATA_KEY]() : m_val(0) {}

    constexpr bool operator==([DATA_KEY] other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=([DATA_KEY] other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr [DATA_KEY] None() { 
        return [DATA_KEY](0); 
    }

    static constexpr [DATA_KEY] from_raw(u16 val) {
        return [DATA_KEY](val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr [DATA_KEY](u16 val) : 
        m_val(val) {
    }    
};

#endif // [MACRO_PREFIX]_STATIC_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
