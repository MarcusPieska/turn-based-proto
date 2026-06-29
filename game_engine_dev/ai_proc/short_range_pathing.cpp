//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "short_range_pathing.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u32 k_par_none = 0xFFFFFFFFu;
static const i16 k_dx4[] = {0, 1, 0, -1};
static const i16 k_dy4[] = {-1, 0, 1, 0};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static bool is_walk (u8 t) {
    return t != TERR_MOUNTAINS[0] && !is_water(t);
}

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool bfs_find (
    const GameArraySimple& map,
    u16 w,
    u16 h,
    u32 tile_n,
    u16 sx,
    u16 sy,
    u16 gx,
    u16 gy,
    u32* par) {
    if (sx == gx && sy == gy) {
        return true;
    }
    u32* q = new u32[tile_n];
    if (q == nullptr) {
        return false;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        par[i] = k_par_none;
    }
    const u32 si = tidx(w, sx, sy);
    const u32 gi = tidx(w, gx, gy);
    u32 qn = 0;
    par[si] = si;
    q[qn++] = si;
    bool found = false;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        if (i == gi) {
            found = true;
            break;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (u32 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 cx = static_cast<u16>(nx);
            const u16 cy = static_cast<u16>(ny);
            if (cx >= w || cy >= h || !is_walk(map.get_terrain(cx, cy))) {
                continue;
            }
            const u32 ni = tidx(w, cx, cy);
            if (par[ni] != k_par_none) {
                continue;
            }
            par[ni] = i;
            q[qn++] = ni;
        }
    }
    delete[] q;
    return found;
}

//================================================================================================================================
//=> - ShortRangePathing -
//================================================================================================================================

ShortRangePathing::ShortRangePathing (const GameArraySimple& map) : m_map(map) {}

u32 ShortRangePathing::cheb (u16 x0, u16 y0, u16 x1, u16 y1) {
    const u32 dx = (x0 > x1) ? static_cast<u32>(x0 - x1) : static_cast<u32>(x1 - x0);
    const u32 dy = (y0 > y1) ? static_cast<u32>(y0 - y1) : static_cast<u32>(y1 - y0);
    return (dx > dy) ? dx : dy;
}

bool ShortRangePathing::near_pt (u16 x, u16 y, u16 tx, u16 ty) const {
    return cheb(x, y, tx, ty) <= 1u;
}

bool ShortRangePathing::can_reach (u16 sx, u16 sy, u16 gx, u16 gy) const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* par = new u32[tile_n];
    if (par == nullptr) {
        return false;
    }
    const bool ok = bfs_find(m_map, w, h, tile_n, sx, sy, gx, gy, par);
    delete[] par;
    return ok;
}

bool ShortRangePathing::one_step (
    u16 sx,
    u16 sy,
    u16 gx,
    u16 gy,
    u16& ox,
    u16& oy) const {
    if (sx == gx && sy == gy) {
        return false;
    }
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* par = new u32[tile_n];
    if (par == nullptr) {
        return false;
    }
    if (!bfs_find(m_map, w, h, tile_n, sx, sy, gx, gy, par)) {
        delete[] par;
        return false;
    }
    const u32 si = tidx(w, sx, sy);
    u32 cur = tidx(w, gx, gy);
    while (par[cur] != si) {
        cur = par[cur];
    }
    const u16 py = static_cast<u16>(cur / static_cast<u32>(w));
    ox = static_cast<u16>(cur - static_cast<u32>(py) * static_cast<u32>(w));
    oy = py;
    delete[] par;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
