//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "bit_array.h"
#include <iostream>

//================================================================================================================================
//=> - Global constants -
//================================================================================================================================

const int MAX_BITS = 32;

constexpr uint32_t BitArray32::masks[MAX_BITS] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
};

//================================================================================================================================
//=> - BitArray32 implementation -
//================================================================================================================================

int BitArray32::get_count () const {
    return m_num_bits;
}

int BitArray32::get_bit (int index) const {
    if (index < 0 || index >= MAX_BITS) {
        return 0;
    }
    return (m_num_bits & masks[index]) ? 1 : 0;
}

void BitArray32::set_bit (int index) {
    if (index < 0 || index >= MAX_BITS) {
        return;
    }
    m_num_bits |= masks[index];
}

void BitArray32::clear_bit (int index) {
    if (index < 0 || index >= MAX_BITS) {
        return;
    }
    m_num_bits &= ~masks[index];
}

void BitArray32::serialize (std::ostream& os) const {
    os.write(reinterpret_cast<const char*>(&m_num_bits), sizeof(uint32_t));
}

BitArray32 BitArray32::deserialize (std::istream& is) {
    uint32_t bits;
    is.read(reinterpret_cast<char*>(&bits), sizeof(uint32_t));
    return BitArray32(bits);
}

//================================================================================================================================
//=> - BitArrayCL implementation -
//================================================================================================================================

BitArrayCL::BitArrayCL (uint32_t num_bits) : m_num_bits(num_bits) {
    m_num_arrays = (num_bits + MAX_BITS - 1) / MAX_BITS;
    m_arrays = new BitArray32[m_num_arrays];
}

BitArrayCL::~BitArrayCL () {
    delete[] m_arrays;
}

int BitArrayCL::get_count () const {
    return m_num_bits;
}


int BitArrayCL::get_bit (int index) const {
    if (index < 0 || index >= static_cast<int>(m_num_bits)) {
        return 0;
    }
    int array_idx = index / MAX_BITS;
    int bit_idx = index % MAX_BITS;
    return m_arrays[array_idx].get_bit(bit_idx);
}

void BitArrayCL::set_bit (int index) {
    if (index < 0 || index >= static_cast<int>(m_num_bits)) {
        return;
    }
    int array_idx = index / MAX_BITS;
    int bit_idx = index % MAX_BITS;
    m_arrays[array_idx].set_bit(bit_idx);
}

void BitArrayCL::clear_bit (int index) {
    if (index < 0 || index >= static_cast<int>(m_num_bits)) {
        return;
    }
    int array_idx = index / MAX_BITS;
    int bit_idx = index % MAX_BITS;
    m_arrays[array_idx].clear_bit(bit_idx);
}

void BitArrayCL::serialize (std::ostream& os) const {
    os.write(reinterpret_cast<const char*>(&m_num_bits), sizeof(uint32_t));
    for (uint32_t i = 0; i < m_num_arrays; i++) {
        m_arrays[i].serialize(os);
    }
}

BitArrayCL* BitArrayCL::deserialize (std::istream& is) {
    uint32_t num_bits;
    is.read(reinterpret_cast<char*>(&num_bits), sizeof(uint32_t));
    BitArrayCL* ba = new BitArrayCL(num_bits);
    for (uint32_t i = 0; i < ba->m_num_arrays; i++) {
        ba->m_arrays[i] = BitArray32::deserialize(is);
    }
    return ba;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
