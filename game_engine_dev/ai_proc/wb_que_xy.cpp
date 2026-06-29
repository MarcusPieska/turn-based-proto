//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "wb_que_xy.h"
#include "ai_whiteboard.h"

//================================================================================================================================
//=> - WB_QueXY -
//================================================================================================================================

WB_QueXY::WB_QueXY (i32 word_n) : m_p(nullptr), m_ent_n(0u), m_head(0u), m_cnt(0u) {
    if (word_n < 2) {
        return;
    }
    m_p = AiWhiteboard::alloc(word_n);
    if (m_p == nullptr) {
        return;
    }
    m_ent_n = static_cast<u32>(word_n) / 2u;
}

WB_QueXY::~WB_QueXY () {
    AiWhiteboard::release(m_p);
    m_p = nullptr;
}

bool WB_QueXY::ok () const {
    return m_p != nullptr && m_ent_n > 0u;
}

u32 WB_QueXY::cap () const {
    return m_ent_n;
}

u32 WB_QueXY::count () const {
    return m_cnt;
}

void WB_QueXY::clear () {
    m_head = 0u;
    m_cnt = 0u;
}

u32 WB_QueXY::slot (u32 i) const {
    return (m_head + i) % m_ent_n;
}

bool WB_QueXY::push (u16 x, u16 y) {
    if (m_cnt >= m_ent_n) {
        return false;
    }
    const u32 t = slot(m_cnt);
    m_p[2u * t] = x;
    m_p[2u * t + 1u] = y;
    m_cnt++;
    return true;
}

u16 WB_QueXY::x_at (u32 i) const {
    const u32 t = slot(i);
    return m_p[2u * t];
}

u16 WB_QueXY::y_at (u32 i) const {
    const u32 t = slot(i);
    return m_p[2u * t + 1u];
}

void WB_QueXY::drop (u32 n) {
    if (n >= m_cnt) {
        clear();
        return;
    }
    m_head = (m_head + n) % m_ent_n;
    m_cnt -= n;
}

void WB_QueXY::swap (WB_QueXY& o) noexcept {
    u16* tp = m_p;
    m_p = o.m_p;
    o.m_p = tp;
    u32 te = m_ent_n;
    m_ent_n = o.m_ent_n;
    o.m_ent_n = te;
    u32 th = m_head;
    m_head = o.m_head;
    o.m_head = th;
    u32 tc = m_cnt;
    m_cnt = o.m_cnt;
    o.m_cnt = tc;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
