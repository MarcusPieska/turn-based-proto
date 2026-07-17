//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "point_seq_walker.h"

#include "game_map_defs.h"
#include "sector_network.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 dist2 (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    return static_cast<u32>(dx * dx + dy * dy);
}

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
//=> - PointSeqWalker -
//================================================================================================================================

PointSeqWalker::PointSeqWalker () :
    m_ok(false),
    m_act(false),
    m_fin(false),
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
    m_best(0),
    m_terr(nullptr),
    m_net(nullptr),
    m_rt(nullptr) {
    m_path.m_n = 0;
}

PointSeqWalker::~PointSeqWalker () {
    clear();
}

void PointSeqWalker::clear () {
    m_ok = false;
    m_act = false;
    m_fin = false;
    m_w = 0;
    m_h = 0;
    m_terr = nullptr;
    m_net = nullptr;
    m_rt = nullptr;
    m_path.m_n = 0;
}

bool PointSeqWalker::begin (const SectorNetwork& net, const SectorNetworkRouter& router, const u8* terr,
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

bool PointSeqWalker::pass (u16 x, u16 y) const {
    if (m_terr == nullptr || x >= m_w || y >= m_h) {
        return false;
    }
    return land_ok(m_terr[static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x)]);
}

void PointSeqWalker::aim_next () {
    if (m_pi < m_path.m_n) {
        const u16 nxt = m_net->nbr(m_sid, m_path.get(m_pi));
        ++m_pi;
        if (nxt != U16_KEY_NULL) {
            m_sid = nxt;
            const Sector* s = m_net->get(m_sid);
            m_tx = s->m_x;
            m_ty = s->m_y;
            m_best = dist2(m_x, m_y, m_tx, m_ty);
            return;
        }
    }
    m_tx = m_gx;
    m_ty = m_gy;
    m_best = dist2(m_x, m_y, m_tx, m_ty);
}

bool PointSeqWalker::greedy () {
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    u16 best_x = m_x;
    u16 best_y = m_y;
    u32 best_d = m_best;
    bool moved = false;
    for (i32 dir = 0; dir < 4; ++dir) {
        const i32 nx = static_cast<i32>(m_x) + dx4[dir];
        const i32 ny = static_cast<i32>(m_y) + dy4[dir];
        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(m_w) || ny >= static_cast<i32>(m_h)) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!pass(ux, uy)) {
            continue;
        }
        const u32 d = dist2(ux, uy, m_tx, m_ty);
        if (d < best_d) {
            best_d = d;
            best_x = ux;
            best_y = uy;
            moved = true;
        }
    }
    if (!moved) {
        return false;
    }
    m_x = best_x;
    m_y = best_y;
    m_best = best_d;
    return true;
}

bool PointSeqWalker::escape () {
    enum { R = 12, Q = 128, W = R * 2 + 1 };
    if (m_best == 0) {
        return false;
    }
    const i32 ox = static_cast<i32>(m_x);
    const i32 oy = static_cast<i32>(m_y);
    u16 qx[Q];
    u16 qy[Q];
    i8 fdx[Q];
    i8 fdy[Q];
    u8 vis[(W * W + 7) / 8];
    for (u32 i = 0; i < sizeof(vis); ++i) {
        vis[i] = 0;
    }
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    u16 qh = 0;
    u16 qt = 0;
    qx[qt] = m_x;
    qy[qt] = m_y;
    fdx[qt] = 0;
    fdy[qt] = 0;
    ++qt;
    {
        const u32 vi = static_cast<u32>(R * W + R);
        vis[vi >> 3] = static_cast<u8>(vis[vi >> 3] | static_cast<u8>(1u << (vi & 7u)));
    }
    while (qh < qt) {
        const u16 cx = qx[qh];
        const u16 cy = qy[qh];
        const i8 sdx = fdx[qh];
        const i8 sdy = fdy[qh];
        ++qh;
        for (i32 dir = 0; dir < 4; ++dir) {
            const i32 nx = static_cast<i32>(cx) + dx4[dir];
            const i32 ny = static_cast<i32>(cy) + dy4[dir];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(m_w) || ny >= static_cast<i32>(m_h)) {
                continue;
            }
            const i32 lx = nx - ox + R;
            const i32 ly = ny - oy + R;
            if (lx < 0 || ly < 0 || lx >= W || ly >= W) {
                continue;
            }
            const u32 vi = static_cast<u32>(ly * W + lx);
            if ((vis[vi >> 3] & static_cast<u8>(1u << (vi & 7u))) != 0) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!pass(ux, uy)) {
                continue;
            }
            const i8 ndx = (sdx == 0 && sdy == 0) ? static_cast<i8>(dx4[dir]) : sdx;
            const i8 ndy = (sdx == 0 && sdy == 0) ? static_cast<i8>(dy4[dir]) : sdy;
            if (dist2(ux, uy, m_tx, m_ty) < m_best) {
                m_x = static_cast<u16>(static_cast<i32>(m_x) + ndx);
                m_y = static_cast<u16>(static_cast<i32>(m_y) + ndy);
                const u32 d = dist2(m_x, m_y, m_tx, m_ty);
                if (d < m_best) {
                    m_best = d;
                }
                return true;
            }
            if (qt >= Q) {
                continue;
            }
            vis[vi >> 3] = static_cast<u8>(vis[vi >> 3] | static_cast<u8>(1u << (vi & 7u)));
            qx[qt] = ux;
            qy[qt] = uy;
            fdx[qt] = ndx;
            fdy[qt] = ndy;
            ++qt;
        }
    }
    return false;
}

bool PointSeqWalker::start (u16 x0, u16 y0, u16 x1, u16 y1) {
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
    m_act = true;
    m_fin = (x0 == x1 && y0 == y1);
    if (!m_fin) {
        aim_next();
    } else {
        m_best = 0;
    }
    return true;
}

bool PointSeqWalker::step () {
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
        if (greedy() || escape()) {
            if (m_x == m_gx && m_y == m_gy) {
                m_fin = true;
            }
            return true;
        }
        if (m_tx == m_gx && m_ty == m_gy) {
            break;
        }
        aim_next();
    }
    m_act = false;
    return false;
}

bool PointSeqWalker::done () const {
    return m_fin;
}

bool PointSeqWalker::is_valid () const {
    return m_ok;
}

u16 PointSeqWalker::x () const {
    return m_x;
}

u16 PointSeqWalker::y () const {
    return m_y;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
