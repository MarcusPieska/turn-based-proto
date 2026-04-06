//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MINE_ADD_VECTOR_KEY_H
#define MINE_ADD_VECTOR_KEY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - MineAddKey class -
//================================================================================================================================

class MineAddKey {
    friend class MineAddVector;
public:
    constexpr MineAddKey() : m_val(0) {}

    constexpr bool operator==(MineAddKey other) const {
        return m_val == other.m_val; 
    }

    constexpr bool operator!=(MineAddKey other) const { 
        return m_val != other.m_val; 
    }

    constexpr bool is_null() const { 
        return m_val == 0; 
    }

    constexpr bool is_valid() const { 
        return m_val != 0; 
    }

    static constexpr MineAddKey None() { 
        return MineAddKey(0); 
    }

    static constexpr MineAddKey from_raw(u16 val) {
        return MineAddKey(val);
    }

    constexpr u16 value() const {
        return m_val;
    }

private:
    u16 m_val;

    explicit constexpr MineAddKey(u16 val) : 
        m_val(val) {
    }    
};

#endif // MINE_ADD_VECTOR_KEY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
