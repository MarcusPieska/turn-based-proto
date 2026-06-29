//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_bit_overlay.h"
#include "runtime_trace_dbg.h"

#include <cstring>

//================================================================================================================================
//=> - MapBitOverlay -
//================================================================================================================================

MapBitOverlay::MapBitOverlay (u16 w, u16 h) :
    m_w(0),
    m_h(0),
    m_bits(nullptr),
    m_bytes(0) {
    if (w == 0 || h == 0) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 bytes = (n + 7u) / 8u;
    u8* bits = new u8[bytes];
    if (bits == nullptr) {
        return;
    }
    std::memset(bits, 0, static_cast<size_t>(bytes));
    m_w = w;
    m_h = h;
    m_bits = bits;
    m_bytes = bytes;
}

MapBitOverlay::~MapBitOverlay () {
    clear();
}

void MapBitOverlay::clear () {
    delete[] m_bits;
    m_bits = nullptr;
    m_w = 0;
    m_h = 0;
    m_bytes = 0;
}

u16 MapBitOverlay::width () const {
    return m_w;
}

u16 MapBitOverlay::height () const {
    return m_h;
}

u32 MapBitOverlay::tidx (u16 x, u16 y) const {
    return static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x);
}

u8 MapBitOverlay::rd_byte (u32 bi) const {
    if (bi >= m_bytes || m_bits == nullptr) {
        return 0;
    }
    return m_bits[bi];
}

u32 MapBitOverlay::byte_count () const {
    return m_bytes;
}

u32 MapBitOverlay::get (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    const u32 i = tidx(x, y);
    const u32 bi = i / 8u;
    const u32 sh = i % 8u;
    return (m_bits[bi] >> sh) & 1u;
}

bool MapBitOverlay::set (u16 x, u16 y) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    const u32 i = tidx(x, y);
    const u32 bi = i / 8u;
    const u32 sh = i % 8u;
    m_bits[bi] = static_cast<u8>(m_bits[bi] | static_cast<u8>(1u << sh));
    return true;
}

bool MapBitOverlay::clr (u16 x, u16 y) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    const u32 i = tidx(x, y);
    const u32 bi = i / 8u;
    const u32 sh = i % 8u;
    m_bits[bi] = static_cast<u8>(m_bits[bi] & static_cast<u8>(~static_cast<u8>(1u << sh)));
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
