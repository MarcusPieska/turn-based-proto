//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "wb_que_xy.h"

#include "whiteboard_mng.h"

//================================================================================================================================
//=> - WB_QueXY -
//================================================================================================================================

WB_QueXY::WB_QueXY () :
    m_wb0(nullptr),
    m_wb1(nullptr),
    m_p(nullptr),
    m_p2(nullptr),
    m_ent_n(0u),
    m_head(0u),
    m_cnt(0u) {
    m_wb0 = new Whiteboard_2B("WB_QueXY", "buf0", 0u);
    m_wb1 = new Whiteboard_2B("WB_QueXY", "buf1", 0u);
    if (m_wb0 == nullptr || m_wb1 == nullptr || !m_wb0->ok() || !m_wb1->ok()) {
        delete m_wb1;
        delete m_wb0;
        m_wb1 = nullptr;
        m_wb0 = nullptr;
        return;
    }
    m_p = m_wb0->get_iter_ptr();
    m_p2 = m_wb1->get_iter_ptr();
    m_ent_n = WhiteboardMng::tile_n();
}

WB_QueXY::~WB_QueXY () {
    delete m_wb1;
    delete m_wb0;
    m_wb1 = nullptr;
    m_wb0 = nullptr;
    m_p = nullptr;
    m_p2 = nullptr;
}

bool WB_QueXY::ok () const {
    return m_wb0 != nullptr && m_wb1 != nullptr && m_p != nullptr && m_p2 != nullptr && m_ent_n > 0u;
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

void WB_QueXY::wr_u16 (u32 wi, u16 v) const {
    const u32 tn = WhiteboardMng::tile_n();
    if (wi < tn) {
        m_p[wi] = v;
    } else {
        m_p2[wi - tn] = v;
    }
}

u16 WB_QueXY::rd_u16 (u32 wi) const {
    const u32 tn = WhiteboardMng::tile_n();
    if (wi < tn) {
        return m_p[wi];
    }
    return m_p2[wi - tn];
}

bool WB_QueXY::push (u16 x, u16 y) {
    if (m_cnt >= m_ent_n) {
        return false;
    }
    const u32 t = slot(m_cnt);
    const u32 w0 = t * 2u;
    wr_u16(w0, x);
    wr_u16(w0 + 1u, y);
    m_cnt++;
    return true;
}

u16 WB_QueXY::x_at (u32 i) const {
    const u32 t = slot(i);
    return rd_u16(t * 2u);
}

u16 WB_QueXY::y_at (u32 i) const {
    const u32 t = slot(i);
    return rd_u16(t * 2u + 1u);
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
    Whiteboard_2B* tb0 = m_wb0;
    m_wb0 = o.m_wb0;
    o.m_wb0 = tb0;
    Whiteboard_2B* tb1 = m_wb1;
    m_wb1 = o.m_wb1;
    o.m_wb1 = tb1;
    u16* tp = m_p;
    m_p = o.m_p;
    o.m_p = tp;
    u16* tp2 = m_p2;
    m_p2 = o.m_p2;
    o.m_p2 = tp2;
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
//=> - End -
//================================================================================================================================
