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

static u32 general_slice_mask (u8 width) {
    if (width >= 32) {
        return 0xFFFFFFFFu;
    }
    return (1u << width) - 1u;
}

u32 GeneralBitBank::get_slice (u16 batch_idx, u16 bit_off, u8 width) const {
    if (width == 0 || width > 32) {
        return 0;
    }
    if (static_cast<u32>(bit_off) + static_cast<u32>(width) > static_cast<u32>(m_batch_size)) {
        return 0;
    }
    u16 page_idx = static_cast<u16>(batch_idx / GENERAL_BATCHES_PER_PAGE);
    if (m_pages[page_idx] == nullptr) {
        return 0;
    }
    const u8* bits = m_pages[page_idx];
    const u32 abs_off = general_to_bit_offset(batch_idx, bit_off, m_batch_size, GENERAL_BATCHES_PER_PAGE);
    const u32 byte_off = abs_off >> 3;
    const u32 shift = abs_off & 0x7u;
    const u32 mask = general_slice_mask(width);
    const u32 span = shift + static_cast<u32>(width);
    if (span <= 8u) {
        return (static_cast<u32>(bits[byte_off]) >> shift) & mask;
    }
    if (span <= 16u) {
        const u32 v = static_cast<u32>(bits[byte_off])
            | (static_cast<u32>(bits[byte_off + 1u]) << 8u);
        return (v >> shift) & mask;
    }
    if (span <= 24u) {
        const u32 v = static_cast<u32>(bits[byte_off])
            | (static_cast<u32>(bits[byte_off + 1u]) << 8u)
            | (static_cast<u32>(bits[byte_off + 2u]) << 16u);
        return (v >> shift) & mask;
    }
    const u32 v = static_cast<u32>(bits[byte_off])
        | (static_cast<u32>(bits[byte_off + 1u]) << 8u)
        | (static_cast<u32>(bits[byte_off + 2u]) << 16u)
        | (static_cast<u32>(bits[byte_off + 3u]) << 24u);
    return (v >> shift) & mask;
}

void GeneralBitBank::set_slice (u16 batch_idx, u16 bit_off, u8 width, u32 val) {
    if (width == 0 || width > 32) {
        return;
    }
    if (static_cast<u32>(bit_off) + static_cast<u32>(width) > static_cast<u32>(m_batch_size)) {
        return;
    }
    u16 page_idx = static_cast<u16>(batch_idx / GENERAL_BATCHES_PER_PAGE);
    if (m_pages[page_idx] == nullptr) {
        return;
    }
    u8* bits = m_pages[page_idx];
    const u32 abs_off = general_to_bit_offset(batch_idx, bit_off, m_batch_size, GENERAL_BATCHES_PER_PAGE);
    const u32 byte_off = abs_off >> 3;
    const u32 shift = abs_off & 0x7u;
    const u32 mask = general_slice_mask(width);
    const u32 vmasked = val & mask;
    const u32 span = shift + static_cast<u32>(width);
    if (span <= 8u) {
        u8 b = bits[byte_off];
        b = static_cast<u8>((static_cast<u32>(b) & ~(mask << shift)) | (vmasked << shift));
        bits[byte_off] = b;
        return;
    }
    if (span <= 16u) {
        u32 v = static_cast<u32>(bits[byte_off])
            | (static_cast<u32>(bits[byte_off + 1u]) << 8u);
        v = (v & ~(mask << shift)) | (vmasked << shift);
        bits[byte_off] = static_cast<u8>(v & 0xFFu);
        bits[byte_off + 1u] = static_cast<u8>((v >> 8u) & 0xFFu);
        return;
    }
    if (span <= 24u) {
        u32 v = static_cast<u32>(bits[byte_off])
            | (static_cast<u32>(bits[byte_off + 1u]) << 8u)
            | (static_cast<u32>(bits[byte_off + 2u]) << 16u);
        v = (v & ~(mask << shift)) | (vmasked << shift);
        bits[byte_off] = static_cast<u8>(v & 0xFFu);
        bits[byte_off + 1u] = static_cast<u8>((v >> 8u) & 0xFFu);
        bits[byte_off + 2u] = static_cast<u8>((v >> 16u) & 0xFFu);
        return;
    }
    u32 v = static_cast<u32>(bits[byte_off])
        | (static_cast<u32>(bits[byte_off + 1u]) << 8u)
        | (static_cast<u32>(bits[byte_off + 2u]) << 16u)
        | (static_cast<u32>(bits[byte_off + 3u]) << 24u);
    v = (v & ~(mask << shift)) | (vmasked << shift);
    bits[byte_off] = static_cast<u8>(v & 0xFFu);
    bits[byte_off + 1u] = static_cast<u8>((v >> 8u) & 0xFFu);
    bits[byte_off + 2u] = static_cast<u8>((v >> 16u) & 0xFFu);
    bits[byte_off + 3u] = static_cast<u8>((v >> 24u) & 0xFFu);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

