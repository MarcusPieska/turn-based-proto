//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BIT_ARRAY_H
#define BIT_ARRAY_H

#include <cstdint>
#include <iosfwd>

//================================================================================================================================
//=> - BitArray32 class -
//================================================================================================================================

class BitArray32 {
public:

    BitArray32 () : bits(0x00000000) {}
    BitArray32 (uint32_t bits) : bits(bits) {}

    int get_bit (int index) const;
    void set_bit (int index);
    void clear_bit (int index);
    
    void serialize (std::ostream& os) const;
    static BitArray32 deserialize (std::istream& is);
    
    friend class Tester_BitArray32;

private:
    static const uint32_t masks[32];
    
    uint32_t bits;
};

//================================================================================================================================
//=> - BitArrayCL class -
//================================================================================================================================

class BitArrayCL {
public:

    BitArrayCL (uint32_t num_bits);
    ~BitArrayCL ();

    int get_bit (int index) const;
    void set_bit (int index);
    void clear_bit (int index);

    void serialize (std::ostream& os) const;
    static BitArrayCL* deserialize (std::istream& is);

private:
    BitArray32* arrays;
    uint32_t num_bits;
    uint32_t num_arrays;
};

#endif // BIT_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
