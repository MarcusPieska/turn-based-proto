//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "static_bit_bank.h"

//================================================================================================================================
//=> - Global constants -
//================================================================================================================================

static const u8 MAX_BITS = 8;
static const u8 static_masks[MAX_BITS] = {
    0x01, 0x02, 0x04, 0x08,
    0x10, 0x20, 0x40, 0x80
};

static u32 static_to_bit_offset (u16 array_idx, u16 flag_idx, u16 array_size) {
    return (static_cast<u32>(array_idx) * static_cast<u32>(array_size)) + static_cast<u32>(flag_idx);
}

//================================================================================================================================
//=> - StaticBitBank implementation -
//================================================================================================================================

StaticBitBank::StaticBitBank (u16 array_count, u16 array_size) :
    m_bits(nullptr),
    m_array_count(array_count),
    m_array_size(array_size),
    m_bit_count(static_cast<u32>(array_count) * static_cast<u32>(array_size)),
    m_byte_count((m_bit_count + static_cast<u32>(MAX_BITS - 1)) / static_cast<u32>(MAX_BITS)) {
    m_bits = new u8[m_byte_count];
    for (u32 i = 0; i < m_byte_count; ++i) {
        m_bits[i] = 0;
    }
}

StaticBitBank::~StaticBitBank () {
    delete[] m_bits;
    m_bits = nullptr;
}

bool StaticBitBank::is_flagged (u16 array_idx, u16 flag_idx) const {
    u32 bit_offset = static_to_bit_offset(array_idx, flag_idx, m_array_size);
    u32 byte_offset = bit_offset >> 3;
    u8 bit_idx = static_cast<u8>(bit_offset & 0x7);
    return (m_bits[byte_offset] & static_masks[bit_idx]) != 0;
}

void StaticBitBank::set_flag (u16 array_idx, u16 flag_idx) {
    u32 bit_offset = static_to_bit_offset(array_idx, flag_idx, m_array_size);
    u32 byte_offset = bit_offset >> 3;
    u8 bit_idx = static_cast<u8>(bit_offset & 0x7);
    m_bits[byte_offset] = static_cast<u8>(m_bits[byte_offset] | static_masks[bit_idx]);
}

void StaticBitBank::clear_flag (u16 array_idx, u16 flag_idx) {
    u32 bit_offset = static_to_bit_offset(array_idx, flag_idx, m_array_size);
    u32 byte_offset = bit_offset >> 3;
    u8 bit_idx = static_cast<u8>(bit_offset & 0x7);
    m_bits[byte_offset] = static_cast<u8>(m_bits[byte_offset] & static_cast<u8>(~static_masks[bit_idx]));
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
