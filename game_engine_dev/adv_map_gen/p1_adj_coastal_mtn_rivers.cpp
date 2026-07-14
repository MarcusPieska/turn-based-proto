//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_coastal_mtn_rivers.h"

#include "generator_constants.h"
#include "p1_step_log.h"
#include "p1_wb_util.h"
#include "wb_1b_multi_vector.h"
#include "wb_2b_multi_vector.h"
#include "wb_b4_multi_vector.h"
#include "wb_que_xy.h"

#include <cstdio>

#define CMR_ABORT(msg) P1_STEP_ABORT("P1_Adj_CoastalMtnRivers", msg)

//================================================================================================================================
//=> - Private constants -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;
static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

static const u16 k_glob_vid = 0u;
static const u16 k_glob_vec_n = 1u;
static const u16 k_dg_vid = 0u;
static const u16 k_dg_vec_n = 1u;
static const u16 k_mv_vec_n = 4u;
static const u16 k_mv_grey = 0u;
static const u16 k_mv_bdr_off = 1u;
static const u16 k_mv_cst_off = 2u;
static const u16 k_mv_scratch = 3u;
static const u16 k_seed_vec_n = 2u;
static const u16 k_seed_x_vid = 0u;
static const u16 k_seed_y_vid = 1u;

static u32 sc_cst (u32 si, u32 sector_n) {
    return sector_n + 1u + si;
}

static void dg_wr (WB_B4MultiVec& dg, u32 i, u16 dist, u16 gen) {
    dg.set(k_dg_vid, i, static_cast<u32>(dist) | (static_cast<u32>(gen) << 16u));
}

static u16 dg_dist (const WB_B4MultiVec& dg, u32 i, u16 g) {
    const u32 v = dg.at(k_dg_vid, i);
    return (static_cast<u16>(v >> 16u) == g) ? static_cast<u16>(v) : k_inf;
}

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static u16 s_flood_gen = 1u;

static u16 next_flood_gen () {
    s_flood_gen++;
    if (s_flood_gen == 0u) {
        s_flood_gen = 1u;
    }
    return s_flood_gen;
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static u32 tidx (u16 w, i32 x, i32 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static i32 man_d (i32 x1, i32 y1, i32 x2, i32 y2) {
    const i32 dx = x2 - x1;
    const i32 dy = y2 - y1;
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

static void mark_riv (
    u8* ov,
    const u8* terrain,
    const WB_1BMultiVec& glob,
    u16 w,
    u16 h,
    i32 x,
    i32 y) 
{
    if (!in_map(w, h, x, y)) {
        return;
    }
    const u32 i = tidx(w, x, y);
    if (is_water(terrain[i]) && glob.at(k_glob_vid, i) == 0u) {
        return;
    }
    ov[i] = 1;
}

static void glob_seed (
    const u8* terrain,
    u16 w,
    u16 h,
    u16 x,
    u16 y,
    WB_1BMultiVec& glob,
    WB_QueXY& que)  
{
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    if (!is_water(terrain[i]) || glob.at(k_glob_vid, i) != 0u) {
        return;
    }
    glob.set(k_glob_vid, i, 1u);
    que.push(x, y);
}

static bool build_glob_ocn_mask (const u8* terrain, u16 w, u16 h, WB_1BMultiVec& glob, WB_QueXY& que) {
    if (terrain == nullptr || w == 0 || h == 0 || !glob.ok()) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    glob.clear(k_glob_vid);
    for (u32 i = 0; i < n; ++i) {
        glob.set(k_glob_vid, i, 0u);
    }
    que.clear();
    for (u32 x = 0; x < wi; ++x) {
        glob_seed(terrain, w, h, static_cast<u16>(x), 0, glob, que);
        glob_seed(terrain, w, h, static_cast<u16>(x), static_cast<u16>(hi - 1u), glob, que);
    }
    for (u32 y = 0; y < hi; ++y) {
        glob_seed(terrain, w, h, 0, static_cast<u16>(y), glob, que);
        glob_seed(terrain, w, h, static_cast<u16>(wi - 1u), static_cast<u16>(y), glob, que);
    }
    for (u32 i = 0; i < n; ++i) {
        if (terrain[i] == TERR_OCEAN[0]) {
            glob_seed(terrain, w, h, static_cast<u16>(i % wi), static_cast<u16>(i / wi), glob, que);
        }
    }
    while (que.count() > 0u) {
        const u16 px = que.x_at(0);
        const u16 py = que.y_at(0);
        que.drop(1);
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
            if (!is_water(terrain[ni]) || glob.at(k_glob_vid, ni) != 0u) {
                continue;
            }
            glob.set(k_glob_vid, ni, 1u);
            que.push(static_cast<u16>(nx), static_cast<u16>(ny));
        }
    }
    return true;
}

static bool tile_adj_glob4 (const WB_1BMultiVec& glob, u16 w, u16 h, i32 x, i32 y) {
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = x + k_dx4[k];
        const i32 ny = y + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        if (glob.at(k_glob_vid, tidx(w, nx, ny)) != 0u) {
            return true;
        }
    }
    return false;
}

static bool sec_land_ok (
    const u8* terrain,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    i32 x,
    i32 y) 
{
    if (!in_map(w, h, x, y)) {
        return false;
    }
    const u32 i = tidx(w, x, y);
    return sec_ov[i] == sid && !is_water(terrain[i]);
}

static bool tile_sec_border (
    const u8* terrain,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    i32 x,
    i32 y) 
{
    if (!sec_land_ok(terrain, sec_ov, w, h, sid, x, y)) {
        return false;
    }
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = x + k_dx4[k];
        const i32 ny = y + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            return true;
        }
        if (!sec_land_ok(terrain, sec_ov, w, h, sid, nx, ny)) {
            return true;
        }
    }
    return false;
}

static u32 build_sec_seed_tbl (
    const u8* terrain,
    const WB_1BMultiVec& glob,
    const u8* lim_ov,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sector_n,
    WB_B4MultiVec& mv,
    WB_2BMultiVec& seed) 
{
    const u32 sn = static_cast<u32>(sector_n);
    mv.clear_all();
    seed.clear_all();
    u32 grey_n = 0;
    for (u32 si = 0; si < sn; ++si) {
        mv.set(k_mv_grey, si, 0u);
        mv.set(k_mv_scratch, si, 0u);
        mv.set(k_mv_scratch, sc_cst(si, sn), 0u);
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = sec_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        const u32 su = static_cast<u32>(sid);
        if (lim_ov[i] == static_cast<u8>(P1_COASTAL_MTN_OV_BLK)) {
            if (mv.at(k_mv_grey, su) == 0u) {
                mv.set(k_mv_grey, su, 1u);
                grey_n++;
            }
        }
        if (mv.at(k_mv_grey, su) == 0u || is_water(terrain[i])) {
            continue;
        }
        const u16 px = static_cast<u16>(i % wi);
        const u16 py = static_cast<u16>(i / wi);
        if (!tile_sec_border(terrain, sec_ov, w, h, sid, static_cast<i32>(px), static_cast<i32>(py))) {
            continue;
        }
        mv.set(k_mv_scratch, su, mv.at(k_mv_scratch, su) + 1u);
        if (tile_adj_glob4(glob, w, h, static_cast<i32>(px), static_cast<i32>(py))) {
            mv.set(k_mv_scratch, sc_cst(su, sn), mv.at(k_mv_scratch, sc_cst(su, sn)) + 1u);
        }
    }
    u32 border_tot = 0;
    u32 cst_tot = 0;
    for (u32 si = 0; si < sn; ++si) {
        mv.set(k_mv_bdr_off, si, border_tot);
        border_tot += mv.at(k_mv_scratch, si);
        mv.set(k_mv_cst_off, si, cst_tot);
        cst_tot += mv.at(k_mv_scratch, sc_cst(si, sn));
    }
    mv.set(k_mv_bdr_off, sn, border_tot);
    mv.set(k_mv_cst_off, sn, cst_tot);
    if (border_tot > n || cst_tot > n) {
        return 0;
    }
    for (u32 si = 0; si < sn; ++si) {
        mv.set(k_mv_scratch, si, mv.at(k_mv_bdr_off, si));
        mv.set(k_mv_scratch, sc_cst(si, sn), mv.at(k_mv_cst_off, si));
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = sec_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        const u32 su = static_cast<u32>(sid);
        if (mv.at(k_mv_grey, su) == 0u || is_water(terrain[i])) {
            continue;
        }
        const u16 tx = static_cast<u16>(i % wi);
        const u16 ty = static_cast<u16>(i / wi);
        if (!tile_sec_border(terrain, sec_ov, w, h, sid, static_cast<i32>(tx), static_cast<i32>(ty))) {
            continue;
        }
        u32 bw = mv.at(k_mv_scratch, su);
        seed.set(k_seed_x_vid, bw, tx);
        seed.set(k_seed_y_vid, bw, ty);
        bw++;
        mv.set(k_mv_scratch, su, bw);
        if (tile_adj_glob4(glob, w, h, static_cast<i32>(tx), static_cast<i32>(ty))) {
            u32 cw = mv.at(k_mv_scratch, sc_cst(su, sn));
            seed.set(k_seed_x_vid, cw, tx);
            seed.set(k_seed_y_vid, cw, ty);
            cw++;
            mv.set(k_mv_scratch, sc_cst(su, sn), cw);
        }
    }
    return grey_n;
}

static bool flood_seeds (
    const u8* terrain,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    const WB_2BMultiVec& seed,
    u32 seed_off,
    u32 seed_n,
    WB_B4MultiVec& dg,
    u16 cur_gen,
    WB_QueXY& bfs_que,
    WB_QueXY& visit_que) 
{
    visit_que.clear();
    bfs_que.clear();
    if (seed_n == 0u) {
        return false;
    }
    for (u32 k = 0; k < seed_n; ++k) {
        const u16 px = seed.at(k_seed_x_vid, seed_off + k);
        const u16 py = seed.at(k_seed_y_vid, seed_off + k);
        if (!sec_land_ok(terrain, sec_ov, w, h, sid, static_cast<i32>(px), static_cast<i32>(py))) {
            continue;
        }
        const u32 i = tidx(w, static_cast<i32>(px), static_cast<i32>(py));
        dg_wr(dg, i, 0, cur_gen);
        bfs_que.push(px, py);
        visit_que.push(px, py);
    }
    if (bfs_que.count() == 0u) {
        return false;
    }
    while (bfs_que.count() > 0u) {
        const u16 px = bfs_que.x_at(0);
        const u16 py = bfs_que.y_at(0);
        bfs_que.drop(1);
        const u32 i = tidx(w, static_cast<i32>(px), static_cast<i32>(py));
        const u16 d = dg_dist(dg, i, cur_gen);
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!sec_land_ok(terrain, sec_ov, w, h, sid, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (static_cast<u16>(dg.at(k_dg_vid, ni) >> 16u) == cur_gen) {
                continue;
            }
            dg_wr(dg, ni, static_cast<u16>(d + 1u), cur_gen);
            bfs_que.push(static_cast<u16>(nx), static_cast<u16>(ny));
            visit_que.push(static_cast<u16>(nx), static_cast<u16>(ny));
        }
    }
    return visit_que.count() > 0u;
}

static bool flood_one (
    const u8* terrain,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    u16 px,
    u16 py,
    WB_B4MultiVec& dg,
    u16 cur_gen,
    WB_QueXY& bfs_que,
    WB_QueXY& visit_que) 
{
    visit_que.clear();
    bfs_que.clear();
    if (!sec_land_ok(terrain, sec_ov, w, h, sid, static_cast<i32>(px), static_cast<i32>(py))) {
        return false;
    }
    const u32 i = tidx(w, static_cast<i32>(px), static_cast<i32>(py));
    dg_wr(dg, i, 0, cur_gen);
    bfs_que.push(px, py);
    visit_que.push(px, py);
    while (bfs_que.count() > 0u) {
        const u16 cx = bfs_que.x_at(0);
        const u16 cy = bfs_que.y_at(0);
        bfs_que.drop(1);
        const u32 ci = tidx(w, static_cast<i32>(cx), static_cast<i32>(cy));
        const u16 d = dg_dist(dg, ci, cur_gen);
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(cx) + k_dx4[k];
            const i32 ny = static_cast<i32>(cy) + k_dy4[k];
            if (!sec_land_ok(terrain, sec_ov, w, h, sid, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (static_cast<u16>(dg.at(k_dg_vid, ni) >> 16u) == cur_gen) {
                continue;
            }
            dg_wr(dg, ni, static_cast<u16>(d + 1u), cur_gen);
            bfs_que.push(static_cast<u16>(nx), static_cast<u16>(ny));
            visit_que.push(static_cast<u16>(nx), static_cast<u16>(ny));
        }
    }
    return visit_que.count() > 0u;
}

static bool pick_max_dist (
    u16 w,
    const WB_B4MultiVec& dg,
    u16 cur_gen,
    WB_QueXY& visit_que,
    u16* px,
    u16* py) 
{
    u16 best_d = 0;
    bool ok = false;
    const u32 tn = visit_que.count();
    for (u32 k = 0; k < tn; ++k) {
        const u16 tx = visit_que.x_at(k);
        const u16 ty = visit_que.y_at(k);
        const u32 i = tidx(w, static_cast<i32>(tx), static_cast<i32>(ty));
        const u16 d = dg_dist(dg, i, cur_gen);
        if (!ok || d > best_d) {
            best_d = d;
            *px = tx;
            *py = ty;
            ok = true;
        }
    }
    return ok;
}

static bool pick_coast_start (
    u16 w,
    u16 h,
    i32 cx,
    i32 cy,
    const WB_2BMultiVec& seed,
    u32 seed_off,
    u32 seed_n,
    u16* px,
    u16* py) 
{
    bool ok = false;
    i32 best_d = 0;
    for (u32 k = 0; k < seed_n; ++k) {
        const u16 tx = seed.at(k_seed_x_vid, seed_off + k);
        const u16 ty = seed.at(k_seed_y_vid, seed_off + k);
        const i32 d = man_d(static_cast<i32>(tx), static_cast<i32>(ty), cx, cy);
        if (!ok || d < best_d) {
            best_d = d;
            *px = tx;
            *py = ty;
            ok = true;
        }
    }
    (void)w;
    (void)h;
    return ok;
}

static bool try_dn_step (
    const u8* terrain,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    const WB_B4MultiVec& dg,
    u16 cur_gen,
    i32 x,
    i32 y,
    i32* nx,
    i32* ny) 
{
    const u16 cd = dg_dist(dg, tidx(w, x, y), cur_gen);
    if (cd == 0 || cd == k_inf) {
        return false;
    }
    u16 best = k_inf;
    i32 bx = x;
    i32 by = y;
    bool ok = false;
    for (i32 k = 0; k < 4; ++k) {
        const i32 tx = x + k_dx4[k];
        const i32 ty = y + k_dy4[k];
        if (!sec_land_ok(terrain, sec_ov, w, h, sid, tx, ty)) {
            continue;
        }
        const u16 nd = dg_dist(dg, tidx(w, tx, ty), cur_gen);
        if (nd >= cd || nd >= best) {
            continue;
        }
        best = nd;
        bx = tx;
        by = ty;
        ok = true;
    }
    if (!ok) {
        return false;
    }
    *nx = bx;
    *ny = by;
    return true;
}

static void walk_paint_dn (
    u8* riv,
    const u8* terrain,
    const WB_1BMultiVec& glob,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    const WB_B4MultiVec& dg,
    u16 cur_gen,
    u16 sx,
    u16 sy,
    u16 stop_d,
    bool paint_from) 
{
    i32 x = static_cast<i32>(sx);
    i32 y = static_cast<i32>(sy);
    const u32 lim = static_cast<u32>(w) * static_cast<u32>(h);
    if (paint_from) {
        mark_riv(riv, terrain, glob, w, h, x, y);
    }
    for (u32 s = 0; s < lim; ++s) {
        const u16 cd = dg_dist(dg, tidx(w, x, y), cur_gen);
        if (cd <= stop_d) {
            if (cd == stop_d && !paint_from) {
                mark_riv(riv, terrain, glob, w, h, x, y);
            }
            break;
        }
        i32 nx = x;
        i32 ny = y;
        if (!try_dn_step(terrain, sec_ov, w, h, sid, dg, cur_gen, x, y, &nx, &ny)) {
            break;
        }
        x = nx;
        y = ny;
        mark_riv(riv, terrain, glob, w, h, x, y);
    }
}

static bool paint_sec_river (
    u8* riv,
    const u8* terrain,
    const WB_1BMultiVec& glob,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    const WB_B4MultiVec& mv,
    const WB_2BMultiVec& seed,
    WB_B4MultiVec& dg,
    WB_QueXY& bfs_que,
    WB_QueXY& visit_que) 
{
    const u32 su = static_cast<u32>(sid);
    const u32 border_off = mv.at(k_mv_bdr_off, su);
    const u32 border_n = mv.at(k_mv_bdr_off, su + 1u) - border_off;
    const u32 cst_off = mv.at(k_mv_cst_off, su);
    const u32 cst_n = mv.at(k_mv_cst_off, su + 1u) - cst_off;
    if (border_n == 0u || cst_n == 0u) {
        return false;
    }
    const u16 gen_bdr = next_flood_gen();
    if (!flood_seeds(terrain, sec_ov, w, h, sid, seed, border_off, border_n, dg, gen_bdr, bfs_que, visit_que)) {
        return false;
    }
    u16 cen_x = 0;
    u16 cen_y = 0;
    if (!pick_max_dist(w, dg, gen_bdr, visit_que, &cen_x, &cen_y)) {
        return false;
    }
    u16 cst_x = 0;
    u16 cst_y = 0;
    if (!pick_coast_start(w, h, static_cast<i32>(cen_x), static_cast<i32>(cen_y), seed, cst_off, cst_n, &cst_x, &cst_y)) {
        return false;
    }
    const u16 gen_cen = next_flood_gen();
    if (!flood_one(terrain, sec_ov, w, h, sid, cen_x, cen_y, dg, gen_cen, bfs_que, visit_que)) {
        return false;
    }
    walk_paint_dn(riv, terrain, glob, sec_ov, w, h, sid, dg, gen_cen, cst_x, cst_y, 0, true);
    const u16 join_x = cen_x;
    const u16 join_y = cen_y;
    const u16 gen_cst = next_flood_gen();
    if (!flood_seeds(terrain, sec_ov, w, h, sid, seed, cst_off, cst_n, dg, gen_cst, bfs_que, visit_que)) {
        return false;
    }
    u16 dep_x = cst_x;
    u16 dep_y = cst_y;
    if (!pick_max_dist(w, dg, gen_cst, visit_que, &dep_x, &dep_y)) {
        return false;
    }
    const u16 gen_dep = next_flood_gen();
    if (!flood_one(terrain, sec_ov, w, h, sid, dep_x, dep_y, dg, gen_dep, bfs_que, visit_que)) {
        return false;
    }
    walk_paint_dn(riv, terrain, glob, sec_ov, w, h, sid, dg, gen_dep, join_x, join_y, 1, false);
    return true;
}

static u32 sweep_grey_pass (
    const u8* terrain,
    u16 w,
    u16 h,
    u8* riv,
    const WB_1BMultiVec& glob,
    const u16* sec_ov,
    u16 sector_n,
    const WB_B4MultiVec& mv,
    const WB_2BMultiVec& seed,
    WB_B4MultiVec& dg,
    WB_QueXY& bfs_que,
    WB_QueXY& visit_que) 
{
    u32 path_n = 0;
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        if (mv.at(k_mv_grey, si) == 0u) {
            continue;
        }
        if (paint_sec_river(riv, terrain, glob, sec_ov, w, h, static_cast<u16>(si), mv, seed, dg, bfs_que, visit_que)) {
            path_n++;
        } else {
            char msg[72];
            std::snprintf(msg, sizeof(msg), "paint_sec_river failed sector %u", static_cast<unsigned>(si));
            p1_step_abort("P1_Adj_CoastalMtnRivers", msg);
        }
    }
    return path_n;
}

//================================================================================================================================
//=> - P1_Adj_CoastalMtnRivers -
//================================================================================================================================

P1_Adj_CoastalMtnRivers::P1_Adj_CoastalMtnRivers (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_adjust(false),
    m_path_n(0) {
}

bool P1_Adj_CoastalMtnRivers::adjust (
    const u8* terrain,
    u16 w,
    u16 h,
    u8* riv,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_CoastalMtnLimitsRslt& coast_lim) 
{
    m_valid_adjust = false;
    m_path_n = 0;
    if (terrain == nullptr || riv == nullptr || !p1_map_size_ok(w, h)) {
        CMR_ABORT("adjust null terrain or riv or invalid map size");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        CMR_ABORT("adjust size mismatch");
    }
    if (sectors.m_ov == nullptr || sectors.m_sector_n == 0) {
        CMR_ABORT("adjust null or empty sectors");
    }
    if (sectors.m_w != w || sectors.m_h != h) {
        CMR_ABORT("adjust sectors size mismatch");
    }
    const u8* lim_ov = coast_lim.m_limit_ov.data();
    if (lim_ov == nullptr || coast_lim.m_w != w || coast_lim.m_h != h) {
        CMR_ABORT("adjust null or mismatched coastal limit ov");
    }
    const u16 sector_n = sectors.m_sector_n;
    const u32 sn = static_cast<u32>(sector_n);
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    WB_1BMultiVec glob("P1_Adj_CoastalMtnRivers", "glob", m_prm.m_seed, k_glob_vec_n);
    if (!glob.ok() || glob.cap(k_glob_vid) < tile_n) {
        CMR_ABORT("adjust glob multi vector failed");
    }
    WB_B4MultiVec mv("P1_Adj_CoastalMtnRivers", "sec_mv", m_prm.m_seed, k_mv_vec_n);
    if (!mv.ok()) {
        CMR_ABORT("adjust sec multi vector failed");
    }
    WB_2BMultiVec seed("P1_Adj_CoastalMtnRivers", "seed", m_prm.m_seed, k_seed_vec_n);
    if (!seed.ok()) {
        CMR_ABORT("adjust seed multi vector failed");
    }
    WB_B4MultiVec dg("P1_Adj_CoastalMtnRivers", "dg", m_prm.m_seed, k_dg_vec_n);
    if (!dg.ok() || dg.cap(k_dg_vid) < tile_n) {
        CMR_ABORT("adjust dist-gen multi vector failed");
    }
    const u32 seg_cap = mv.cap(k_mv_grey);
    if (sn + 1u > seg_cap || (sn + 1u) * 2u > mv.cap(k_mv_scratch)) {
        CMR_ABORT("adjust sector metadata exceeds multi vector cap");
    }
    WB_QueXY bfs_que;
    WB_QueXY visit_que;
    if (!bfs_que.ok() || !visit_que.ok()) {
        CMR_ABORT("adjust bfs queue failed");
    }
    s_flood_gen = 1u;
    if (!build_glob_ocn_mask(terrain, w, h, glob, bfs_que)) {
        CMR_ABORT("adjust glob ocean mask failed");
    }
    const u32 grey_n = build_sec_seed_tbl(
        terrain, glob, lim_ov, sectors.m_ov, w, h, sector_n, mv, seed);
    if (grey_n == 0u) {
        CMR_ABORT("adjust no grey coastal sectors");
    }
    const u32 border_tot = mv.at(k_mv_bdr_off, sn);
    const u32 cst_tot = mv.at(k_mv_cst_off, sn);
    if (border_tot > seed.cap(k_seed_x_vid) || cst_tot > seed.cap(k_seed_y_vid)) {
        CMR_ABORT("adjust seed list exceeds multi vector cap");
    }
    m_path_n = sweep_grey_pass(
        terrain, w, h, riv, glob, sectors.m_ov, sector_n, mv, seed, dg,
        bfs_que, visit_que);
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_CoastalMtnRivers::is_valid () const {
    return m_valid_adjust;
}

u32 P1_Adj_CoastalMtnRivers::path_n () const {
    return m_path_n;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
