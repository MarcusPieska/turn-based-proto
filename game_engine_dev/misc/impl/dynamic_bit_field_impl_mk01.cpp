//================================================================================================================================
//=> - DynamicBitField mk01 (one byte per field) -
//================================================================================================================================

static const DynamicBitFieldLay* s_lay = nullptr;

DynamicBitField::DynamicBitField (const DynamicBitFieldLay* lay, GeneralBitBank* bank) :
    m_data(nullptr),
    m_ix(U16_KEY_NULL) {
    (void)bank;
    s_lay = lay;
    if (lay == nullptr || lay->m_n == 0 || lay->m_vals == nullptr || lay->m_bytes == 0) {
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
    return m_data[i];
}

void DynamicBitField::set (u16 i, u8 v) {
    if (s_lay == nullptr || m_data == nullptr || i >= s_lay->m_n) {
        return;
    }
    m_data[i] = v;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
