//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "p1_gen_river_lines.h"
#include "p1_wb_util.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

struct Rng32 {
    u32 m_s;
};

static void rng_seed (Rng32* g, u32 seed) {
    g->m_s = seed != 0u ? seed : 1u;
}

static u32 rng_next (Rng32* g) {
    g->m_s = g->m_s * 1664525u + 1013904223u;
    return g->m_s;
}

static void swap_u32 (u32* a, u32* b) {
    const u32 t = *a;
    *a = *b;
    *b = t;
}

static void sort_u32_by_key (u32* ord, const u32* key, u32 n) {
    for (u32 i = 1; i < n; ++i) {
        u32 j = i;
        while (j > 0 && key[ord[j]] < key[ord[j - 1]]) {
            swap_u32(&ord[j], &ord[j - 1]);
            --j;
        }
    }
}

static u32 mv_cap (u16 w, u16 h) {
    return static_cast<u32>(w) + static_cast<u32>(h) + 64u;
}

static const u16 k_dep_none = 0xFFFFu;
static const u32 k_try_n = 8u;
static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};
static const i32 k_dxd[4] = {-1, 1, -1, 1};
static const i32 k_dyd[4] = {-1, -1, 1, 1};

enum RiverMoveDir : u8 { k_mv_l = 0, k_mv_r = 1, k_mv_u = 2, k_mv_d = 3 };

struct PathCtx {
    const u16* ov;
    const u8* terrain;
    const u8* glob_wat;
    u16 w;
    u16 h;
};

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static u32 tidx (u16 w, i32 x, i32 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w) && static_cast<u32>(y) < static_cast<u32>(h);
}

static i32 abs_i32 (i32 v) { return (v < 0) ? -v : v; }
static i32 man_d (i32 x1, i32 y1, i32 x2, i32 y2) { return abs_i32(x2 - x1) + abs_i32(y2 - y1); }

static void mark_riv (
    u8* ov,
    const u8* terrain,
    const u8* glob_wat,
    u16 w,
    u16 h,
    i32 x,
    i32 y) 
{
    if (!in_map(w, h, x, y)) {
        return;
    }
    const u32 i = tidx(w, x, y);
    if (is_water(terrain[i]) && glob_wat[i] == 0) {
        return;
    }
    ov[i] = 1;
}

static void apply_mv (i32* x, i32* y, RiverMoveDir m) {
    if (m == k_mv_l) { (*x)--; }
    else if (m == k_mv_r) { (*x)++; }
    else if (m == k_mv_u) { (*y)--; }
    else { (*y)++; }
}

static bool tile_sec_link_ok (const PathCtx& c, i32 x, i32 y, u16 ia, u16 ib) {
    const u16 s = c.ov[tidx(c.w, x, y)];
    if (s == static_cast<u16>(P1_RIVER_SECTOR_NONE)) {
        return false;
    }
    return s == ia || s == ib;
}

static bool tile_sec_coast_ok (const PathCtx& c, i32 x, i32 y, u16 sid) {
    const u32 i = tidx(c.w, x, y);
    if (is_water(c.terrain[i])) {
        return true;
    }
    const u16 s = c.ov[i];
    if (s == static_cast<u16>(P1_RIVER_SECTOR_NONE)) {
        return false;
    }
    return s == sid;
}

static bool chk_path_link (
    const PathCtx& c,
    u16 ia,
    u16 ib,
    i32 x,
    i32 y,
    const RiverMoveDir* mv,
    u32 mv_n) 
{
    if (!tile_sec_link_ok(c, x, y, ia, ib)) {
        return false;
    }
    for (u32 mi = 0; mi < mv_n; ++mi) {
        apply_mv(&x, &y, mv[mi]);
        if (!in_map(c.w, c.h, x, y)) {
            return false;
        }
        if (!tile_sec_link_ok(c, x, y, ia, ib)) {
            return false;
        }
    }
    return true;
}

static bool chk_path_coast (
    const PathCtx& c,
    u16 sid,
    i32 x,
    i32 y,
    const RiverMoveDir* mv,
    u32 mv_n) 
{
    if (!tile_sec_coast_ok(c, x, y, sid)) {
        return false;
    }
    for (u32 mi = 0; mi < mv_n; ++mi) {
        apply_mv(&x, &y, mv[mi]);
        if (!in_map(c.w, c.h, x, y)) {
            return false;
        }
        if (!tile_sec_coast_ok(c, x, y, sid)) {
            return false;
        }
    }
    return true;
}

static void build_moves (RiverMoveDir* mv, u32* mv_n, u32 cap, i32 dx, i32 dy) {
    *mv_n = 0;
    for (i32 i = 0; i < abs_i32(dx); ++i) {
        if (*mv_n >= cap) {
            return;
        }
        mv[(*mv_n)++] = (dx > 0) ? k_mv_r : k_mv_l;
    }
    for (i32 i = 0; i < abs_i32(dy); ++i) {
        if (*mv_n >= cap) {
            return;
        }
        mv[(*mv_n)++] = (dy > 0) ? k_mv_d : k_mv_u;
    }
}

static void inject_lat (RiverMoveDir* mv, u32* mv_n, u32 cap) {
    if (*mv_n == 0) {
        return;
    }
    u32 cnt[4] = {0, 0, 0, 0};
    for (u32 i = 0; i < *mv_n; ++i) {
        cnt[static_cast<u8>(mv[i])]++;
    }
    const u32 tot = *mv_n;
    u32 mx = 0;
    RiverMoveDir dom = k_mv_l;
    for (u8 i = 0; i < 4; ++i) {
        if (cnt[i] > mx) {
            mx = cnt[i];
            dom = static_cast<RiverMoveDir>(i);
        }
    }
    if (mx * 100u <= tot * 70u) {
        return;
    }
    const u32 ex = tot / 10u;
    if (ex == 0) {
        return;
    }
    if (dom == k_mv_d || dom == k_mv_u) {
        for (u32 i = 0; i < ex; ++i) {
            if (*mv_n + 2 > cap) {
                return;
            }
            mv[(*mv_n)++] = k_mv_l;
            mv[(*mv_n)++] = k_mv_r;
        }
    } else {
        for (u32 i = 0; i < ex; ++i) {
            if (*mv_n + 2 > cap) {
                return;
            }
            mv[(*mv_n)++] = k_mv_u;
            mv[(*mv_n)++] = k_mv_d;
        }
    }
}

static void shuffle_mv (RiverMoveDir* out, const RiverMoveDir* mv, u32 mv_n, Rng32* rng) {
    u32* key = new u32[mv_n];
    u32* ord = new u32[mv_n];
    if (key == nullptr || ord == nullptr) {
        delete[] ord;
        delete[] key;
        return;
    }
    for (u32 i = 0; i < mv_n; ++i) {
        key[i] = rng_next(rng);
        ord[i] = i;
    }
    sort_u32_by_key(ord, key, mv_n);
    for (u32 i = 0; i < mv_n; ++i) {
        out[i] = mv[ord[i]];
    }
    delete[] ord;
    delete[] key;
}

static void paint_mv (
    u8* ov,
    const PathCtx& c,
    i32 x,
    i32 y,
    const RiverMoveDir* mv,
    u32 mv_n) 
{
    mark_riv(ov, c.terrain, c.glob_wat, c.w, c.h, x, y);
    for (u32 mi = 0; mi < mv_n; ++mi) {
        apply_mv(&x, &y, mv[mi]);
        mark_riv(ov, c.terrain, c.glob_wat, c.w, c.h, x, y);
    }
}

static bool try_scramble (
    u8* ov,
    const PathCtx& c,
    bool coast,
    u16 sec_a,
    u16 sec_b,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    Rng32* rng) 
{
    const u32 cap = mv_cap(c.w, c.h);
    RiverMoveDir* raw = new RiverMoveDir[cap];
    RiverMoveDir* ord = new RiverMoveDir[cap];
    if (raw == nullptr || ord == nullptr) {
        delete[] ord;
        delete[] raw;
        return false;
    }
    u32 raw_n = 0;
    build_moves(raw, &raw_n, cap, x2 - x1, y2 - y1);
    inject_lat(raw, &raw_n, cap);
    for (u32 t = 0; t < k_try_n; ++t) {
        shuffle_mv(ord, raw, raw_n, rng);
        const bool ok = coast
            ? chk_path_coast(c, sec_a, x1, y1, ord, raw_n)
            : chk_path_link(c, sec_a, sec_b, x1, y1, ord, raw_n);
        if (ok) {
            paint_mv(ov, c, x1, y1, ord, raw_n);
            delete[] ord;
            delete[] raw;
            return true;
        }
    }
    delete[] ord;
    delete[] raw;
    return false;
}

static void flood_dep (u16* dep, const u16* ov, u16 w, u16 h, u16 sid, i32 sx, i32 sy) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        dep[i] = k_dep_none;
    }
    if (!in_map(w, h, sx, sy) || ov[tidx(w, sx, sy)] != sid) {
        return;
    }
    u32* q = new u32[n];
    if (q == nullptr) {
        return;
    }
    u32 qn = 0;
    const u32 si = tidx(w, sx, sy);
    dep[si] = 0;
    q[qn++] = si;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 d = dep[i];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny) || ov[tidx(w, nx, ny)] != sid) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (dep[ni] != k_dep_none) {
                continue;
            }
            dep[ni] = static_cast<u16>(d + 1u);
            q[qn++] = ni;
        }
    }
    delete[] q;
}

struct WalkPick {
    i32 fx;
    i32 fy;
    i32 mx;
    i32 my;
    bool diag;
    u16 bd;
    u16 sd;
    bool ok;
};

static bool pick_better_dn (u16 nd, u16 sd, i32 md, u16 bnd, u16 bsd, i32 bmd) {
    if (nd < bnd) {
        return true;
    }
    if (nd > bnd) {
        return false;
    }
    if (sd > bsd) {
        return true;
    }
    if (sd < bsd) {
        return false;
    }
    return md < bmd;
}

static void pick_try (
    WalkPick* best,
    const u16* ov_sec,
    const u16* brd,
    const u16* sdep,
    u16 w,
    u16 h,
    u16 sid,
    u16 cd,
    i32 nx,
    i32 ny,
    i32 mx,
    i32 my,
    bool diag,
    bool flat,
    i32 tx,
    i32 ty) 
{
    if (!in_map(w, h, nx, ny) || ov_sec[tidx(w, nx, ny)] != sid) {
        return;
    }
    if (diag && (!in_map(w, h, mx, my) || ov_sec[tidx(w, mx, my)] != sid)) {
        return;
    }
    const u16 nd = brd[tidx(w, nx, ny)];
    if (nd == k_dep_none || nd > cd) {
        return;
    }
    if (!flat && nd >= cd) {
        return;
    }
    const u16 sd = sdep[tidx(w, nx, ny)];
    const i32 md = man_d(nx, ny, tx, ty);
    if (!best->ok || pick_better_dn(nd, sd, md, best->bd, best->sd, man_d(best->fx, best->fy, tx, ty))) {
        best->fx = nx;
        best->fy = ny;
        best->mx = mx;
        best->my = my;
        best->diag = diag;
        best->bd = nd;
        best->sd = sd;
        best->ok = true;
    }
}

static void walk_gather (
    WalkPick* best,
    const u16* ov_sec,
    const u16* brd,
    const u16* sdep,
    u16 w,
    u16 h,
    u16 sid,
    u16 cd,
    i32 x,
    i32 y,
    i32 tx,
    i32 ty,
    bool flat) 
{
    best->ok = false;
    for (i32 k = 0; k < 4; ++k) {
        pick_try(best, ov_sec, brd, sdep, w, h, sid, cd, x + k_dx4[k], y + k_dy4[k], x, y, false, flat, tx, ty);
    }
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = x + k_dxd[k];
        const i32 ny = y + k_dyd[k];
        const i32 c1x = x;
        const i32 c1y = ny;
        const i32 c2x = nx;
        const i32 c2y = y;
        const bool c1 = in_map(w, h, c1x, c1y) && ov_sec[tidx(w, c1x, c1y)] == sid;
        const bool c2 = in_map(w, h, c2x, c2y) && ov_sec[tidx(w, c2x, c2y)] == sid;
        if (c1 && c2) {
            const u16 s1 = sdep[tidx(w, c1x, c1y)];
            const u16 s2 = sdep[tidx(w, c2x, c2y)];
            if (s1 >= s2) {
                pick_try(best, ov_sec, brd, sdep, w, h, sid, cd, nx, ny, c1x, c1y, true, flat, tx, ty);
            } else {
                pick_try(best, ov_sec, brd, sdep, w, h, sid, cd, nx, ny, c2x, c2y, true, flat, tx, ty);
            }
        } else if (c1) {
            pick_try(best, ov_sec, brd, sdep, w, h, sid, cd, nx, ny, c1x, c1y, true, flat, tx, ty);
        } else if (c2) {
            pick_try(best, ov_sec, brd, sdep, w, h, sid, cd, nx, ny, c2x, c2y, true, flat, tx, ty);
        }
    }
}

static void walk_paint_step (
    u8* ov,
    const u8* terrain,
    const u8* glob_wat,
    u16 w,
    u16 h,
    const WalkPick& best) 
{
    if (best.diag) {
        mark_riv(ov, terrain, glob_wat, w, h, best.mx, best.my);
        mark_riv(ov, terrain, glob_wat, w, h, best.fx, best.fy);
    } else {
        mark_riv(ov, terrain, glob_wat, w, h, best.fx, best.fy);
    }
}

static bool walk_dep (
    u8* ov,
    const u16* ov_sec,
    const u16* brd,
    const u16* sdep,
    const u8* terrain,
    const u8* glob_wat,
    u16 w,
    u16 h,
    u16 sid,
    i32 x,
    i32 y,
    i32 tx,
    i32 ty) 
{
    const u32 lim = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 s = 0; s < lim; ++s) {
        mark_riv(ov, terrain, glob_wat, w, h, x, y);
        if (x == tx && y == ty) {
            return true;
        }
        const u16 cd = brd[tidx(w, x, y)];
        if (cd == k_dep_none || cd == 0) {
            return false;
        }
        WalkPick best = {};
        walk_gather(&best, ov_sec, brd, sdep, w, h, sid, cd, x, y, tx, ty, false);
        if (!best.ok) {
            walk_gather(&best, ov_sec, brd, sdep, w, h, sid, cd, x, y, tx, ty, true);
        }
        if (!best.ok) {
            return false;
        }
        walk_paint_step(ov, terrain, glob_wat, w, h, best);
        x = best.fx;
        y = best.fy;
    }
    return false;
}

static bool walk_sec_bfs (
    u8* ov,
    const u16* ov_sec,
    const u8* terrain,
    const u8* glob_wat,
    u16 w,
    u16 h,
    u16 sid,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2) 
{
    if (!in_map(w, h, x1, y1) || !in_map(w, h, x2, y2)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_vis("P1_Gen_RiverLines", "walk_bfs_vis", 0u);
    if (!wb_vis.ok()) {
        return false;
    }
    u16* vis = wb_vis.get_iter_ptr();
    u32* par = new u32[n];
    u32* q = new u32[n];
    if (par == nullptr || q == nullptr) {
        delete[] q;
        delete[] par;
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        par[i] = n;
    }
    const u32 si = tidx(w, x1, y1);
    const u32 gi = tidx(w, x2, y2);
    if (ov_sec[si] != sid || ov_sec[gi] != sid) {
        delete[] q;
        delete[] par;
        return false;
    }
    vis[si] = 1;
    u32 qn = 0;
    q[qn++] = si;
    bool hit = false;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        if (i == gi) {
            hit = true;
            break;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (ov_sec[ni] != sid || vis[ni] != 0) {
                continue;
            }
            vis[ni] = 1;
            par[ni] = i;
            q[qn++] = ni;
        }
    }
    if (!hit) {
        delete[] q;
        delete[] par;
        return false;
    }
    for (u32 c = gi; c != si; c = par[c]) {
        const u16 py = static_cast<u16>(c / static_cast<u32>(w));
        const u16 px = static_cast<u16>(c - static_cast<u32>(py) * static_cast<u32>(w));
        mark_riv(ov, terrain, glob_wat, w, h, static_cast<i32>(px), static_cast<i32>(py));
        if (par[c] >= n) {
            break;
        }
    }
    mark_riv(ov, terrain, glob_wat, w, h, x1, y1);
    delete[] q;
    delete[] par;
    return true;
}

static void paint_half (
    u8* ov,
    const u16* sec_ov,
    const u8* terrain,
    const u8* glob_wat,
    u16 w,
    u16 h,
    u16 sid,
    i32 cx,
    i32 cy,
    i32 bx,
    i32 by) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_brd("P1_Gen_RiverLines", "paint_brd", 0u);
    Whiteboard_2B wb_sdep("P1_Gen_RiverLines", "paint_sdep", 0u);
    if (!wb_brd.ok() || !wb_sdep.ok()) {
        return;
    }
    u16* brd = wb_brd.get_iter_ptr();
    u16* sdep = wb_sdep.get_iter_ptr();
    flood_dep(brd, sec_ov, w, h, sid, bx, by);
    flood_dep(sdep, sec_ov, w, h, sid, cx, cy);
    if (!walk_dep(ov, sec_ov, brd, sdep, terrain, glob_wat, w, h, sid, cx, cy, bx, by)) {
        walk_sec_bfs(ov, sec_ov, terrain, glob_wat, w, h, sid, cx, cy, bx, by);
    }
}

static bool bfs_sec_goal (
    const u16* ov,
    u16 w,
    u16 h,
    u16 sid,
    i32 sx,
    i32 sy,
    bool coast,
    const u8* terrain,
    const u8* glob_wat,
    u16 goal_sid,
    i32 ex,
    i32 ey,
    i32* gx,
    i32* gy,
    i32* gx2,
    i32* gy2) 
{
    if (!in_map(w, h, sx, sy) || ov[tidx(w, sx, sy)] != sid) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_vis("P1_Gen_RiverLines", "sec_goal_vis", 0u);
    if (!wb_vis.ok()) {
        return false;
    }
    u16* vis = wb_vis.get_iter_ptr();
    for (u32 i = 0; i < n; ++i) {
        vis[i] = 0;
    }
    u32* q = new u32[n];
    if (q == nullptr) {
        return false;
    }
    const u32 si = tidx(w, sx, sy);
    vis[si] = 1;
    u32 qn = 0;
    q[qn++] = si;
    bool ok = false;
    i32 best = -1;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            bool hit = false;
            if (coast) {
                const u32 wi = tidx(w, nx, ny);
                hit = is_water(terrain[wi]) && glob_wat[wi] != 0;
            } else {
                hit = ov[tidx(w, nx, ny)] == goal_sid;
            }
            if (hit) {
                i32 sc = man_d(px, py, sx, sy);
                if (!coast) {
                    sc += man_d(nx, ny, ex, ey);
                }
                if (best == -1 || sc < best) {
                    best = sc;
                    *gx = static_cast<i32>(px);
                    *gy = static_cast<i32>(py);
                    if (!coast) {
                        *gx2 = nx;
                        *gy2 = ny;
                    }
                    ok = true;
                }
                continue;
            }
            if (ov[tidx(w, nx, ny)] != sid) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (vis[ni] != 0) {
                continue;
            }
            vis[ni] = 1;
            q[qn++] = ni;
        }
    }
    delete[] q;
    return ok;
}

static bool paint_water (
    u8* ov,
    const u8* terrain,
    const u8* glob_wat,
    u16 w,
    u16 h,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2) 
{
    const u32 si = tidx(w, x1, y1);
    const u32 gi = tidx(w, x2, y2);
    if (!in_map(w, h, x1, y1) || !in_map(w, h, x2, y2) || glob_wat[si] == 0 || glob_wat[gi] == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_vis("P1_Gen_RiverLines", "paint_wat_vis", 0u);
    if (!wb_vis.ok()) {
        return false;
    }
    u16* vis = wb_vis.get_iter_ptr();
    u32* par = new u32[n];
    u32* q = new u32[n];
    if (par == nullptr || q == nullptr) {
        delete[] q;
        delete[] par;
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        par[i] = n;
    }
    vis[si] = 1;
    u32 qn = 0;
    q[qn++] = si;
    bool hit = false;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        if (i == gi) {
            hit = true;
            break;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (glob_wat[ni] == 0 || vis[ni] != 0) {
                continue;
            }
            vis[ni] = 1;
            par[ni] = i;
            q[qn++] = ni;
        }
    }
    if (!hit) {
        delete[] q;
        delete[] par;
        return false;
    }
    for (u32 c = gi; c != si; c = par[c]) {
        const u16 py = static_cast<u16>(c / static_cast<u32>(w));
        const u16 px = static_cast<u16>(c - static_cast<u32>(py) * static_cast<u32>(w));
        mark_riv(ov, terrain, glob_wat, w, h, static_cast<i32>(px), static_cast<i32>(py));
        if (par[c] >= n) {
            break;
        }
    }
    mark_riv(ov, terrain, glob_wat, w, h, x1, y1);
    delete[] q;
    delete[] par;
    return true;
}

static void paint_link_b (
    u8* ov,
    const u16* sec_ov,
    const u8* terrain,
    const u8* glob_wat,
    u16 w,
    u16 h,
    u16 ia,
    u16 ib,
    i32 ax,
    i32 ay,
    i32 bx,
    i32 by) 
{
    i32 bax = 0;
    i32 bay = 0;
    i32 bbx = 0;
    i32 bby = 0;
    if (!bfs_sec_goal(sec_ov, w, h, ia, ax, ay, false, terrain, glob_wat, ib, bx, by, &bax, &bay, &bbx, &bby)) {
        return;
    }
    paint_half(ov, sec_ov, terrain, glob_wat, w, h, ia, ax, ay, bax, bay);
    mark_riv(ov, terrain, glob_wat, w, h, bax, bay);
    mark_riv(ov, terrain, glob_wat, w, h, bbx, bby);
    paint_half(ov, sec_ov, terrain, glob_wat, w, h, ib, bx, by, bbx, bby);
}

static void paint_coast_b (
    u8* ov,
    const u16* sec_ov,
    const u8* terrain,
    const u8* glob_wat,
    u16 w,
    u16 h,
    u16 sid,
    i32 cx,
    i32 cy,
    i32 wx,
    i32 wy) 
{
    i32 lx = 0;
    i32 ly = 0;
    i32 dummy = 0;
    if (!bfs_sec_goal(sec_ov, w, h, sid, cx, cy, true, terrain, glob_wat, 0, wx, wy, &lx, &ly, &dummy, &dummy)) {
        return;
    }
    paint_half(ov, sec_ov, terrain, glob_wat, w, h, sid, cx, cy, lx, ly);
    mark_riv(ov, terrain, glob_wat, w, h, lx, ly);
    for (i32 k = 0; k < 4; ++k) {
        const i32 sx = lx + k_dx4[k];
        const i32 sy = ly + k_dy4[k];
        const u32 ti = tidx(w, sx, sy);
        if (in_map(w, h, sx, sy) && glob_wat[ti] != 0) {
            paint_water(ov, terrain, glob_wat, w, h, sx, sy, wx, wy);
            return;
        }
    }
}

static void paint_link (
    u8* ov,
    const PathCtx& c,
    u16 ia,
    u16 ib,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    Rng32* rng) 
{
    if (!try_scramble(ov, c, false, ia, ib, x1, y1, x2, y2, rng)) {
        paint_link_b(ov, c.ov, c.terrain, c.glob_wat, c.w, c.h, ia, ib, x1, y1, x2, y2);
    }
}

static void paint_coast (
    u8* ov,
    const PathCtx& c,
    u16 sid,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    Rng32* rng) 
{
    if (!try_scramble(ov, c, true, sid, 0, x1, y1, x2, y2, rng)) {
        paint_coast_b(ov, c.ov, c.terrain, c.glob_wat, c.w, c.h, sid, x1, y1, x2, y2);
    }
}

static bool find_nearest_water (
    const u8* terrain,
    const u8* glob_wat,
    u16 w,
    u16 h,
    i32 sx,
    i32 sy,
    i32* out_wx,
    i32* out_wy) 
{
    if (terrain == nullptr || out_wx == nullptr || out_wy == nullptr || !in_map(w, h, sx, sy)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_vis("P1_Gen_RiverLines", "near_wat_vis", 0u);
    Whiteboard_2B wb_qd("P1_Gen_RiverLines", "near_wat_qd", 0u);
    if (!wb_vis.ok() || !wb_qd.ok()) {
        return false;
    }
    u16* vis = wb_vis.get_iter_ptr();
    for (u32 i = 0; i < n; ++i) {
        vis[i] = 0;
    }
    u16* qd = wb_qd.get_iter_ptr();
    u32* q = new u32[n];
    if (q == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        qd[i] = 0xFFFFu;
    }
    const u32 si = tidx(w, sx, sy);
    vis[si] = 1;
    qd[si] = 0;
    u32 qn = 0;
    q[qn++] = si;
    i32 bwx = -1;
    i32 bwy = -1;
    i32 bd = -1;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const i32 cx = static_cast<i32>(px);
        const i32 cy = static_cast<i32>(py);
        const i32 cd = static_cast<i32>(qd[i]);
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = cx + k_dx4[d];
            const i32 ny = cy + k_dy4[d];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (glob_wat[ni] != 0) {
                if (bd == -1 || cd + 1 < bd) {
                    bd = cd + 1;
                    bwx = nx;
                    bwy = ny;
                }
                continue;
            }
            if (vis[ni] != 0) {
                continue;
            }
            vis[ni] = 1;
            qd[ni] = static_cast<u16>(cd + 1);
            q[qn++] = ni;
        }
        if (bd != -1 && cd > bd) {
            break;
        }
    }
    delete[] q;
    if (bd == -1) {
        return false;
    }
    *out_wx = bwx;
    *out_wy = bwy;
    return true;
}

static void glob_seed (const u8* terrain, u16 w, u16 h, u32 x, u32 y, u8* mask, u32* q, u32* qn) {
    if (x >= static_cast<u32>(w) || y >= static_cast<u32>(h)) {
        return;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + x;
    if (!is_water(terrain[i]) || mask[i] != 0) {
        return;
    }
    mask[i] = 1;
    q[*qn] = i;
    *qn = *qn + 1u;
}

static u8* build_glob_ocn_mask (const u8* terrain, u16 w, u16 h) {
    if (terrain == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* mask = new u8[n];
    if (mask == nullptr) {
        return nullptr;
    }
    std::memset(mask, 0, n);
    u32* q = new u32[n];
    if (q == nullptr) {
        delete[] mask;
        return nullptr;
    }
    u32 qn = 0;
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    for (u32 x = 0; x < wi; ++x) {
        glob_seed(terrain, w, h, x, 0, mask, q, &qn);
        glob_seed(terrain, w, h, x, hi - 1u, mask, q, &qn);
    }
    for (u32 y = 0; y < hi; ++y) {
        glob_seed(terrain, w, h, 0, y, mask, q, &qn);
        glob_seed(terrain, w, h, wi - 1u, y, mask, q, &qn);
    }
    for (u32 i = 0; i < n; ++i) {
        if (terrain[i] == TERR_OCEAN[0]) {
            glob_seed(terrain, w, h, i % wi, i / wi, mask, q, &qn);
        }
    }
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u32 py = i / wi;
        const u32 px = i - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + k_dx4[d];
            const i32 ny = static_cast<i32>(py) + k_dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
            if (!is_water(terrain[ni]) || mask[ni] != 0) {
                continue;
            }
            mask[ni] = 1;
            q[qn++] = ni;
        }
    }
    delete[] q;
    return mask;
}

static void flood_wat_comp (
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u32 seed,
    u8* wat,
    u32* mouths,
    u32* mouth_n,
    u32 mouth_cap,
    u32* q,
    u32 qcap) 
{
    if (wat[seed] != 0) {
        return;
    }
    u32 qn = 0;
    wat[seed] = 1;
    q[qn++] = seed;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (riv[ni] != 0) {
                if (*mouth_n < mouth_cap) {
                    mouths[(*mouth_n)++] = ni;
                }
                continue;
            }
            if (!is_water(terrain[ni]) || wat[ni] != 0) {
                continue;
            }
            if (qn >= qcap) {
                continue;
            }
            wat[ni] = 1;
            q[qn++] = ni;
        }
    }
}

static void flood_all_water (
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u8* wat,
    u32* mouths,
    u32* mouth_n,
    u32 mouth_cap) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* q = new u32[n];
    if (q == nullptr) {
        *mouth_n = 0;
        return;
    }
    std::memset(wat, 0, n);
    *mouth_n = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_water(terrain[i]) || wat[i] != 0) {
            continue;
        }
        flood_wat_comp(terrain, riv, w, h, i, wat, mouths, mouth_n, mouth_cap, q, n);
    }
    delete[] q;
}

static u16 mouth_basin (
    const u16* basin,
    const u8* riv,
    u16 w,
    u16 h,
    u16 mx,
    u16 my) 
{
    const u32 si = tidx(w, mx, my);
    if (basin[si] != static_cast<u16>(P1_RIVER_BASIN_NONE)) {
        return basin[si];
    }
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(mx) + ((k < 4) ? k_dx4[k] : k_dxd[k - 4]);
        const i32 ny = static_cast<i32>(my) + ((k < 4) ? k_dy4[k] : k_dyd[k - 4]);
        if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
            continue;
        }
        const u32 ni = tidx(w, nx, ny);
        if (riv[ni] == 0 || basin[ni] == static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            continue;
        }
        return basin[ni];
    }
    return static_cast<u16>(P1_RIVER_BASIN_NONE);
}

static void trace_sys (
    u16 w,
    u16 h,
    const u8* riv,
    const u16* basin,
    u16 mx,
    u16 my,
    u16 basin_id,
    u16 sys_ix,
    u16* rdep,
    u16* rsys,
    P1_RiverSysEntry* ent) 
{
    ent->m_mx = mx;
    ent->m_my = my;
    ent->m_max_d = 0;
    ent->m_tile_n = 0;
    const u32 si = tidx(w, mx, my);
    if (riv[si] == 0 || rdep[si] != 0) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* q = new u32[n];
    if (q == nullptr) {
        return;
    }
    u32 qn = 0;
    rdep[si] = 1;
    rsys[si] = sys_ix;
    ent->m_tile_n = 1;
    ent->m_max_d = 1;
    q[qn++] = si;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 d = rdep[i];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 8; ++k) {
            const i32 nx = static_cast<i32>(px) + ((k < 4) ? k_dx4[k] : k_dxd[k - 4]);
            const i32 ny = static_cast<i32>(py) + ((k < 4) ? k_dy4[k] : k_dyd[k - 4]);
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (riv[ni] == 0 || rdep[ni] != 0 || basin[ni] != basin_id) {
                continue;
            }
            const u16 nd = static_cast<u16>(d + 1u);
            rdep[ni] = nd;
            rsys[ni] = sys_ix;
            ent->m_tile_n++;
            if (nd > ent->m_max_d) {
                ent->m_max_d = nd;
            }
            q[qn++] = ni;
        }
    }
    delete[] q;
}

struct SysSortRec {
    u16 m_old_ix;
    P1_RiverSysEntry m_ent;
};

static bool sys_sort_cmp (const SysSortRec& a, const SysSortRec& b) {
    if (a.m_ent.m_tile_n != b.m_ent.m_tile_n) {
        return a.m_ent.m_tile_n > b.m_ent.m_tile_n;
    }
    if (a.m_ent.m_max_d != b.m_ent.m_max_d) {
        return a.m_ent.m_max_d > b.m_ent.m_max_d;
    }
    return a.m_old_ix < b.m_old_ix;
}

static void swap_sys_rec (SysSortRec* a, SysSortRec* b) {
    SysSortRec t = *a;
    *a = *b;
    *b = t;
}

static void sort_sys (u16 w, u16 h, P1_RiverSysEntry* sys, u16* rsys, u16 sys_n) {
    if (sys == nullptr || rsys == nullptr || sys_n == 0) {
        return;
    }
    SysSortRec* recs = new SysSortRec[sys_n];
    u16* remap = new u16[sys_n];
    if (recs == nullptr || remap == nullptr) {
        delete[] remap;
        delete[] recs;
        return;
    }
    for (u16 i = 0; i < sys_n; ++i) {
        recs[i].m_old_ix = i;
        recs[i].m_ent = sys[i];
    }
    for (u16 i = 1; i < sys_n; ++i) {
        u16 j = i;
        while (j > 0 && sys_sort_cmp(recs[j], recs[j - 1])) {
            swap_sys_rec(&recs[j], &recs[j - 1]);
            --j;
        }
    }
    for (u16 i = 0; i < sys_n; ++i) {
        sys[i] = recs[i].m_ent;
        remap[recs[i].m_old_ix] = i;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (rsys[i] == P1_RIVER_LINE_SYS_NONE) {
            continue;
        }
        rsys[i] = remap[rsys[i]];
    }
    delete[] remap;
    delete[] recs;
}

static bool build_riv_data (
    const u8* terrain,
    const u8* riv,
    const u16* basin,
    u16 w,
    u16 h,
    P1_Gen_RiverLinesRslt* out) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    out->m_rdep = new u16[n];
    out->m_rsys = new u16[n];
    u8* wat = new u8[n];
    if (out->m_rdep == nullptr || out->m_rsys == nullptr || wat == nullptr) {
        delete[] wat;
        delete[] out->m_rsys;
        delete[] out->m_rdep;
        out->m_rsys = nullptr;
        out->m_rdep = nullptr;
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        out->m_rdep[i] = 0;
        out->m_rsys[i] = P1_RIVER_LINE_SYS_NONE;
    }
    u32* mouths = new u32[n];
    P1_RiverSysEntry* tmp = new P1_RiverSysEntry[n];
    if (mouths == nullptr || tmp == nullptr) {
        delete[] tmp;
        delete[] mouths;
        delete[] wat;
        delete[] out->m_rsys;
        delete[] out->m_rdep;
        out->m_rsys = nullptr;
        out->m_rdep = nullptr;
        return false;
    }
    u32 mouth_n = 0;
    flood_all_water(terrain, riv, w, h, wat, mouths, &mouth_n, n);
    delete[] wat;
    u16 tmp_n = 0;
    for (u32 mi = 0; mi < mouth_n; ++mi) {
        const u32 i = mouths[mi];
        if (out->m_rdep[i] != 0) {
            continue;
        }
        const u16 px = static_cast<u16>(i % static_cast<u32>(w));
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 basin_id = mouth_basin(basin, riv, w, h, px, py);
        if (basin_id == static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            continue;
        }
        P1_RiverSysEntry ent = {};
        const u16 sys_ix = tmp_n;
        trace_sys(w, h, riv, basin, px, py, basin_id, sys_ix, out->m_rdep, out->m_rsys, &ent);
        if (ent.m_tile_n == 0) {
            continue;
        }
        tmp[tmp_n++] = ent;
    }
    delete[] mouths;
    out->m_sys_n = tmp_n;
    if (out->m_sys_n == 0) {
        delete[] tmp;
        out->m_sys = nullptr;
        return true;
    }
    out->m_sys = new P1_RiverSysEntry[out->m_sys_n];
    if (out->m_sys == nullptr) {
        delete[] tmp;
        delete[] out->m_rsys;
        delete[] out->m_rdep;
        out->m_rsys = nullptr;
        out->m_rdep = nullptr;
        return false;
    }
    for (u16 i = 0; i < out->m_sys_n; ++i) {
        out->m_sys[i] = tmp[i];
    }
    delete[] tmp;
    sort_sys(w, h, out->m_sys, out->m_rsys, out->m_sys_n);
    return true;
}

static bool paint_riv_lines (
    u8* ov,
    u16 w,
    u16 h,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverNetworkRslt& network,
    const u8* terrain,
    u32 seed) 
{
    u8* glob = build_glob_ocn_mask(terrain, w, h);
    if (glob == nullptr) {
        return false;
    }
    Rng32 rng;
    rng_seed(&rng, seed);
    PathCtx ctx = {};
    ctx.ov = sectors.m_ov;
    ctx.terrain = terrain;
    ctx.glob_wat = glob;
    ctx.w = w;
    ctx.h = h;
    for (u16 ci = 0; ci < network.m_conn_n; ++ci) {
        const u16 ia = network.m_conns[ci].m_a;
        const u16 ib = network.m_conns[ci].m_b;
        if (ia >= sectors.m_sector_n || ib >= sectors.m_sector_n) {
            continue;
        }
        paint_link(
            ov,
            ctx,
            ia,
            ib,
            static_cast<i32>(sectors.m_nodes[ia].m_cx),
            static_cast<i32>(sectors.m_nodes[ia].m_cy),
            static_cast<i32>(sectors.m_nodes[ib].m_cx),
            static_cast<i32>(sectors.m_nodes[ib].m_cy),
            &rng);
    }
    for (u32 si = 0; si < static_cast<u32>(network.m_sector_n); ++si) {
        if (!network.m_coastal[si] || network.m_river_sys[si] == static_cast<u16>(P1_RIVER_SYS_NONE)) {
            continue;
        }
        i32 wx = 0;
        i32 wy = 0;
        const i32 x1 = static_cast<i32>(sectors.m_nodes[si].m_cx);
        const i32 y1 = static_cast<i32>(sectors.m_nodes[si].m_cy);
        if (!find_nearest_water(terrain, glob, w, h, x1, y1, &wx, &wy)) {
            continue;
        }
        paint_coast(ov, ctx, static_cast<u16>(si), x1, y1, wx, wy, &rng);
    }
    delete[] glob;
    return true;
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr || wi == 0 || hi == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0;
    *g = 0;
    *b = 0;
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

//================================================================================================================================
//=> - P1_Gen_RiverLines -
//================================================================================================================================

P1_Gen_RiverLines::P1_Gen_RiverLines (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

P1_Gen_RiverLines::~P1_Gen_RiverLines () {
    clear_rslt();
}

void P1_Gen_RiverLines::clear_rslt () {
    delete[] m_rslt.m_ov;
    delete[] m_rslt.m_sys;
    delete[] m_rslt.m_rdep;
    delete[] m_rslt.m_rsys;
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

bool P1_Gen_RiverLines::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverNetworkRslt& network) 
{
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h) || sectors.m_ov == nullptr || sectors.m_nodes == nullptr) {
        return false;
    }
    if (network.m_river_sys == nullptr || network.m_coastal == nullptr || network.m_ov == nullptr) {
        return false;
    }
    if (network.m_conn_n > 0 && network.m_conns == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_rslt.m_ov = new u8[n];
    if (m_rslt.m_ov == nullptr) {
        clear_rslt();
        return false;
    }
    std::memset(m_rslt.m_ov, 0, n);
    if (!paint_riv_lines(m_rslt.m_ov, w, h, sectors, network, terrain, m_prm.m_seed)) {
        clear_rslt();
        return false;
    }
    if (!build_riv_data(terrain, m_rslt.m_ov, network.m_ov, w, h, &m_rslt)) {
        clear_rslt();
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RiverLines::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverLinesRslt& P1_Gen_RiverLines::result () const {
    return m_rslt;
}

void P1_Gen_RiverLines::save_output (cstr path, const u8* terrain) const {
    if (!m_valid_generation || path == nullptr || terrain == nullptr || m_rslt.m_ov == nullptr) {
        return;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        terr_rgb(terrain[i], &r, &g, &b);
        if (m_rslt.m_ov[i] != 0) {
            r = 0;
            g = 0;
            b = 255;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
