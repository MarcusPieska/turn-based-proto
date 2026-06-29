//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_bit_array_overlay.h"
#include "runtime_trace_dbg.h"

#include <cstring>

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 bit_mask (u32 n) {
    if (n >= 32u) {
        return 0xFFFFFFFFu;
    }
    return (1u << n) - 1u;
}

static u32 rd_bits (const u8* bits, u32 bit_off, u32 n) {
    const u32 byte_off = bit_off / 8u;
    const u32 shift = bit_off % 8u;
    const u32 mask = bit_mask(n);
    const u32 span = shift + n;
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

static void wr_bits (u8* bits, u32 bit_off, u32 n, u32 val) {
    const u32 byte_off = bit_off / 8u;
    const u32 shift = bit_off % 8u;
    const u32 mask = bit_mask(n);
    val &= mask;
    const u32 span = shift + n;
    if (span <= 8u) {
        const u32 lo = mask;
        u8 b = bits[byte_off];
        b = static_cast<u8>((b & static_cast<u8>(~(lo << shift))) | static_cast<u8>((val & lo) << shift));
        bits[byte_off] = b;
        return;
    }
    if (span <= 16u) {
        u32 v = static_cast<u32>(bits[byte_off])
            | (static_cast<u32>(bits[byte_off + 1u]) << 8u);
        const u32 clr_mask = ~(mask << shift);
        v = (v & clr_mask) | ((val & mask) << shift);
        bits[byte_off] = static_cast<u8>(v & 0xFFu);
        bits[byte_off + 1u] = static_cast<u8>((v >> 8u) & 0xFFu);
        return;
    }
    if (span <= 24u) {
        u32 v = static_cast<u32>(bits[byte_off])
            | (static_cast<u32>(bits[byte_off + 1u]) << 8u)
            | (static_cast<u32>(bits[byte_off + 2u]) << 16u);
        const u32 clr_mask = ~(mask << shift);
        v = (v & clr_mask) | ((val & mask) << shift);
        bits[byte_off] = static_cast<u8>(v & 0xFFu);
        bits[byte_off + 1u] = static_cast<u8>((v >> 8u) & 0xFFu);
        bits[byte_off + 2u] = static_cast<u8>((v >> 16u) & 0xFFu);
        return;
    }
    u32 v = static_cast<u32>(bits[byte_off])
        | (static_cast<u32>(bits[byte_off + 1u]) << 8u)
        | (static_cast<u32>(bits[byte_off + 2u]) << 16u)
        | (static_cast<u32>(bits[byte_off + 3u]) << 24u);
    const u32 clr_mask = ~(mask << shift);
    v = (v & clr_mask) | ((val & mask) << shift);
    bits[byte_off] = static_cast<u8>(v & 0xFFu);
    bits[byte_off + 1u] = static_cast<u8>((v >> 8u) & 0xFFu);
    bits[byte_off + 2u] = static_cast<u8>((v >> 16u) & 0xFFu);
    bits[byte_off + 3u] = static_cast<u8>((v >> 24u) & 0xFFu);
}

//================================================================================================================================
//=> - MapBitArrayOverlay -
//================================================================================================================================

MapBitArrayOverlay::MapBitArrayOverlay (u16 w, u16 h, u8 bits_per_val) :
    m_w(0),
    m_h(0),
    m_bpv(0),
    m_bits(nullptr),
    m_bytes(0) {
    if (w == 0 || h == 0 || bits_per_val == 0) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 total_bits = n * static_cast<u32>(bits_per_val);
    const u32 bytes = (total_bits + 7u) / 8u + 4u;
    u8* bits = new u8[bytes];
    if (bits == nullptr) {
        return;
    }
    std::memset(bits, 0, static_cast<size_t>(bytes));
    m_w = w;
    m_h = h;
    m_bpv = bits_per_val;
    m_bits = bits;
    m_bytes = bytes;
}

MapBitArrayOverlay::~MapBitArrayOverlay () {
    clear();
}

void MapBitArrayOverlay::clear () {
    delete[] m_bits;
    m_bits = nullptr;
    m_w = 0;
    m_h = 0;
    m_bpv = 0;
    m_bytes = 0;
}

u16 MapBitArrayOverlay::width () const {
    return m_w;
}

u16 MapBitArrayOverlay::height () const {
    return m_h;
}

u8 MapBitArrayOverlay::bits_per_val () const {
    return m_bpv;
}

u32 MapBitArrayOverlay::tidx (u16 x, u16 y) const {
    return static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x);
}

u8 MapBitArrayOverlay::rd_byte (u32 bi) const {
    if (bi >= m_bytes || m_bits == nullptr) {
        return 0;
    }
    return m_bits[bi];
}

u32 MapBitArrayOverlay::byte_count () const {
    return m_bytes;
}

u32 MapBitArrayOverlay::val_mask () const {
    if (m_bpv >= 32u) {
        return 0xFFFFFFFFu;
    }
    return (1u << static_cast<u32>(m_bpv)) - 1u;
}

u32 MapBitArrayOverlay::get (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    const u32 i = tidx(x, y);
    const u32 bit_off = i * static_cast<u32>(m_bpv);
    return rd_bits(m_bits, bit_off, static_cast<u32>(m_bpv));
}

bool MapBitArrayOverlay::set (u16 x, u16 y, u32 val) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    const u32 i = tidx(x, y);
    const u32 bit_off = i * static_cast<u32>(m_bpv);
    wr_bits(m_bits, bit_off, static_cast<u32>(m_bpv), val & val_mask());
    return true;
}

u32 MapBitArrayOverlay::max_val () const {
    return val_mask();
}

u32 MapBitArrayOverlay::get_uint (u16 x, u16 y) const {
    return get(x, y);
}

bool MapBitArrayOverlay::set_uint (u16 x, u16 y, u32 val) {
    return set(x, y, val);
}

void MapBitArrayOverlay::fill_all (u32 val) {
    if (m_bits == nullptr || m_w == 0 || m_h == 0) {
        return;
    }
    val &= val_mask();
    if (m_bpv == 4u && val == 0xFu) {
        std::memset(m_bits, 0xFF, static_cast<size_t>(m_bytes));
        return;
    }
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    for (u32 i = 0; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(m_w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(m_w));
        set(x, y, val);
    }
}

void MapBitArrayOverlay::assign_u8 (const u8* vals) {
    if (m_bits == nullptr || vals == nullptr || m_bpv != 4u || m_w == 0 || m_h == 0) {
        return;
    }
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    for (u32 i = 0; i < n; ++i) {
        const u32 bo = i >> 1;
        const u32 sh = (i & 1u) << 2;
        const u8 v = static_cast<u8>(vals[i] & 0xFu);
        m_bits[bo] = static_cast<u8>((m_bits[bo] & static_cast<u8>(~(0xFu << sh))) | static_cast<u8>(v << sh));
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
