//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "river_pathing.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u32 k_par_none = 0xFFFFFFFFu;
static const i32 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i32 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, u16 x, u16 y) {
    return x < w && y < h;
}

static bool nbr8 (u16 w, u16 h, u16 x, u16 y, u32 k, u16& ox, u16& oy) {
    const i32 nx = static_cast<i32>(x) + k_dx8[k];
    const i32 ny = static_cast<i32>(y) + k_dy8[k];
    if (nx < 0 || ny < 0) {
        return false;
    }
    ox = static_cast<u16>(nx);
    oy = static_cast<u16>(ny);
    return in_map(w, h, ox, oy);
}

//================================================================================================================================
//=> - RiverPathing -
//================================================================================================================================

bool RiverPathing::is_riv (const GameArraySimple& map, u16 x, u16 y) {
    return map.get_river(x, y) != 0u;
}

bool RiverPathing::has_unexp_chb (
    const GameArraySimple& map,
    const MapBitOverlay& exp,
    u16 x,
    u16 y,
    u16 sight) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (i16 dy = -static_cast<i16>(sight); dy <= static_cast<i16>(sight); ++dy) {
        for (i16 dx = -static_cast<i16>(sight); dx <= static_cast<i16>(sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(x) + dx;
            const i32 yi = static_cast<i32>(y) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 ax = static_cast<u16>(xi);
            const u16 ay = static_cast<u16>(yi);
            if (ax >= w || ay >= h) {
                continue;
            }
            if (exp.get(ax, ay) == 0u) {
                return true;
            }
        }
    }
    return false;
}

bool RiverPathing::is_riv_front (const GameArraySimple& map, const MapBitOverlay& exp, u16 x, u16 y, u16 sight) {
    return is_riv(map, x, y) && has_unexp_chb(map, exp, x, y, sight);
}

bool RiverPathing::pick_near_front (
    const GameArraySimple& map,
    const MapBitOverlay& exp,
    u16 sx,
    u16 sy,
    u16 sight,
    u16& ox,
    u16& oy,
    u16 skx,
    u16 sky,
    bool has_sk) {

    const u16 w = map.width();
    const u16 h = map.height();
    for (u32 k = 0; k < 8u; ++k) {
        u16 nx = 0;
        u16 ny = 0;
        if (!nbr8(w, h, sx, sy, k, nx, ny)) {
            continue;
        }
        if (has_sk && nx == skx && ny == sky) {
            continue;
        }
        if (!is_riv_front(map, exp, nx, ny, sight)) {
            continue;
        }
        ox = nx;
        oy = ny;
        return true;
    }
    return false;
}

static bool fill_path (u16 w, u16 sx, u16 sy, u16 gx, u16 gy, const u32* par, PathMk1& path) {
    path.clr();
    const u32 si = tidx(w, sx, sy);
    const u32 gi = tidx(w, gx, gy);
    u16 chain_n = 0;
    u32 cur = gi;
    while (cur != si) {
        ++chain_n;
        if (par[cur] == k_par_none) {
            return false;
        }
        cur = par[cur];
    }
    ++chain_n;
    u16* cx = new u16[chain_n];
    u16* cy = new u16[chain_n];
    if (cx == nullptr || cy == nullptr) {
        delete[] cx;
        delete[] cy;
        return false;
    }
    cur = gi;
    u16 ci = chain_n;
    while (true) {
        const u16 py = static_cast<u16>(cur / static_cast<u32>(w));
        const u16 px = static_cast<u16>(cur - static_cast<u32>(py) * static_cast<u32>(w));
        --ci;
        cx[ci] = px;
        cy[ci] = py;
        if (cur == si) {
            break;
        }
        cur = par[cur];
    }
    for (u16 i = 0; i < chain_n; ++i) {
        if (!path.push(cx[i], cy[i])) {
            delete[] cx;
            delete[] cy;
            return false;
        }
    }
    delete[] cx;
    delete[] cy;
    return true;
}

bool RiverPathing::find_path_to_front (
    const GameArraySimple& map,
    const MapBitOverlay& exp,
    u16 sx,
    u16 sy,
    u16 sight,
    PathMk1& path) {
    if (!is_riv(map, sx, sy)) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tile_n = map.tile_n();
    u32* par = new u32[tile_n];
    u32* q = new u32[tile_n];
    if (par == nullptr || q == nullptr) {
        delete[] par;
        delete[] q;
        return false;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        par[i] = k_par_none;
    }
    const u32 si = tidx(w, sx, sy);
    par[si] = si;
    u32 qn = 0;
    q[qn++] = si;
    u32* dist = new u32[tile_n];
    if (dist == nullptr) {
        delete[] par;
        delete[] q;
        return false;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        dist[i] = k_par_none;
    }
    dist[si] = 0u;
    u32 best_i = k_par_none;
    u32 best_d = 0xFFFFFFFFu;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u32 d = dist[i];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        if (is_riv_front(map, exp, px, py, sight) && d < best_d) {
            best_d = d;
            best_i = i;
        }
        for (u32 k = 0; k < 8u; ++k) {
            u16 nx = 0;
            u16 ny = 0;
            if (!nbr8(w, h, px, py, k, nx, ny)) {
                continue;
            }
            if (!is_riv(map, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (par[ni] != k_par_none) {
                continue;
            }
            par[ni] = i;
            dist[ni] = d + 1u;
            q[qn++] = ni;
        }
    }
    bool ok = false;
    if (best_i != k_par_none) {
        const u16 gy = static_cast<u16>(best_i / static_cast<u32>(w));
        const u16 gx = static_cast<u16>(best_i - static_cast<u32>(gy) * static_cast<u32>(w));
        ok = fill_path(w, sx, sy, gx, gy, par, path);
    }
    delete[] dist;
    delete[] par;
    delete[] q;
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
