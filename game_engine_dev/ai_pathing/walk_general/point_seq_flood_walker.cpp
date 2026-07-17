//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "point_seq_flood_walker.h"

#include "game_map_defs.h"
#include "sector_network.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool land_ok (u8 terr) {
    if (overlay_is_water_terr(terr)) {
        return false;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_NONE[0]) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - PointSeqFloodWalker -
//================================================================================================================================

PointSeqFloodWalker::PointSeqFloodWalker () :
    m_ok(false),
    m_act(false),
    m_fin(false),
    m_need(false),
    m_w(0),
    m_h(0),
    m_x(0),
    m_y(0),
    m_gx(0),
    m_gy(0),
    m_tx(0),
    m_ty(0),
    m_sid(U16_KEY_NULL),
    m_pi(0),
    m_fn(0),
    m_fi(0),
    m_terr(nullptr),
    m_net(nullptr),
    m_rt(nullptr) {
    m_path.m_n = 0;
}

PointSeqFloodWalker::~PointSeqFloodWalker () {
    clear();
}

void PointSeqFloodWalker::clear () {
    m_ok = false;
    m_act = false;
    m_fin = false;
    m_need = false;
    m_w = 0;
    m_h = 0;
    m_fn = 0;
    m_fi = 0;
    m_terr = nullptr;
    m_net = nullptr;
    m_rt = nullptr;
    m_path.m_n = 0;
}

bool PointSeqFloodWalker::begin (const SectorNetwork& net, const SectorNetworkRouter& router, const u8* terr,
    u16 w, u16 h) {
    clear();
    if (!net.is_valid() || !router.is_valid() || terr == nullptr || w == 0 || h == 0) {
        return false;
    }
    m_net = &net;
    m_rt = &router;
    m_terr = terr;
    m_w = w;
    m_h = h;
    m_ok = true;
    return true;
}

bool PointSeqFloodWalker::pass (u16 x, u16 y) const {
    if (m_terr == nullptr || x >= m_w || y >= m_h) {
        return false;
    }
    return land_ok(m_terr[static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x)]);
}

void PointSeqFloodWalker::aim_next () {
    if (m_pi < m_path.m_n) {
        const u16 nxt = m_net->nbr(m_sid, m_path.get(m_pi));
        ++m_pi;
        if (nxt != U16_KEY_NULL) {
            m_sid = nxt;
            const Sector* s = m_net->get(m_sid);
            m_tx = s->m_x;
            m_ty = s->m_y;
            m_need = true;
            m_fn = 0;
            m_fi = 0;
            return;
        }
    }
    m_tx = m_gx;
    m_ty = m_gy;
    m_need = true;
    m_fn = 0;
    m_fi = 0;
}

bool PointSeqFloodWalker::plan () {
    const i32 W = static_cast<i32>(PSF_WIN);
    const i32 HALF = W / 2;
    const u8 NONE = 255;
    m_fn = 0;
    m_fi = 0;
    if (m_w < 1 || m_h < 1) {
        return false;
    }
    if (m_x == m_tx && m_y == m_ty) {
        m_need = false;
        return false;
    }
    const i32 bw = (static_cast<i32>(m_w) < W) ? static_cast<i32>(m_w) : W;
    const i32 bh = (static_cast<i32>(m_h) < W) ? static_cast<i32>(m_h) : W;
    i32 x0 = static_cast<i32>(m_x) - HALF;
    i32 y0 = static_cast<i32>(m_y) - HALF;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x0 + bw > static_cast<i32>(m_w)) {
        x0 = static_cast<i32>(m_w) - bw;
    }
    if (y0 + bh > static_cast<i32>(m_h)) {
        y0 = static_cast<i32>(m_h) - bh;
    }
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    const i32 sx = static_cast<i32>(m_x) - x0;
    const i32 sy = static_cast<i32>(m_y) - y0;
    if (sx < 0 || sy < 0 || sx >= bw || sy >= bh) {
        return false;
    }
    i32 dx = static_cast<i32>(m_tx) - x0;
    i32 dy = static_cast<i32>(m_ty) - y0;
    if (dx < 0) {
        dx = 0;
    }
    if (dy < 0) {
        dy = 0;
    }
    if (dx >= bw) {
        dx = bw - 1;
    }
    if (dy >= bh) {
        dy = bh - 1;
    }
    if (!pass(static_cast<u16>(x0 + dx), static_cast<u16>(y0 + dy))) {
        i32 best_d = 0x7fffffff;
        i32 bx = -1;
        i32 by = -1;
        for (i32 ly = 0; ly < bh; ++ly) {
            for (i32 lx = 0; lx < bw; ++lx) {
                if (!pass(static_cast<u16>(x0 + lx), static_cast<u16>(y0 + ly))) {
                    continue;
                }
                const i32 ddx = (x0 + lx) - static_cast<i32>(m_tx);
                const i32 ddy = (y0 + ly) - static_cast<i32>(m_ty);
                const i32 d = ddx * ddx + ddy * ddy;
                if (d < best_d) {
                    best_d = d;
                    bx = lx;
                    by = ly;
                }
            }
        }
        if (bx < 0) {
            return false;
        }
        dx = bx;
        dy = by;
    }
    const u8 src = static_cast<u8>(sy * bw + sx);
    const u8 dst = static_cast<u8>(dy * bw + dx);
    if (src == dst) {
        return false;
    }
    const i32 cells = bw * bh;
    for (i32 i = 0; i < cells; ++i) {
        m_par[i] = NONE;
    }
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    u16 qh = 0;
    u16 qt = 0;
    m_q[qt++] = dst;
    m_par[dst] = dst;
    bool hit = false;
    while (qh < qt) {
        const u8 cur = m_q[qh++];
        if (cur == src) {
            hit = true;
            break;
        }
        const i32 cx = static_cast<i32>(cur % bw);
        const i32 cy = static_cast<i32>(cur / bw);
        for (i32 dir = 0; dir < 4; ++dir) {
            const i32 nx = cx + dx4[dir];
            const i32 ny = cy + dy4[dir];
            if (nx < 0 || ny < 0 || nx >= bw || ny >= bh) {
                continue;
            }
            const u8 ni = static_cast<u8>(ny * bw + nx);
            if (m_par[ni] != NONE) {
                continue;
            }
            if (!pass(static_cast<u16>(x0 + nx), static_cast<u16>(y0 + ny))) {
                continue;
            }
            m_par[ni] = cur;
            m_q[qt++] = ni;
        }
    }
    if (!hit || m_par[src] == NONE || m_par[src] == src) {
        return false;
    }
    u8 rev[PSF_WIN_N];
    u16 rn = 0;
    u8 at = src;
    while (at != dst) {
        const u8 nxt = m_par[at];
        if (nxt == at || rn >= PSF_WIN_N) {
            return false;
        }
        rev[rn++] = nxt;
        at = nxt;
    }
    for (u16 i = 0; i < rn; ++i) {
        const u8 wi = rev[i];
        m_sx[i] = static_cast<u16>(x0 + static_cast<i32>(wi % bw));
        m_sy[i] = static_cast<u16>(y0 + static_cast<i32>(wi / bw));
    }
    m_fn = rn;
    m_fi = 0;
    m_need = false;
    return m_fn > 0;
}

bool PointSeqFloodWalker::start (u16 x0, u16 y0, u16 x1, u16 y1) {
    if (!m_ok || m_net == nullptr || m_rt == nullptr) {
        return false;
    }
    if (!pass(x0, y0) || !pass(x1, y1)) {
        return false;
    }
    const u16 a = m_net->id_at(x0, y0);
    const u16 b = m_net->id_at(x1, y1);
    if (a == U16_KEY_NULL || b == U16_KEY_NULL) {
        return false;
    }
    if (!m_rt->find(a, b, &m_path)) {
        return false;
    }
    m_x = x0;
    m_y = y0;
    m_gx = x1;
    m_gy = y1;
    m_sid = a;
    m_pi = 0;
    m_fn = 0;
    m_fi = 0;
    m_act = true;
    m_fin = (x0 == x1 && y0 == y1);
    m_need = !m_fin;
    if (!m_fin) {
        aim_next();
    }
    return true;
}

bool PointSeqFloodWalker::step () {
    if (!m_act || m_fin) {
        return false;
    }
    if (m_x == m_gx && m_y == m_gy) {
        m_fin = true;
        return false;
    }
    if (m_x == m_tx && m_y == m_ty) {
        if (m_tx == m_gx && m_ty == m_gy) {
            m_fin = true;
            return false;
        }
        aim_next();
    }
    for (u8 try_n = 0; try_n < 6; ++try_n) {
        if (m_need || m_fi >= m_fn) {
            if (!plan()) {
                if (m_tx == m_gx && m_ty == m_gy) {
                    break;
                }
                aim_next();
                continue;
            }
        }
        if (m_fi >= m_fn) {
            if (m_tx == m_gx && m_ty == m_gy) {
                break;
            }
            aim_next();
            continue;
        }
        m_x = m_sx[m_fi];
        m_y = m_sy[m_fi];
        ++m_fi;
        if (m_fi >= m_fn && (m_x != m_tx || m_y != m_ty)) {
            m_need = true;
        }
        if (m_x == m_gx && m_y == m_gy) {
            m_fin = true;
        }
        return true;
    }
    m_act = false;
    return false;
}

bool PointSeqFloodWalker::done () const {
    return m_fin;
}

bool PointSeqFloodWalker::is_valid () const {
    return m_ok;
}

u16 PointSeqFloodWalker::x () const {
    return m_x;
}

u16 PointSeqFloodWalker::y () const {
    return m_y;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
