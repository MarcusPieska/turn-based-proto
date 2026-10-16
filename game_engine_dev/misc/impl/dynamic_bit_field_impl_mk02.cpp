//================================================================================================================================
//=> - DynamicBitField mk02 (packed bitfield) -
//================================================================================================================================

static const DynamicBitFieldLay* s_lay = nullptr;

static u32 s_mask (u8 n) {
    if (n >= 32) {
        return 0xFFFFFFFFu;
    }
    return (1u << n) - 1u;
}

static u8 s_rd (const u8* bits, u16 bit_off, u8 n) {
    if (n == 0) {
        return 0;
    }
    const u32 byte_off = SC_U32(bit_off / 8u);
    const u32 shift = SC_U32(bit_off % 8u);
    const u32 mask = s_mask(n);
    const u32 span = shift + SC_U32(n);
    if (span <= 8u) {
        return SC_U8((SC_U32(bits[byte_off]) >> shift) & mask);
    }
    const u32 v = SC_U32(bits[byte_off]) | (SC_U32(bits[byte_off + 1u]) << 8u);
    return SC_U8((v >> shift) & mask);
}

static void s_wr (u8* bits, u16 bit_off, u8 n, u8 val) {
    if (n == 0) {
        return;
    }
    const u32 byte_off = SC_U32(bit_off / 8u);
    const u32 shift = SC_U32(bit_off % 8u);
    const u32 mask = s_mask(n);
    const u32 vmasked = SC_U32(val) & mask;
    const u32 span = shift + SC_U32(n);
    if (span <= 8u) {
        u8 b = bits[byte_off];
        b = SC_U8((SC_U32(b) & ~(mask << shift)) | (vmasked << shift));
        bits[byte_off] = b;
        return;
    }
    u32 v = SC_U32(bits[byte_off]) | (SC_U32(bits[byte_off + 1u]) << 8u);
    v = (v & ~(mask << shift)) | (vmasked << shift);
    bits[byte_off] = SC_U8(v & 0xFFu);
    bits[byte_off + 1u] = SC_U8((v >> 8u) & 0xFFu);
}

DynamicBitField::DynamicBitField (const DynamicBitFieldLay* lay, GeneralBitBank* bank) :
    m_data(nullptr),
    m_ix(U16_KEY_NULL) {
    (void)bank;
    s_lay = lay;
    if (lay == nullptr || lay->m_n == 0 || lay->m_vals == nullptr || lay->m_slots == nullptr || lay->m_bytes == 0) {
        s_lay = nullptr;
        return;
    }
    m_data = new u8[lay->m_bytes];
    if (m_data == nullptr) {
        s_lay = nullptr;
        return;
    }
    for (u32 i = 0; i < lay->m_bytes; ++i) {
        m_data[i] = 0;
    }
}

DynamicBitField::~DynamicBitField () {
    delete[] m_data;
    m_data = nullptr;
}

u16 DynamicBitField::n () const {
    if (s_lay == nullptr) {
        return 0;
    }
    return s_lay->m_n;
}

u8 DynamicBitField::vals (u16 i) const {
    if (s_lay == nullptr || i >= s_lay->m_n) {
        return 0;
    }
    return s_lay->m_vals[i];
}

u32 DynamicBitField::bytes () const {
    if (s_lay == nullptr) {
        return 0;
    }
    return s_lay->m_bytes;
}

u16 DynamicBitField::ix () const {
    return m_ix;
}

u8 DynamicBitField::get (u16 i) const {
    if (s_lay == nullptr || m_data == nullptr || i >= s_lay->m_n) {
        return 0;
    }
    const DynamicBitFieldSlot& s = s_lay->m_slots[i];
    return s_rd(m_data, s.m_off, s.m_wid);
}

void DynamicBitField::set (u16 i, u8 v) {
    if (s_lay == nullptr || m_data == nullptr || i >= s_lay->m_n) {
        return;
    }
    const DynamicBitFieldSlot& s = s_lay->m_slots[i];
    s_wr(m_data, s.m_off, s.m_wid, v);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
