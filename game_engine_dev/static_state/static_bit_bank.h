//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef STATIC_BIT_BANK_H
#define STATIC_BIT_BANK_H

#include "game_primitives.h"

//================================================================================================================================
//=> - StaticBitBank class -
//================================================================================================================================

class StaticBitBank {
public:
    StaticBitBank (u16 array_count, u16 array_size);
    ~StaticBitBank ();

    bool is_flagged (u16 array_idx, u16 flag_idx) const;
    void set_flag (u16 array_idx, u16 flag_idx);
    void clear_flag (u16 array_idx, u16 flag_idx);

private:
    StaticBitBank (const StaticBitBank& other) = delete;
    StaticBitBank (StaticBitBank&& other) = delete;

    u8* m_bits;
    u16 m_array_count;
    u16 m_array_size;
    u32 m_bit_count;
    u32 m_byte_count;
};

#endif // STATIC_BIT_BANK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
