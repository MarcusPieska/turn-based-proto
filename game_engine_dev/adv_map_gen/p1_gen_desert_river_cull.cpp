//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "p1_gen_desert_river_cull.h"

#include "generator_constants.h"
#include "p1_gen_climate.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 0, 1, 0};
static const i32 k_dy4[4] = {0, -1, 0, 1};
static const i32 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i32 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

static const u32 k_prev_none = 0xFFFFFFFFu;

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_desert_climate (u8 cl) {
    return cl == CLIMATE_DESERT;
}

static bool is_riv_mouth (const u8* terrain, const u8* riv, u16 w, u16 h, u16 x, u16 y) {
    const u32 i = tidx(w, x, y);
    if (riv[i] == 0) {
        return false;
    }
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        if (is_water(terrain[tidx(w, static_cast<u16>(nx), static_cast<u16>(ny))])) {
            return true;
        }
    }
    return false;
}

static u32 count_riv_neighbors (
    const u8* riv,
    u16 w,
    u16 h,
    u32 i,
    u32 skip) 
{
    const u16 py = static_cast<u16>(i / static_cast<u32>(w));
    const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
    u32 n = 0;
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(px) + k_dx8[k];
        const i32 ny = static_cast<i32>(py) + k_dy8[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
        if (ni == skip || riv[ni] == 0) {
            continue;
        }
        n++;
    }
    return n;
}

static bool is_branch_tip (
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u16 x,
    u16 y) 
{
    const u32 i = tidx(w, x, y);
    if (riv[i] == 0) {
        return false;
    }
    if (is_riv_mouth(terrain, riv, w, h, x, y)) {
        return false;
    }
    return count_riv_neighbors(riv, w, h, i, k_prev_none) == 1;
}

static u32 pick_riv_next (
    const u8* riv,
    u16 w,
    u16 h,
    u32 cur,
    u32 prev,
    u32* out) 
{
    const u16 py = static_cast<u16>(cur / static_cast<u32>(w));
    const u16 px = static_cast<u16>(cur - static_cast<u32>(py) * static_cast<u32>(w));
    u32 n = 0;
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(px) + k_dx8[k];
        const i32 ny = static_cast<i32>(py) + k_dy8[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
        if (ni == prev || riv[ni] == 0) {
            continue;
        }
        out[n++] = ni;
    }
    return n;
}

static void cull_desert_run (
    const u8* riv,
    const u8* climate,
    u8* mark,
    u16 w,
    u16 h,
    u32 start,
    u32 prev,
    u32 n_max) 
{
    u32 cur = start;
    u32 p = prev;
    u32 steps = 0;
    while (riv[cur] != 0 && is_desert_climate(climate[cur]) && steps < n_max) {
        if (mark[cur] != 0) {
            break;
        }
        mark[cur] = 1;
        steps++;
        u32 next[8];
        const u32 nn = pick_riv_next(riv, w, h, cur, p, next);
        if (nn != 1) {
            break;
        }
        p = cur;
        cur = next[0];
    }
}

static void walk_upstream_cull_desert (
    u8* riv,
    const u8* climate,
    u16 w,
    u16 h,
    u32 start,
    u32 prev,
    u8* walk_vis,
    u32 n_max,
    u32* culled) 
{
    struct WalkFr {
        u32 m_cur;
        u32 m_prev;
    };
    WalkFr* st = new WalkFr[n_max];
    if (st == nullptr) {
        return;
    }
    u32 sp = 0;
    st[sp++] = WalkFr{start, prev};
    while (sp > 0) {
        const WalkFr f = st[--sp];
        const u32 cur = f.m_cur;
        const u32 p = f.m_prev;
        if (walk_vis[cur] != 0) {
            continue;
        }
        walk_vis[cur] = 1;
        if (riv[cur] == 0) {
            continue;
        }
        u32 next[8];
        const u32 nn = pick_riv_next(riv, w, h, cur, p, next);
        if (is_desert_climate(climate[cur])) {
            riv[cur] = 0;
            (*culled)++;
        }
        for (u32 k = 0; k < nn; ++k) {
            if (sp >= n_max) {
                break;
            }
            st[sp++] = WalkFr{next[k], cur};
        }
    }
    delete[] st;
}

static void flood_riv_from_mouths (
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u8* flooded,
    u32 n_max) 
{
    u32* st = new u32[n_max];
    if (st == nullptr) {
        return;
    }
    u32 sp = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!is_riv_mouth(terrain, riv, w, h, x, y)) {
                continue;
            }
            const u32 mi = tidx(w, x, y);
            if (riv[mi] == 0 || flooded[mi] != 0) {
                continue;
            }
            flooded[mi] = 1;
            st[sp++] = mi;
        }
    }
    while (sp > 0) {
        const u32 cur = st[--sp];
        const u16 py = static_cast<u16>(cur / static_cast<u32>(w));
        const u16 px = static_cast<u16>(cur - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 8; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx8[k];
            const i32 ny = static_cast<i32>(py) + k_dy8[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
            if (riv[ni] == 0 || flooded[ni] != 0) {
                continue;
            }
            flooded[ni] = 1;
            if (sp >= n_max) {
                break;
            }
            st[sp++] = ni;
        }
    }
    delete[] st;
}

static u32 cull_from_downstream_mouths (
    const u8* terrain,
    u8* riv,
    const u8* climate,
    u16 w,
    u16 h,
    u8* walk_vis,
    u8* flooded,
    u32 n_max) 
{
    u32 culled = 0;
    std::memset(walk_vis, 0, static_cast<size_t>(n_max));
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!is_riv_mouth(terrain, riv, w, h, x, y)) {
                continue;
            }
            const u32 mi = tidx(w, x, y);
            if (riv[mi] != 0 && is_desert_climate(climate[mi])) {
                riv[mi] = 0;
                culled++;
            }
            u32 nb[8];
            const u32 nn = pick_riv_next(riv, w, h, mi, k_prev_none, nb);
            for (u32 k = 0; k < nn; ++k) {
                walk_upstream_cull_desert(riv, climate, w, h, nb[k], mi, walk_vis, n_max, &culled);
            }
        }
    }
    std::memset(flooded, 0, static_cast<size_t>(n_max));
    flood_riv_from_mouths(terrain, riv, w, h, flooded, n_max);
    for (u32 i = 0; i < n_max; ++i) {
        if (riv[i] == 0 || flooded[i] != 0) {
            continue;
        }
        riv[i] = 0;
        culled++;
    }
    return culled;
}

static void cull_from_upstream_tips (
    const u8* terrain,
    const u8* riv,
    const u8* climate,
    u8* mark,
    u16 w,
    u16 h,
    u32 n_max) 
{
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!is_branch_tip(terrain, riv, w, h, x, y)) {
                continue;
            }
            cull_desert_run(riv, climate, mark, w, h, tidx(w, x, y), k_prev_none, n_max);
        }
    }
}

static bool apply_desert_river_cull (
    u8* riv,
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* climate,
    bool cull_from_upstream,
    u32* culled_n) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* mark = new u8[n];
    u8* walk_vis = new u8[n];
    u8* flooded = new u8[n];
    if (mark == nullptr || walk_vis == nullptr || flooded == nullptr) {
        delete[] flooded;
        delete[] walk_vis;
        delete[] mark;
        return false;
    }
    u32 cn = 0;
    if (cull_from_upstream) {
        std::memset(mark, 0, static_cast<size_t>(n));
        cull_from_upstream_tips(terrain, riv, climate, mark, w, h, n);
        for (u32 i = 0; i < n; ++i) {
            if (mark[i] == 0) {
                continue;
            }
            riv[i] = 0;
            cn++;
        }
    } else {
        cn = cull_from_downstream_mouths(terrain, riv, climate, w, h, walk_vis, flooded, n);
    }
    if (culled_n != nullptr) {
        *culled_n = cn;
    }
    delete[] flooded;
    delete[] walk_vis;
    delete[] mark;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_DesertRiverCull -
//================================================================================================================================

P1_Gen_DesertRiverCull::P1_Gen_DesertRiverCull (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_culled_n(0) {
}

bool P1_Gen_DesertRiverCull::generate (
    u8* riv,
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* climate,
    bool cull_from_upstream) 
{
    m_valid_generation = false;
    m_culled_n = 0;
    if (!p1_run_prm_ok(m_prm) || riv == nullptr || terrain == nullptr || climate == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h || w == 0 || h == 0) {
        return false;
    }
    if (!apply_desert_river_cull(riv, w, h, terrain, climate, cull_from_upstream, &m_culled_n)) {
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_DesertRiverCull::is_valid () const {
    return m_valid_generation;
}

u32 P1_Gen_DesertRiverCull::culled_n () const {
    return m_culled_n;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
