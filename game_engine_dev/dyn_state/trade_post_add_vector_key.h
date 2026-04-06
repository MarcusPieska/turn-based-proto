//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TRADE_POST_ADD_VECTOR_KEY_H
#define TRADE_POST_ADD_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - TradePostAddKey class -
//================================================================================================================================

class TradePostAddKey {
    friend class TradePostAddVector;
public:
    constexpr TradePostAddKey() : m_val(0) {}

    constexpr bool operator==(TradePostAddKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(TradePostAddKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr TradePostAddKey None() { 
        return TradePostAddKey(0); 
    }

    static constexpr TradePostAddKey from_raw(u16 val) {
        return TradePostAddKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr TradePostAddKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // TRADE_POST_ADD_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
