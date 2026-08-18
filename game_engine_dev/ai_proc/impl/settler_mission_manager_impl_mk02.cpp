//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "game_map_defs.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Local -
//================================================================================================================================

static const i8 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i8 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const u32 k_none = 0xFFFFFFFFu;

static const u8* g_terr = nullptr;
static u16 g_w = 0;
static u16 g_h = 0;

static Whiteboard_4B& par () {
    static Whiteboard_4B b("SettlerMissionManager", "par", 0);
    return b;
}

static Whiteboard_4B& que () {
    static Whiteboard_4B b("SettlerMissionManager", "que", 0);
    return b;
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

static bool pass (u16 x, u16 y) {
    if (g_terr == nullptr || x >= g_w || y >= g_h) {
        return false;
    }
    return land_ok(g_terr[static_cast<u32>(y) * static_cast<u32>(g_w) + static_cast<u32>(x)]);
}

static u8 dir_of (i32 dx, i32 dy) {
    for (u8 d = 0; d < 8u; ++d) {
        if (k_dx[d] == dx && k_dy[d] == dy) {
            return d;
        }
    }
    return 8u;
}

static bool pack (u8* ps, u16* pn, u16* pi, u16 x, u16 y, u16 tx, u16 ty) {
    *pn = 0;
    *pi = 0;
    if (!pass(x, y) || !pass(tx, ty)) {
        return false;
    }
    if (x == tx && y == ty) {
        return true;
    }
    Whiteboard_4B& p = par();
    Whiteboard_4B& q = que();
    if (!p.ok() || !q.ok()) {
        return false;
    }
    const u32 tn = static_cast<u32>(g_w) * static_cast<u32>(g_h);
    std::memset(p.raw(), 0xFF, static_cast<size_t>(tn) * sizeof(u32));
    const u32 src = static_cast<u32>(y) * static_cast<u32>(g_w) + static_cast<u32>(x);
    const u32 dst = static_cast<u32>(ty) * static_cast<u32>(g_w) + static_cast<u32>(tx);
    p.wr_i(dst, dst);
    q.wr_i(0, dst);
    u32 qh = 0;
    u32 qt = 1;
    bool hit = false;
    while (qh < qt) {
        const u32 cur = q.rd_i(qh);
        qh += 1u;
        if (cur == src) {
            hit = true;
            break;
        }
        const u16 cx = static_cast<u16>(cur % static_cast<u32>(g_w));
        const u16 cy = static_cast<u16>(cur / static_cast<u32>(g_w));
        for (u8 d = 0; d < 8u; ++d) {
            const int nx = static_cast<int>(cx) + k_dx[d];
            const int ny = static_cast<int>(cy) + k_dy[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<int>(g_w) || ny >= static_cast<int>(g_h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!pass(ux, uy)) {
                continue;
            }
            const u32 ni = static_cast<u32>(uy) * static_cast<u32>(g_w) + static_cast<u32>(ux);
            if (p.rd_i(ni) != k_none) {
                continue;
            }
            p.wr_i(ni, cur);
            if (ni == src) {
                hit = true;
                break;
            }
            if (qt >= tn) {
                break;
            }
            q.wr_i(qt, ni);
            qt += 1u;
        }
        if (hit) {
            break;
        }
    }
    if (!hit || p.rd_i(src) == k_none) {
        return false;
    }
    u32 at = src;
    u16 n = 0;
    while (at != dst && n < static_cast<u16>(SMM_PATH_N)) {
        const u32 nxt = p.rd_i(at);
        if (nxt == k_none || nxt == at) {
            return false;
        }
        const i32 ax = static_cast<i32>(at % static_cast<u32>(g_w));
        const i32 ay = static_cast<i32>(at / static_cast<u32>(g_w));
        const i32 bx = static_cast<i32>(nxt % static_cast<u32>(g_w));
        const i32 by = static_cast<i32>(nxt / static_cast<u32>(g_w));
        const u8 d = dir_of(bx - ax, by - ay);
        if (d >= 8u) {
            return false;
        }
        ps[n] = d;
        n = static_cast<u16>(n + 1u);
        at = nxt;
    }
    *pn = n;
    *pi = 0;
    return n > 0u;
}

//================================================================================================================================
//=> - SettlerMissionManager mk02 -
//================================================================================================================================

bool SettlerMissionManager::wbeg (
    const SectorNetwork& net,
    const SectorNetworkRouter& rt,
    const u8* terr,
    u16 w,
    u16 h)
{
    (void)net;
    (void)rt;
    if (terr == nullptr || w == 0 || h == 0) {
        return false;
    }
    g_terr = terr;
    g_w = w;
    g_h = h;
    return par().ok() && que().ok();
}

bool SettlerMissionManager::aim (u16 s, u16 x0, u16 y0, u16 tx, u16 ty) {
    Slot& sl = m_slot[s];
    sl.m_x = x0;
    sl.m_y = y0;
    sl.m_tx = tx;
    sl.m_ty = ty;
    return pack(sl.m_ps, &sl.m_pn, &sl.m_pi, sl.m_x, sl.m_y, sl.m_tx, sl.m_ty);
}

bool SettlerMissionManager::wgo (u16 s) {
    Slot& sl = m_slot[s];
    if (sl.m_x == sl.m_tx && sl.m_y == sl.m_ty) {
        return false;
    }
    if (sl.m_pi >= sl.m_pn) {
        if (!pack(sl.m_ps, &sl.m_pn, &sl.m_pi, sl.m_x, sl.m_y, sl.m_tx, sl.m_ty)) {
            return false;
        }
        if (sl.m_pi >= sl.m_pn) {
            return false;
        }
    }
    const u8 d = sl.m_ps[sl.m_pi];
    sl.m_pi = static_cast<u16>(sl.m_pi + 1u);
    if (d >= 8u) {
        return false;
    }
    const int x = static_cast<int>(sl.m_x) + k_dx[d];
    const int y = static_cast<int>(sl.m_y) + k_dy[d];
    if (x < 0 || y < 0 || x >= static_cast<int>(g_w) || y >= static_cast<int>(g_h)) {
        return false;
    }
    sl.m_x = static_cast<u16>(x);
    sl.m_y = static_cast<u16>(y);
    return true;
}

bool SettlerMissionManager::wdn (u16 s) const {
    const Slot& sl = m_slot[s];
    return sl.m_x == sl.m_tx && sl.m_y == sl.m_ty;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
