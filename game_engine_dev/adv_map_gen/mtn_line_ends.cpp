//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "mtn_line_ends.h"

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static const i32 k_dx8[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
static const i32 k_dy8[8] = {0, 0, -1, 1, -1, 1, -1, 1};

static bool on_line (const u16* ov, u32 i) {
    return ov != nullptr && ov[i] != 0u;
}

//================================================================================================================================
//=> - MtnLineEnds -
//================================================================================================================================

MtnLineEnds::MtnLineEnds (u32 seed) :
    m_seed(seed),
    m_ok(false),
    m_open(false),
    m_ov(nullptr),
    m_w(0),
    m_h(0),
    m_scan(0),
    m_line_n(0),
    m_comp_n(0),
    m_done("MtnLineEnds", "done", seed),
    m_mk2("MtnLineEnds", "mark2", seed),
    m_s1("MtnLineEnds", "step1", seed),
    m_s2("MtnLineEnds", "step2", seed),
    m_bfs(),
    m_visit() {
    m_ok = m_done.ok() && m_mk2.ok() && m_s1.ok() && m_s2.ok() && m_bfs.ok() && m_visit.ok();
    P1_WB_CHK(m_done);
    P1_WB_CHK(m_mk2);
    P1_WB_CHK(m_s1);
    P1_WB_CHK(m_s2);
}

MtnLineEnds::~MtnLineEnds () {
    m_ov = nullptr;
    m_open = false;
}

bool MtnLineEnds::ok () const {
    return m_ok;
}

u32 MtnLineEnds::line_n () const {
    return m_line_n;
}

u32 MtnLineEnds::comp_n () const {
    return m_comp_n;
}

bool MtnLineEnds::begin (const u16* line_ov, u16 w, u16 h) {
    m_open = false;
    m_ov = nullptr;
    m_scan = 0u;
    m_line_n = 0u;
    m_comp_n = 0u;
    if (!m_ok || line_ov == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    if (w != m_done.w() || h != m_done.h()) {
        return false;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(m_done.raw(), 0, static_cast<size_t>(npx));
    std::memset(m_mk2.raw(), 0, static_cast<size_t>(npx));
    std::memset(m_s1.get_iter_ptr(), 0, static_cast<size_t>(npx) * sizeof(u16));
    std::memset(m_s2.get_iter_ptr(), 0, static_cast<size_t>(npx) * sizeof(u16));
    m_ov = line_ov;
    m_w = w;
    m_h = h;
    m_open = true;
    return true;
}

bool MtnLineEnds::flood_pass (
    u16 sx,
    u16 sy,
    const u8* in_comp,
    u8* mark,
    u16* step,
    WB_QueXY* visit,
    u16* far_x,
    u16* far_y) 
{
    const u32 wi = static_cast<u32>(m_w);
    const u32 hi = static_cast<u32>(m_h);
    const u32 si = static_cast<u32>(sy) * wi + static_cast<u32>(sx);
    m_bfs.clear();
    if (visit != nullptr) {
        visit->clear();
    }
    if (!on_line(m_ov, si)) {
        return false;
    }
    if (in_comp != nullptr && in_comp[si] == 0u) {
        return false;
    }
    if (mark[si] != 0u) {
        return false;
    }
    if (!m_bfs.push(sx, sy)) {
        return false;
    }
    mark[si] = 1u;
    step[si] = 0u;
    if (visit != nullptr && !visit->push(sx, sy)) {
        return false;
    }
    *far_x = sx;
    *far_y = sy;
    u16 far_s = 0u;
    u32 qi = 0u;
    while (qi < m_bfs.count()) {
        const u16 px = m_bfs.x_at(qi);
        const u16 py = m_bfs.y_at(qi);
        const u32 pi = static_cast<u32>(py) * wi + static_cast<u32>(px);
        const u16 ps = step[pi];
        qi++;
        if (ps > far_s) {
            far_s = ps;
            *far_x = px;
            *far_y = py;
        }
        for (i32 d = 0; d < 8; ++d) {
            const i32 nx = static_cast<i32>(px) + k_dx8[d];
            const i32 ny = static_cast<i32>(py) + k_dy8[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
            if (!on_line(m_ov, ni) || mark[ni] != 0u) {
                continue;
            }
            if (in_comp != nullptr && in_comp[ni] == 0u) {
                continue;
            }
            mark[ni] = 1u;
            step[ni] = static_cast<u16>(ps + 1u);
            if (!m_bfs.push(static_cast<u16>(nx), static_cast<u16>(ny))) {
                return false;
            }
            if (visit != nullptr && !visit->push(static_cast<u16>(nx), static_cast<u16>(ny))) {
                return false;
            }
        }
    }
    return true;
}

bool MtnLineEnds::flood_comp (u16 sx, u16 sy, MtnLineEndPair* out) {
    u8* done = m_done.raw();
    u8* mark2 = m_mk2.raw();
    u16* step1 = m_s1.get_iter_ptr();
    u16* step2 = m_s2.get_iter_ptr();
    const u32 wi = static_cast<u32>(m_w);
    u16 far_x = sx;
    u16 far_y = sy;
    if (!flood_pass(sx, sy, nullptr, done, step1, &m_visit, &far_x, &far_y)) {
        return false;
    }
    const u32 n = m_visit.count();
    if (n == 0u) {
        return false;
    }
    out->m_sx = sx;
    out->m_sy = sy;
    out->m_ax = far_x;
    out->m_ay = far_y;
    for (u32 vi = 0; vi < n; ++vi) {
        const u32 ti = static_cast<u32>(m_visit.y_at(vi)) * wi + static_cast<u32>(m_visit.x_at(vi));
        mark2[ti] = 0u;
        step2[ti] = 0u;
    }
    if (!flood_pass(out->m_ax, out->m_ay, done, mark2, step2, nullptr, &far_x, &far_y)) {
        return false;
    }
    out->m_bx = far_x;
    out->m_by = far_y;
    m_line_n += n;
    m_comp_n++;
    return true;
}

bool MtnLineEnds::next (MtnLineEndPair* out) {
    if (out == nullptr) {
        return false;
    }
    if (!m_ok || !m_open || m_ov == nullptr) {
        *out = mtn_line_end_pair_nil();
        return false;
    }
    const u32 npx = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    u8* done = m_done.raw();
    while (m_scan < npx) {
        const u32 i = m_scan++;
        if (!on_line(m_ov, i) || done[i] != 0u) {
            continue;
        }
        const u16 sx = static_cast<u16>(i % static_cast<u32>(m_w));
        const u16 sy = static_cast<u16>(i / static_cast<u32>(m_w));
        if (!flood_comp(sx, sy, out)) {
            *out = mtn_line_end_pair_nil();
            m_open = false;
            return false;
        }
        return true;
    }
    *out = mtn_line_end_pair_nil();
    m_open = false;
    return false;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
