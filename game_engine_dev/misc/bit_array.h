//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BIT_ARRAY_H
#define BIT_ARRAY_H

#include <cstdint>
#include <iosfwd>

#include "game_primitives.h"

//================================================================================================================================
//=> - BitArray32 class -
//================================================================================================================================

class BitArray32 {
public:

    BitArray32 () : m_num_bits(0x00000000) {}
    BitArray32 (u32 num_bits) : m_num_bits(num_bits) {}

    u32 get_count () const;
    u32 get_bit (u32 index) const;
    void set_bit (u32 index);
    void clear_bit (u32 index);
    
    void serialize (std::ostream& os) const;
    static BitArray32 deserialize (std::istream& is);
    
    friend class Tester_BitArray32;

private:
    static const u32 masks[32];
    
    u32 m_num_bits;
};

//================================================================================================================================
//=> - BitArrayCL class -
//================================================================================================================================

class BitArrayCL {
public:

    BitArrayCL (u32 num_bits);
    ~BitArrayCL ();

    u32 get_count () const;
    u32 get_bit (u32 index) const;
    void set_bit (u32 index);
    void clear_bit (u32 index);

    void serialize (std::ostream& os) const;
    static BitArrayCL* deserialize (std::istream& is);

private:
    BitArray32* m_arrays;
    u32 m_num_bits;
    u32 m_num_arrays;
};

#endif // BIT_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
