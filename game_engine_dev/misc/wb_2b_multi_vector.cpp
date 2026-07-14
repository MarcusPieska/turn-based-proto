//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "wb_2b_multi_vector.h"

//================================================================================================================================
//=> - WB_2BMultiVec -
//================================================================================================================================

WB_2BMultiVec::WB_2BMultiVec (cstr cls, cstr fn, u32 turn, u16 vec_n) :
    m_wb(cls, fn, turn),
    m_p(nullptr),
    m_vec_n(vec_n),
    m_seg_cap(0u),
    m_cnt(),
    m_cls(cls),
    m_fn(fn) {
    for (u32 i = 0; i < 16u; ++i) {
        m_cnt[i] = 0u;
    }
    if (!m_wb.ok() || m_vec_n == 0u || m_vec_n > 16u) {
        m_vec_n = 0u;
        return;
    }
    m_p = m_wb.get_iter_ptr();
    if (m_p == nullptr) {
        m_vec_n = 0u;
        return;
    }
    const u32 tn = WhiteboardMng::tile_n();
    m_seg_cap = tn / static_cast<u32>(m_vec_n);
    if (m_seg_cap == 0u) {
        m_vec_n = 0u;
    }
}

WB_2BMultiVec::~WB_2BMultiVec () {
    m_p = nullptr;
}

bool WB_2BMultiVec::ok () const {
    return m_wb.ok() && m_p != nullptr && m_vec_n > 0u && m_seg_cap > 0u;
}

u16 WB_2BMultiVec::vec_n () const {
    return m_vec_n;
}

u32 WB_2BMultiVec::seg_base (u16 vid) const {
    return static_cast<u32>(vid) * m_seg_cap;
}

bool WB_2BMultiVec::vid_ok (u16 vid) const {
    return ok() && vid < m_vec_n;
}

u32 WB_2BMultiVec::cap (u16 vid) const {
    if (!vid_ok(vid)) {
        return 0u;
    }
    return m_seg_cap;
}

u32 WB_2BMultiVec::count (u16 vid) const {
    if (!vid_ok(vid)) {
        return 0u;
    }
    return m_cnt[vid];
}

void WB_2BMultiVec::clear (u16 vid) {
    if (!vid_ok(vid)) {
        return;
    }
    m_cnt[vid] = 0u;
}

void WB_2BMultiVec::clear_all () {
    for (u16 v = 0; v < m_vec_n; ++v) {
        m_cnt[v] = 0u;
    }
}

bool WB_2BMultiVec::push (u16 vid, u16 v) {
    if (!vid_ok(vid)) {
        return false;
    }
    const u32 i = m_cnt[vid];
    if (i >= m_seg_cap) {
        return false;
    }
    m_p[seg_base(vid) + i] = v;
    m_cnt[vid] = i + 1u;
    return true;
}

u16 WB_2BMultiVec::at (u16 vid, u32 i) const {
    if (!vid_ok(vid) || i >= m_seg_cap) {
        return 0u;
    }
    return m_p[seg_base(vid) + i];
}

void WB_2BMultiVec::set (u16 vid, u32 i, u16 v) {
    if (!vid_ok(vid) || i >= m_seg_cap) {
        return;
    }
    const u32 wi = seg_base(vid) + i;
    m_p[wi] = v;
    if (i >= m_cnt[vid]) {
        m_cnt[vid] = i + 1u;
    }
}

cstr WB_2BMultiVec::cls () const {
    return m_cls;
}

cstr WB_2BMultiVec::fn () const {
    return m_fn;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
