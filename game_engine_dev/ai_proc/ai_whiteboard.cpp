//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "ai_whiteboard.h"

//================================================================================================================================
//=> - AiWhiteboard -
//================================================================================================================================

u16* AiWhiteboard::m_mem[AiWhiteboard::k_slot_n] = {};
bool AiWhiteboard::m_out[AiWhiteboard::k_slot_n] = {};
u32 AiWhiteboard::m_word_n = 0;
u32 AiWhiteboard::m_chkout = 0;

u16* AiWhiteboard::alloc (i32 tile_n) {
    if (tile_n <= 0) {
        return nullptr;
    }
    const u32 need = static_cast<u32>(tile_n);
    if (m_word_n == 0) {
        m_word_n = need;
        m_mem[0] = new u16[m_word_n];
        if (m_mem[0] == nullptr) {
            m_word_n = 0;
            return nullptr;
        }
        m_out[0] = false;
    } else if (need != m_word_n) {
        return nullptr;
    }
    u32 slot = k_slot_n;
    for (u32 i = 0; i < k_slot_n; ++i) {
        if (m_mem[i] != nullptr && !m_out[i]) {
            slot = i;
            break;
        }
    }
    if (slot == k_slot_n) {
        for (u32 i = 0; i < k_slot_n; ++i) {
            if (m_mem[i] == nullptr) {
                m_mem[i] = new u16[m_word_n];
                if (m_mem[i] == nullptr) {
                    return nullptr;
                }
                m_out[i] = false;
                slot = i;
                break;
            }
        }
    }
    if (slot == k_slot_n) {
        return nullptr;
    }
    m_out[slot] = true;
    m_chkout++;
    return m_mem[slot];
}

void AiWhiteboard::release (u16* ptr) {
    if (ptr == nullptr) {
        return;
    }
    for (u32 i = 0; i < k_slot_n; ++i) {
        if (m_mem[i] == ptr) {
            if (m_out[i]) {
                m_out[i] = false;
                if (m_chkout > 0) {
                    m_chkout--;
                }
            }
            return;
        }
    }
}

void AiWhiteboard::dealloc () {
    if (m_chkout > 0) {
        return;
    }
    for (u32 i = 0; i < k_slot_n; ++i) {
        delete[] m_mem[i];
        m_mem[i] = nullptr;
        m_out[i] = false;
    }
    m_word_n = 0;
}

u32 AiWhiteboard::chkout () {
    return m_chkout;
}

AiWbSheet::AiWbSheet (i32 tile_n) : m_p(AiWhiteboard::alloc(tile_n)) {}

AiWbSheet::~AiWbSheet () {
    AiWhiteboard::release(m_p);
}

u16* AiWbSheet::get () {
    return m_p;
}

u16 AiWbSheet::rd (u32 i) const {
    return m_p[i];
}

void AiWbSheet::wr (u32 i, u16 v) {
    m_p[i] = v;
}

bool AiWbSheet::ok () const {
    return m_p != nullptr;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
