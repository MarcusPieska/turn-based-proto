//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "general_bit_bank.h"

//================================================================================================================================
//=> - Global constants -
//================================================================================================================================

static const u8 MAX_BITS = 8;
static const u8 general_masks[MAX_BITS] = {
    0x01, 0x02, 0x04, 0x08,
    0x10, 0x20, 0x40, 0x80
};

static u32 general_to_bit_offset (u16 batch_idx, u16 flag_idx, u16 batch_size, u16 batches_per_page) {
    u32 batch_in_page_idx = static_cast<u32>(batch_idx % batches_per_page);
    return (batch_in_page_idx * static_cast<u32>(batch_size)) + static_cast<u32>(flag_idx);
}

static u32 general_get_page_byte_count (u16 batch_size, u16 batches_per_page) {
    u32 page_bit_count = static_cast<u32>(batch_size) * static_cast<u32>(batches_per_page);
    return (page_bit_count + static_cast<u32>(MAX_BITS - 1)) / static_cast<u32>(MAX_BITS);
}

//================================================================================================================================
//=> - GeneralBitBank implementation -
//================================================================================================================================

GeneralBitBank::GeneralBitBank (u16 batch_size) :
    m_batch_size(batch_size),
    m_claimed_batch_count(0),
    m_allocated_page_count(0) {
    for (u16 i = 0; i < MAX_PAGES; ++i) {
        m_pages[i] = nullptr;
    }
}

GeneralBitBank::~GeneralBitBank () {
    for (u16 i = 0; i < MAX_PAGES; ++i) {
        if (m_pages[i] != nullptr) {
            delete[] m_pages[i];
            m_pages[i] = nullptr;
        }
    }
}

u16 GeneralBitBank::claim_batch () {
    u16 batch_idx = m_claimed_batch_count;
    u16 page_idx = static_cast<u16>(batch_idx / GENERAL_BATCHES_PER_PAGE);

    if (m_pages[page_idx] == nullptr) {
        u32 page_byte_count = general_get_page_byte_count(m_batch_size, GENERAL_BATCHES_PER_PAGE);
        m_pages[page_idx] = new u8[page_byte_count];
        for (u32 i = 0; i < page_byte_count; ++i) {
            m_pages[page_idx][i] = 0;
        }
        m_allocated_page_count = static_cast<u8>(m_allocated_page_count + 1);
    }

    m_claimed_batch_count = static_cast<u16>(m_claimed_batch_count + 1);
    return batch_idx;
}

bool GeneralBitBank::is_flagged (u16 batch_idx, u16 flag_idx) const {
    u16 page_idx = static_cast<u16>(batch_idx / GENERAL_BATCHES_PER_PAGE);
    u32 bit_offset = general_to_bit_offset(batch_idx, flag_idx, m_batch_size, GENERAL_BATCHES_PER_PAGE);
    u32 byte_offset = bit_offset >> 3;
    u8 bit_idx = static_cast<u8>(bit_offset & 0x7);
    return (m_pages[page_idx][byte_offset] & general_masks[bit_idx]) != 0;
}

void GeneralBitBank::set_flag (u16 batch_idx, u16 flag_idx) {
    u16 page_idx = static_cast<u16>(batch_idx / GENERAL_BATCHES_PER_PAGE);
    u32 bit_offset = general_to_bit_offset(batch_idx, flag_idx, m_batch_size, GENERAL_BATCHES_PER_PAGE);
    u32 byte_offset = bit_offset >> 3;
    u8 bit_idx = static_cast<u8>(bit_offset & 0x7);
    m_pages[page_idx][byte_offset] = static_cast<u8>(m_pages[page_idx][byte_offset] | general_masks[bit_idx]);
}

void GeneralBitBank::clear_flag (u16 batch_idx, u16 flag_idx) {
    u16 page_idx = static_cast<u16>(batch_idx / GENERAL_BATCHES_PER_PAGE);
    u32 bit_offset = general_to_bit_offset(batch_idx, flag_idx, m_batch_size, GENERAL_BATCHES_PER_PAGE);
    u32 byte_offset = bit_offset >> 3;
    u8 bit_idx = static_cast<u8>(bit_offset & 0x7);
    m_pages[page_idx][byte_offset] = static_cast<u8>(m_pages[page_idx][byte_offset] & static_cast<u8>(~general_masks[bit_idx]));
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
