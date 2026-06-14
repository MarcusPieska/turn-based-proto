//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "whiteboard.h"

//================================================================================================================================
//=> - Whiteboard -
//================================================================================================================================

u16* Whiteboard::m_mem[Whiteboard::k_slot_n] = {};
bool Whiteboard::m_out[Whiteboard::k_slot_n] = {};
u32 Whiteboard::m_word_n = 0;
u32 Whiteboard::m_chkout = 0;

u16* Whiteboard::alloc (i32 tile_n) {
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

void Whiteboard::release (u16* ptr) {
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

void Whiteboard::dealloc () {
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

u32 Whiteboard::chkout () {
    return m_chkout;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
