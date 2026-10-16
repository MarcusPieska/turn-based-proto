//================================================================================================================================
//=> - DynamicBitField mk03 (GeneralBitBank batches) -
//================================================================================================================================

#include "general_bit_bank.h"

static const DynamicBitFieldLay* s_lay = nullptr;
static GeneralBitBank* s_bank = nullptr;

DynamicBitField::DynamicBitField (const DynamicBitFieldLay* lay, GeneralBitBank* bank) :
    m_data(nullptr),
    m_ix(U16_KEY_NULL) {
    s_lay = lay;
    s_bank = bank;
    if (lay == nullptr || bank == nullptr || lay->m_n == 0 || lay->m_vals == nullptr || lay->m_slots == nullptr ||
        lay->m_bits == 0) {
        s_lay = nullptr;
        s_bank = nullptr;
        return;
    }
    m_ix = bank->claim_batch();
}

DynamicBitField::~DynamicBitField () {
    m_ix = U16_KEY_NULL;
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
    if (s_lay == nullptr || s_bank == nullptr || m_ix == U16_KEY_NULL || i >= s_lay->m_n) {
        return 0;
    }
    const DynamicBitFieldSlot& s = s_lay->m_slots[i];
    return SC_U8(s_bank->get_slice(m_ix, s.m_off, s.m_wid));
}

void DynamicBitField::set (u16 i, u8 v) {
    if (s_lay == nullptr || s_bank == nullptr || m_ix == U16_KEY_NULL || i >= s_lay->m_n) {
        return;
    }
    const DynamicBitFieldSlot& s = s_lay->m_slots[i];
    s_bank->set_slice(m_ix, s.m_off, s.m_wid, SC_U32(v));
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
