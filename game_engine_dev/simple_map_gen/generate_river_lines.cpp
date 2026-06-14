//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "generate_river_lines.h"

#include "generate_global_ocean.h"
#include "generator_constants.h"
#include "generator_whiteboard.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

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
    if (s == static_cast<u16>(RIVER_SECTOR_NONE)) {
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
    if (s == static_cast<u16>(RIVER_SECTOR_NONE)) {
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
    const std::vector<RiverMoveDir>& mv) 
{
    if (!tile_sec_link_ok(c, x, y, ia, ib)) {
        return false;
    }
    for (RiverMoveDir m : mv) {
        apply_mv(&x, &y, m);
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
    const std::vector<RiverMoveDir>& mv) 
{
    if (!tile_sec_coast_ok(c, x, y, sid)) {
        return false;
    }
    for (RiverMoveDir m : mv) {
        apply_mv(&x, &y, m);
        if (!in_map(c.w, c.h, x, y)) {
            return false;
        }
        if (!tile_sec_coast_ok(c, x, y, sid)) {
            return false;
        }
    }
    return true;
}

static void build_moves (std::vector<RiverMoveDir>* mv, i32 dx, i32 dy) {
    mv->clear();
    for (i32 i = 0; i < abs_i32(dx); ++i) {
        mv->push_back((dx > 0) ? k_mv_r : k_mv_l);
    }
    for (i32 i = 0; i < abs_i32(dy); ++i) {
        mv->push_back((dy > 0) ? k_mv_d : k_mv_u);
    }
}

static void inject_lat (std::vector<RiverMoveDir>* mv) {
    if (mv->empty()) {
        return;
    }
    u32 cnt[4] = {0, 0, 0, 0};
    for (RiverMoveDir m : *mv) {
        cnt[static_cast<u8>(m)]++;
    }
    const u32 tot = static_cast<u32>(mv->size());
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
        for (u32 i = 0; i < ex; ++i) { mv->push_back(k_mv_l); mv->push_back(k_mv_r); }
    } else {
        for (u32 i = 0; i < ex; ++i) { mv->push_back(k_mv_u); mv->push_back(k_mv_d); }
    }
}

static void shuffle_mv (std::vector<RiverMoveDir>* out, const std::vector<RiverMoveDir>& mv) {
    out->resize(mv.size());
    std::vector<u32> key(mv.size());
    for (u32 i = 0; i < mv.size(); ++i) {
        key[i] = static_cast<u32>(std::rand());
    }
    std::vector<u32> ord(mv.size());
    for (u32 i = 0; i < mv.size(); ++i) {
        ord[i] = i;
    }
    std::sort(ord.begin(), ord.end(), [&](u32 a, u32 b) { return key[a] < key[b]; });
    for (u32 i = 0; i < mv.size(); ++i) {
        (*out)[i] = mv[ord[i]];
    }
}

static void paint_mv (
    u8* ov,
    const PathCtx& c,
    i32 x,
    i32 y,
    const std::vector<RiverMoveDir>& mv) 
{
    mark_riv(ov, c.terrain, c.glob_wat, c.w, c.h, x, y);
    for (RiverMoveDir m : mv) {
        apply_mv(&x, &y, m);
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
    i32 y2) 
{
    std::vector<RiverMoveDir> raw;
    std::vector<RiverMoveDir> ord;
    build_moves(&raw, x2 - x1, y2 - y1);
    inject_lat(&raw);
    for (u32 t = 0; t < k_try_n; ++t) {
        shuffle_mv(&ord, raw);
        const bool ok = coast
            ? chk_path_coast(c, sec_a, x1, y1, ord)
            : chk_path_link(c, sec_a, sec_b, x1, y1, ord);
        if (ok) {
            paint_mv(ov, c, x1, y1, ord);
            return true;
        }
    }
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
    std::vector<u32> q;
    const u32 si = tidx(w, sx, sy);
    dep[si] = 0;
    q.push_back(si);
    for (std::size_t qi = 0; qi < q.size(); ++qi) {
        const u32 i = q[qi];
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
            q.push_back(ni);
        }
    }
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
    u16* vis = GeneratorWhiteboard::alloc(static_cast<i32>(n));
    if (vis == nullptr) {
        return false;
    }
    std::vector<u32> par(n, n);
    std::vector<u32> q;
    const u32 si = tidx(w, x1, y1);
    const u32 gi = tidx(w, x2, y2);
    if (ov_sec[si] != sid || ov_sec[gi] != sid) {
        GeneratorWhiteboard::release(vis);
        return false;
    }
    vis[si] = 1;
    q.push_back(si);
    bool hit = false;
    for (std::size_t qi = 0; qi < q.size(); ++qi) {
        const u32 i = q[qi];
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
            q.push_back(ni);
        }
    }
    if (!hit) {
        GeneratorWhiteboard::release(vis);
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
    GeneratorWhiteboard::release(vis);
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
    u16* brd = GeneratorWhiteboard::alloc(static_cast<i32>(n));
    u16* sdep = GeneratorWhiteboard::alloc(static_cast<i32>(n));
    if (brd == nullptr || sdep == nullptr) {
        GeneratorWhiteboard::release(brd);
        GeneratorWhiteboard::release(sdep);
        return;
    }
    flood_dep(brd, sec_ov, w, h, sid, bx, by);
    flood_dep(sdep, sec_ov, w, h, sid, cx, cy);
    if (!walk_dep(ov, sec_ov, brd, sdep, terrain, glob_wat, w, h, sid, cx, cy, bx, by)) {
        walk_sec_bfs(ov, sec_ov, terrain, glob_wat, w, h, sid, cx, cy, bx, by);
    }
    GeneratorWhiteboard::release(brd);
    GeneratorWhiteboard::release(sdep);
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
    u16* vis = GeneratorWhiteboard::alloc(static_cast<i32>(n));
    if (vis == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        vis[i] = 0;
    }
    std::vector<u32> q;
    const u32 si = tidx(w, sx, sy);
    vis[si] = 1;
    q.push_back(si);
    bool ok = false;
    i32 best = -1;
    for (std::size_t qi = 0; qi < q.size(); ++qi) {
        const u32 i = q[qi];
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
            q.push_back(ni);
        }
    }
    GeneratorWhiteboard::release(vis);
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
    u16* vis = GeneratorWhiteboard::alloc(static_cast<i32>(n));
    if (vis == nullptr) {
        return false;
    }
    std::vector<u32> par(n, n);
    std::vector<u32> q;
    vis[si] = 1;
    q.push_back(si);
    bool hit = false;
    for (std::size_t qi = 0; qi < q.size(); ++qi) {
        const u32 i = q[qi];
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
            q.push_back(ni);
        }
    }
    if (!hit) {
        GeneratorWhiteboard::release(vis);
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
    GeneratorWhiteboard::release(vis);
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
    i32 y2) 
{
    if (!try_scramble(ov, c, false, ia, ib, x1, y1, x2, y2)) {
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
    i32 y2) 
{
    if (!try_scramble(ov, c, true, sid, 0, x1, y1, x2, y2)) {
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
    u16* vis = GeneratorWhiteboard::alloc(static_cast<i32>(n));
    if (vis == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        vis[i] = 0;
    }
    std::vector<i32> qx;
    std::vector<i32> qy;
    std::vector<i32> qd;
    vis[tidx(w, sx, sy)] = 1;
    qx.push_back(sx);
    qy.push_back(sy);
    qd.push_back(0);
    i32 bwx = -1;
    i32 bwy = -1;
    i32 bd = -1;
    for (std::size_t qi = 0; qi < qx.size(); ++qi) {
        const i32 cx = qx[qi];
        const i32 cy = qy[qi];
        const i32 cd = qd[qi];
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
            qx.push_back(nx);
            qy.push_back(ny);
            qd.push_back(cd + 1);
        }
        if (bd != -1 && cd > bd) {
            break;
        }
    }
    GeneratorWhiteboard::release(vis);
    if (bd == -1) {
        return false;
    }
    *out_wx = bwx;
    *out_wy = bwy;
    return true;
}

//================================================================================================================================
//=> - Generate_RiverLines -
//================================================================================================================================

RiverLinesResult* Generate_RiverLines::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverSectorsResult* sectors,
    const RiverNetworkResult* network,
    u32 seed) 
{
    if (terrain == nullptr || w == 0 || h == 0 || sectors == nullptr || sectors->nodes == nullptr
        || sectors->overlay == nullptr || network == nullptr) {
        return nullptr;
    }
    if (network->conn_n > 0 && network->conns == nullptr) {
        return nullptr;
    }
    if (network->coastal == nullptr || network->river_sys == nullptr) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    RiverLinesResult* out = new RiverLinesResult();
    if (out == nullptr) {
        return nullptr;
    }
    out->w = w;
    out->h = h;
    out->overlay = new u8[n];
    if (out->overlay == nullptr) {
        delete out;
        return nullptr;
    }
    std::memset(out->overlay, 0, n * sizeof(u8));
    u8* glob = Generate_GlobalOcean::build_mask(terrain, w, h);
    if (glob == nullptr) {
        delete[] out->overlay;
        delete out;
        return nullptr;
    }
    std::srand(seed);
    PathCtx ctx = {};
    ctx.ov = sectors->overlay;
    ctx.terrain = terrain;
    ctx.glob_wat = glob;
    ctx.w = w;
    ctx.h = h;
    for (u16 ci = 0; ci < network->conn_n; ++ci) {
        const u16 ia = network->conns[ci].a;
        const u16 ib = network->conns[ci].b;
        if (ia >= sectors->sector_n || ib >= sectors->sector_n) {
            continue;
        }
        paint_link(
            out->overlay,
            ctx,
            ia,
            ib,
            static_cast<i32>(sectors->nodes[ia].cx),
            static_cast<i32>(sectors->nodes[ia].cy),
            static_cast<i32>(sectors->nodes[ib].cx),
            static_cast<i32>(sectors->nodes[ib].cy));
    }
    for (u32 si = 0; si < static_cast<u32>(network->sector_n); ++si) {
        if (!network->coastal[si] || network->river_sys[si] == static_cast<u16>(RIVER_SYS_NONE)) {
            continue;
        }
        i32 wx = 0;
        i32 wy = 0;
        const i32 x1 = static_cast<i32>(sectors->nodes[si].cx);
        const i32 y1 = static_cast<i32>(sectors->nodes[si].cy);
        if (!find_nearest_water(terrain, glob, w, h, x1, y1, &wx, &wy)) {
            continue;
        }
        paint_coast(out->overlay, ctx, static_cast<u16>(si), x1, y1, wx, wy);
    }
    delete[] glob;
    return out;
}

void Generate_RiverLines::free_result (RiverLinesResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->overlay;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
