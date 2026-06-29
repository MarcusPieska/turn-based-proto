//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "explore_distant_mk1.h"
#include "ai_whiteboard.h"
#include "game_map_defs.h"
#include "runtime_trace_dbg.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_dep_none = 0xFFFFu;
static const u8 k_lim_cd = 12u;
static const u8 k_follow_max = 24u;
static const i16 k_dx4[] = {0, 1, 0, -1};
static const i16 k_dy4[] = {-1, 0, 1, 0};
#define EDM1_PHASE_GO 0
#define EDM1_PHASE_FOLLOW 1

#define EDM1_LIM_NONE 0
#define EDM1_LIM_WATER 1
#define EDM1_LIM_MTN 2
#define EDM1_LIM_BORDER 3



//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

struct EdSt {
    u8 m_phase;
    u8 m_lim;
    u16 m_tgt_x;
    u16 m_tgt_y;
    u16 m_prev_x;
    u16 m_prev_y;
    u16* m_edge_x;
    u16* m_edge_y;
    u16 m_edge_n;
    u16 m_edge_i;
    u8 m_lim_cd;
    u8 m_follow_n;
    u32 m_rng;
};

static u32 rng_next (u32* s) {
    *s = (*s * 1103515245u) + 12345u;
    return (*s >> 16u) & 0x7fffu;
}

static u32 dist_man (u16 x0, u16 y0, u16 x1, u16 y1) {
    const u32 dx = (x0 > x1) ? static_cast<u32>(x0 - x1) : static_cast<u32>(x1 - x0);
    const u32 dy = (y0 > y1) ? static_cast<u32>(y0 - y1) : static_cast<u32>(y1 - y0);
    return dx + dy;
}

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static bool is_mtn (u8 t) {
    return t == TERR_MOUNTAINS[0];
}

static bool is_border (u16 x, u16 y, u16 w, u16 h) {
    return x == 0u || y == 0u || x + 1u >= w || y + 1u >= h;
}

static bool is_walk (u8 t) {
    if (is_water(t) || is_mtn(t)) {
        return false;
    }
    return true;
}

static bool in_map (u16 x, u16 y, u16 w, u16 h) {
    return x < w && y < h;
}

static void pick_edge (const GameArraySimple& map, u32* rng, u16& ox, u16& oy) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 per = static_cast<u32>(w) * 2u + static_cast<u32>(h) * 2u - 4u;
    u32 pick = (per > 0u) ? (rng_next(rng) % per) : 0u;
    if (pick < static_cast<u32>(w)) {
        ox = static_cast<u16>(pick);
        oy = 0u;
        return;
    }
    pick -= static_cast<u32>(w);
    if (pick < static_cast<u32>(h) - 1u) {
        ox = static_cast<u16>(w - 1u);
        oy = static_cast<u16>(pick + 1u);
        return;
    }
    pick -= static_cast<u32>(h) - 1u;
    if (pick < static_cast<u32>(w) - 1u) {
        ox = static_cast<u16>(w - 2u - pick);
        oy = static_cast<u16>(h - 1u);
        return;
    }
    pick -= static_cast<u32>(w) - 1u;
    ox = 0u;
    oy = static_cast<u16>(h - 2u - pick);
}

static bool has_water_nbr (const GameArraySimple& map, u16 x, u16 y) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u32 i = 0; i < 4u; ++i) {
        const i32 nx = static_cast<i32>(x) + k_dx4[i];
        const i32 ny = static_cast<i32>(y) + k_dy4[i];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!in_map(ux, uy, w, h)) {
            continue;
        }
        if (is_water(map.get_terrain(ux, uy))) {
            return true;
        }
    }
    return false;
}

static bool is_shore (const GameArraySimple& map, u16 x, u16 y) {
    return is_walk(map.get_terrain(x, y)) && has_water_nbr(map, x, y);
}

static bool is_feat_tile (const GameArraySimple& map, u8 lim, u16 x, u16 y, bool water_feat) {
    const u16 w = map.width();
    const u16 h = map.height();
    if (lim == EDM1_LIM_BORDER) {
        return is_border(x, y, w, h);
    }
    if (lim == EDM1_LIM_MTN) {
        return is_mtn(map.get_terrain(x, y));
    }
    if (lim == EDM1_LIM_WATER) {
        if (water_feat) {
            return is_water(map.get_terrain(x, y));
        }
        return is_shore(map, x, y);
    }
    return false;
}

static u8 sense_lim_see (const GameArraySimple& map, u16 x, u16 y) {
    const u16 w = map.width();
    const u16 h = map.height();
    if (is_border(x, y, w, h)) {
        return EDM1_LIM_BORDER;
    }
    const u8 t = map.get_terrain(x, y);
    if (is_mtn(t)) {
        return EDM1_LIM_MTN;
    }
    if (is_water(t) || is_shore(map, x, y)) {
        return EDM1_LIM_WATER;
    }
    return EDM1_LIM_NONE;
}

static u8 reveal (
    const GameArraySimple& map, MapBitOverlay& explored, u16 cx, u16 cy, u16 sight,
    u8 player, u16* o_lx, u16* o_ly) {
    const u16 w = map.width();
    const u16 h = map.height();
    u8 lim = EDM1_LIM_NONE;
    for (i16 dy = -static_cast<i16>(sight); dy <= static_cast<i16>(sight); ++dy) {
        for (i16 dx = -static_cast<i16>(sight); dx <= static_cast<i16>(sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            const i32 md = (adx > ady) ? adx : ady;
            if (static_cast<u32>(md) > static_cast<u32>(sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(cx) + static_cast<i32>(dx);
            const i32 yi = static_cast<i32>(cy) + static_cast<i32>(dy);
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 x = static_cast<u16>(xi);
            const u16 y = static_cast<u16>(yi);
            if (!in_map(x, y, w, h)) {
                continue;
            }
            if (explored.get(x, y) != 0u) {
                continue;
            }
            explored.set(x, y);
            TRACE_EXPLORE_DISCOVER((x, y, static_cast<u16>(player)));
            if (lim == EDM1_LIM_NONE) {
                const u8 t = sense_lim_see(map, x, y);
                if (t != EDM1_LIM_NONE) {
                    lim = t;
                    *o_lx = x;
                    *o_ly = y;
                }
            }
        }
    }
    return lim;
}

static void free_edge (EdSt* st) {
    delete[] st->m_edge_x;
    delete[] st->m_edge_y;
    st->m_edge_x = nullptr;
    st->m_edge_y = nullptr;
    st->m_edge_n = 0;
    st->m_edge_i = 0;
}

static void mark_comp (u16* cmp, const GameArraySimple& map, u8 lim, u16 sx, u16 sy, bool water_feat, u16 w, u16 h, u32 tile_n) {
    for (u32 i = 0; i < tile_n; ++i) {
        cmp[i] = 0;
    }
    if (!is_feat_tile(map, lim, sx, sy, water_feat)) {
        return;
    }
    u32* q = new u32[tile_n];
    if (q == nullptr) {
        return;
    }
    u32 qn = 0;
    const u32 si = tidx(w, sx, sy);
    cmp[si] = 1;
    q[qn++] = si;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (u32 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!in_map(ux, uy, w, h)) {
                continue;
            }
            if (!is_feat_tile(map, lim, ux, uy, water_feat)) {
                continue;
            }
            const u32 ni = tidx(w, ux, uy);
            if (cmp[ni] != 0) {
                continue;
            }
            cmp[ni] = 1;
            q[qn++] = ni;
        }
    }
    delete[] q;
}

static bool is_comp_edge (const u16* cmp, u16 w, u16 h, u16 x, u16 y) {
    if (cmp[tidx(w, x, y)] == 0) {
        return false;
    }
    for (u32 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (nx < 0 || ny < 0) {
            return true;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!in_map(ux, uy, w, h)) {
            return true;
        }
        if (cmp[tidx(w, ux, uy)] == 0) {
            return true;
        }
    }
    return false;
}

static u16 flood_lim_edge (u16* dep, u16* edg, const u16* cmp, u16 w, u16 h, u32 tile_n) {
    for (u32 i = 0; i < tile_n; ++i) {
        dep[i] = k_dep_none;
        edg[i] = 0;
    }
    u32* q = new u32[tile_n];
    if (q == nullptr) {
        return 0;
    }
    u32 qn = 0;
    u16 edge_n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!is_comp_edge(cmp, w, h, x, y)) {
                continue;
            }
            const u32 i = tidx(w, x, y);
            dep[i] = 0;
            edg[i] = 1;
            edge_n++;
            q[qn++] = i;
        }
    }
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 d = dep[i];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (u32 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!in_map(ux, uy, w, h)) {
                continue;
            }
            const u32 ni = tidx(w, ux, uy);
            if (cmp[ni] == 0 || dep[ni] != k_dep_none) {
                continue;
            }
            dep[ni] = static_cast<u16>(d + 1u);
            q[qn++] = ni;
        }
    }
    delete[] q;
    return edge_n;
}

static u16 add_waypt (u16* wpx, u16* wpy, u16* wpm, u16 w, u16 x, u16 y, u16 wp_n) {
    const u32 i = tidx(w, x, y);
    if (wpm[i] != 0) {
        return wp_n;
    }
    wpm[i] = 1;
    wpx[wp_n] = x;
    wpy[wp_n] = y;
    return static_cast<u16>(wp_n + 1u);
}

static u16 collect_waypts (u16* wpx, u16* wpy, u16* wpm, const u16* edg, const GameArraySimple& map, u16 w, u16 h, u32 tile_n) {
    for (u32 i = 0; i < tile_n; ++i) {
        wpm[i] = 0;
    }
    u16 wp_n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (edg[tidx(w, x, y)] == 0) {
                continue;
            }
            if (is_walk(map.get_terrain(x, y))) {
                wp_n = add_waypt(wpx, wpy, wpm, w, x, y, wp_n);
                continue;
            }
            for (u32 k = 0; k < 4u; ++k) {
                const i32 nx = static_cast<i32>(x) + k_dx4[k];
                const i32 ny = static_cast<i32>(y) + k_dy4[k];
                if (nx < 0 || ny < 0) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                if (!in_map(ux, uy, w, h)) {
                    continue;
                }
                if (!is_walk(map.get_terrain(ux, uy))) {
                    continue;
                }
                wp_n = add_waypt(wpx, wpy, wpm, w, ux, uy, wp_n);
            }
        }
    }
    return wp_n;
}

static void chain_waypts (u16* ox, u16* oy, const u16* tx, const u16* ty, u16 n, u16 ux, u16 uy) {
    if (n == 0) {
        return;
    }
    bool* used = new bool[n];
    if (used == nullptr) {
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        used[i] = false;
    }
    u16 cur = 0;
    u32 best = 0xFFFFFFFFu;
    for (u16 i = 0; i < n; ++i) {
        const u32 d = dist_man(tx[i], ty[i], ux, uy);
        if (d < best) {
            best = d;
            cur = i;
        }
    }
    ox[0] = tx[cur];
    oy[0] = ty[cur];
    used[cur] = true;
    for (u16 o = 1; o < n; ++o) {
        best = 0xFFFFFFFFu;
        u16 pick = cur;
        for (u16 i = 0; i < n; ++i) {
            if (used[i]) {
                continue;
            }
            const u32 d = dist_man(tx[i], ty[i], tx[cur], ty[cur]);
            if (d < best) {
                best = d;
                pick = i;
            }
        }
        cur = pick;
        used[cur] = true;
        ox[o] = tx[cur];
        oy[o] = ty[cur];
    }
    delete[] used;
}

static bool build_edge_list (EdSt* st, const GameArraySimple& map, u16 sx, u16 sy, u16 ux, u16 uy) {
    free_edge(st);
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    const bool water_feat = is_water(map.get_terrain(sx, sy));
    AiWbSheet cmp(static_cast<i32>(tile_n));
    AiWbSheet dep(static_cast<i32>(tile_n));
    AiWbSheet edg(static_cast<i32>(tile_n));
    if (!cmp.ok() || !dep.ok() || !edg.ok()) {
        return false;
    }
    mark_comp(cmp.get(), map, st->m_lim, sx, sy, water_feat, w, h, tile_n);
    const u16 edge_n = flood_lim_edge(dep.get(), edg.get(), cmp.get(), w, h, tile_n);
    if (edge_n == 0) {
        return false;
    }
    u16* wpx = new u16[edge_n * 4u];
    u16* wpy = new u16[edge_n * 4u];
    if (wpx == nullptr || wpy == nullptr) {
        delete[] wpx;
        delete[] wpy;
        return false;
    }
    const u16 wp_n = collect_waypts(wpx, wpy, dep.get(), edg.get(), map, w, h, tile_n);
    if (wp_n == 0) {
        delete[] wpx;
        delete[] wpy;
        return false;
    }
    st->m_edge_x = new u16[wp_n];
    st->m_edge_y = new u16[wp_n];
    if (st->m_edge_x == nullptr || st->m_edge_y == nullptr) {
        delete[] wpx;
        delete[] wpy;
        free_edge(st);
        return false;
    }
    chain_waypts(st->m_edge_x, st->m_edge_y, wpx, wpy, wp_n, ux, uy);
    st->m_edge_n = wp_n;
    st->m_edge_i = 0;
    delete[] wpx;
    delete[] wpy;
    return true;
}

static bool begin_follow (EdSt* st, const GameArraySimple& map, u8 lim, u16 sx, u16 sy, u16 ux, u16 uy) {
    st->m_phase = EDM1_PHASE_FOLLOW;
    st->m_lim = lim;
    return build_edge_list(st, map, sx, sy, ux, uy);
}

static void new_distant (EdSt* st, const GameArraySimple& map) {
    free_edge(st);
    pick_edge(map, &st->m_rng, st->m_tgt_x, st->m_tgt_y);
    st->m_phase = EDM1_PHASE_GO;
    st->m_lim = EDM1_LIM_NONE;
}

static void end_follow (EdSt* st, const GameArraySimple& map) {
    new_distant(st, map);
    st->m_lim_cd = k_lim_cd;
    st->m_follow_n = 0;
}

static bool pick_walk_nbr (
    EdSt* st,
    const GameArraySimple& map,
    u16 ux,
    u16 uy,
    u16& ox,
    u16& oy) {
    const u16 w = map.width();
    const u16 h = map.height();
    u16 cx[4];
    u16 cy[4];
    u32 n = 0;
    for (u32 i = 0; i < 4u; ++i) {
        const i32 nx = static_cast<i32>(ux) + k_dx4[i];
        const i32 ny = static_cast<i32>(uy) + k_dy4[i];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (!in_map(tx, ty, w, h)) {
            continue;
        }
        if (!is_walk(map.get_terrain(tx, ty))) {
            continue;
        }
        cx[n] = tx;
        cy[n] = ty;
        n++;
    }
    if (n == 0u) {
        return false;
    }
    const u32 start = (n > 1u) ? (rng_next(&st->m_rng) % n) : 0u;
    for (u32 t = 0; t < n; ++t) {
        const u32 i = (start + t) % n;
        if (cx[i] != st->m_prev_x || cy[i] != st->m_prev_y) {
            ox = cx[i];
            oy = cy[i];
            return true;
        }
    }
    ox = cx[0];
    oy = cy[0];
    return true;
}

static bool step_go (EdSt* st, const GameArraySimple& map, MapBitOverlay& exp, u16& ux, u16& uy) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 best = 0xFFFFFFFFu;
    u16 bx = ux;
    u16 by = uy;
    bool found = false;
    for (u32 i = 0; i < 4u; ++i) {
        const i32 nx = static_cast<i32>(ux) + k_dx4[i];
        const i32 ny = static_cast<i32>(uy) + k_dy4[i];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (!in_map(tx, ty, w, h)) {
            continue;
        }
        if (!is_walk(map.get_terrain(tx, ty))) {
            continue;
        }
        const u32 d = dist_man(tx, ty, st->m_tgt_x, st->m_tgt_y);
        if (d < best) {
            best = d;
            bx = tx;
            by = ty;
            found = true;
        }
    }
    if (found && (bx != ux || by != uy)) {
        st->m_prev_x = ux;
        st->m_prev_y = uy;
        ux = bx;
        uy = by;
        return true;
    }
    best = 0xFFFFFFFFu;
    found = false;
    for (u32 i = 0; i < 4u; ++i) {
        const i32 nx = static_cast<i32>(ux) + k_dx4[i];
        const i32 ny = static_cast<i32>(uy) + k_dy4[i];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (!in_map(tx, ty, w, h)) {
            continue;
        }
        if (!is_walk(map.get_terrain(tx, ty))) {
            continue;
        }
        if (exp.get(tx, ty) != 0u) {
            continue;
        }
        const u32 d = dist_man(tx, ty, st->m_tgt_x, st->m_tgt_y);
        if (d < best) {
            best = d;
            bx = tx;
            by = ty;
            found = true;
        }
    }
    if (found && (bx != ux || by != uy)) {
        st->m_prev_x = ux;
        st->m_prev_y = uy;
        ux = bx;
        uy = by;
        return true;
    }
    if (pick_walk_nbr(st, map, ux, uy, bx, by) && (bx != ux || by != uy)) {
        st->m_prev_x = ux;
        st->m_prev_y = uy;
        ux = bx;
        uy = by;
        return true;
    }
    return false;
}

static bool step_follow_edge (EdSt* st, const GameArraySimple& map, u16& ux, u16& uy) {
    if (st->m_edge_n == 0) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    u16 tries = 0;
    while (tries < st->m_edge_n) {
        if (st->m_edge_i >= st->m_edge_n) {
            return false;
        }
        const u16 tx = st->m_edge_x[st->m_edge_i];
        const u16 ty = st->m_edge_y[st->m_edge_i];
        if (dist_man(ux, uy, tx, ty) <= 1u) {
            st->m_edge_i++;
            if (st->m_edge_i >= st->m_edge_n) {
                return false;
            }
            tries = 0;
            continue;
        }
        u32 best = 0xFFFFFFFFu;
        u16 bx = ux;
        u16 by = uy;
        bool found = false;
        for (u32 i = 0; i < 4u; ++i) {
            const i32 nx = static_cast<i32>(ux) + k_dx4[i];
            const i32 ny = static_cast<i32>(uy) + k_dy4[i];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 cx = static_cast<u16>(nx);
            const u16 cy = static_cast<u16>(ny);
            if (!in_map(cx, cy, w, h)) {
                continue;
            }
            if (!is_walk(map.get_terrain(cx, cy))) {
                continue;
            }
            const u32 d = dist_man(cx, cy, tx, ty);
            if (d < best) {
                best = d;
                bx = cx;
                by = cy;
                found = true;
            }
        }
        if (found && (bx != ux || by != uy)) {
            st->m_prev_x = ux;
            st->m_prev_y = uy;
            ux = bx;
            uy = by;
            return true;
        }
        st->m_edge_i++;
        tries++;
    }
    return false;
}

static bool try_lim_see (EdSt* st, const GameArraySimple& map, u8 lim, u16 sx, u16 sy, u16 ux, u16 uy) {
    if (lim == EDM1_LIM_NONE || st->m_phase != EDM1_PHASE_GO || st->m_lim_cd > 0u) {
        return false;
    }
    if (!begin_follow(st, map, lim, sx, sy, ux, uy)) {
        end_follow(st, map);
        return false;
    }
    st->m_follow_n = 0;
    return true;
}

//================================================================================================================================
//=> - ExploreAi -
//================================================================================================================================

ExploreAi::ExploreAi (
    const GameArraySimple& map,
    MapBitOverlay& explored,
    u16 sx,
    u16 sy,
    u16 sight,
    u8 player) :
    m_map(map),
    m_exp(explored),
    m_x(sx),
    m_y(sy),
    m_sx(sx),
    m_sy(sy),
    m_sight(sight),
    m_player(player) {}

ExploreAi::~ExploreAi () {}

u16 ExploreAi::x () const {
    return m_x;
}

u16 ExploreAi::y () const {
    return m_y;
}

u16 ExploreAi::sx () const {
    return m_sx;
}

u16 ExploreAi::sy () const {
    return m_sy;
}

//================================================================================================================================
//=> - ExploreDistantMk1 -
//================================================================================================================================

ExploreDistantMk1::ExploreDistantMk1 (
    const GameArraySimple& map,
    MapBitOverlay& explored,
    u16 sx,
    u16 sy,
    u32 seed,
    u16 sight,
    u8 player) :
    ExploreAi(map, explored, sx, sy, sight, player),
    m_st(static_cast<void*>(new EdSt())) {
    EdSt* st = static_cast<EdSt*>(m_st);
    st->m_phase = EDM1_PHASE_GO;
    st->m_lim = EDM1_LIM_NONE;
    st->m_prev_x = 0;
    st->m_prev_y = 0;
    st->m_edge_x = nullptr;
    st->m_edge_y = nullptr;
    st->m_edge_n = 0;
    st->m_edge_i = 0;
    st->m_lim_cd = 0;
    st->m_follow_n = 0;
    st->m_rng = seed;
    new_distant(st, m_map);
}

ExploreDistantMk1::~ExploreDistantMk1 () {
    EdSt* st = static_cast<EdSt*>(m_st);
    free_edge(st);
    delete st;
    m_st = nullptr;
}

u8 ExploreDistantMk1::phase () const {
    return static_cast<EdSt*>(m_st)->m_phase;
}

u8 ExploreDistantMk1::lim () const {
    return static_cast<EdSt*>(m_st)->m_lim;
}

void ExploreDistantMk1::move (u16 moves) {
    EdSt* st = static_cast<EdSt*>(m_st);
    st->m_prev_x = m_x;
    st->m_prev_y = m_y;
    u16 lx = m_x;
    u16 ly = m_y;
    const u8 lim0 = reveal(m_map, m_exp, m_x, m_y, m_sight, m_player, &lx, &ly);
    if (lim0 != EDM1_LIM_NONE) {
        try_lim_see(st, m_map, lim0, lx, ly, m_x, m_y);
    }
    for (u16 m = 0; m < moves; ++m) {
        if (st->m_phase == EDM1_PHASE_FOLLOW) {
            if (step_follow_edge(st, m_map, m_x, m_y)) {
                st->m_follow_n++;
                reveal(m_map, m_exp, m_x, m_y, m_sight, m_player, &lx, &ly);
                if (st->m_follow_n < k_follow_max) {
                    continue;
                }
            }
            end_follow(st, m_map);
        }
        if (st->m_phase == EDM1_PHASE_GO) {
            if (!step_go(st, m_map, m_exp, m_x, m_y)) {
                new_distant(st, m_map);
                continue;
            }
            if (st->m_lim_cd > 0u) {
                st->m_lim_cd--;
            }
            lx = m_x;
            ly = m_y;
            const u8 lim = reveal(m_map, m_exp, m_x, m_y, m_sight, m_player, &lx, &ly);
            if (lim != EDM1_LIM_NONE) {
                try_lim_see(st, m_map, lim, lx, ly, m_x, m_y);
                continue;
            }
            if (m_x == st->m_tgt_x && m_y == st->m_tgt_y) {
                new_distant(st, m_map);
            }
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
