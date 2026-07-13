//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_coastal_mtn_rivers.h"

#include "generator_constants.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"

//================================================================================================================================
//=> - Private constants -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;
static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};
#define P1_COASTAL_RIV_SECTOR_CAP 20000u

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

static u16 dist_at (const u16* dist, const u16* gen, u32 i, u16 g) {
    return gen[i] == g ? dist[i] : k_inf;
}

static void mark_riv (
    u8* ov,
    const u8* terrain,
    const u16* glob,
    u16 w,
    u16 h,
    i32 x,
    i32 y) 
{
    if (!in_map(w, h, x, y)) {
        return;
    }
    const u32 i = tidx(w, x, y);
    if (is_water(terrain[i]) && glob[i] == 0) {
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
    u16* mask,
    WB_QueXY& que)  
{
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    if (!is_water(terrain[i]) || mask[i] != 0) {
        return;
    }
    mask[i] = 1;
    que.push(x, y);
}

static bool build_glob_ocn_mask (const u8* terrain, u16 w, u16 h, u16* mask, WB_QueXY& que) {
    if (terrain == nullptr || mask == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        mask[i] = 0;
    }
    que.clear();
    for (u32 x = 0; x < wi; ++x) {
        glob_seed(terrain, w, h, static_cast<u16>(x), 0, mask, que);
        glob_seed(terrain, w, h, static_cast<u16>(x), static_cast<u16>(hi - 1u), mask, que);
    }
    for (u32 y = 0; y < hi; ++y) {
        glob_seed(terrain, w, h, 0, static_cast<u16>(y), mask, que);
        glob_seed(terrain, w, h, static_cast<u16>(wi - 1u), static_cast<u16>(y), mask, que);
    }
    for (u32 i = 0; i < n; ++i) {
        if (terrain[i] == TERR_OCEAN[0]) {
            glob_seed(terrain, w, h, static_cast<u16>(i % wi), static_cast<u16>(i / wi), mask, que);
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
            if (!is_water(terrain[ni]) || mask[ni] != 0) {
                continue;
            }
            mask[ni] = 1;
            que.push(static_cast<u16>(nx), static_cast<u16>(ny));
        }
    }
    return true;
}

static bool tile_adj_glob4 (const u16* glob, u16 w, u16 h, i32 x, i32 y) {
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = x + k_dx4[k];
        const i32 ny = y + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        if (glob[tidx(w, nx, ny)] != 0) {
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

struct SecSeedTbl {
    u32 m_border_off[P1_COASTAL_RIV_SECTOR_CAP + 1u];
    u32 m_cst_off[P1_COASTAL_RIV_SECTOR_CAP + 1u];
};

static u32 build_sec_seed_tbl (
    const u8* terrain,
    const u16* glob,
    const u8* lim_ov,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sector_n,
    bool* sec_grey,
    SecSeedTbl* tbl,
    u16* seed_x,
    u16* seed_y) 
{
    if (tbl == nullptr || seed_x == nullptr || seed_y == nullptr || sec_grey == nullptr) {
        return 0;
    }
    u32 border_cnt[P1_COASTAL_RIV_SECTOR_CAP];
    u32 cst_cnt[P1_COASTAL_RIV_SECTOR_CAP];
    u32 grey_n = 0;
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        border_cnt[si] = 0;
        cst_cnt[si] = 0;
        sec_grey[si] = false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = sec_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        if (lim_ov[i] == static_cast<u8>(P1_COASTAL_MTN_OV_BLK)) {
            if (!sec_grey[sid]) {
                sec_grey[sid] = true;
                grey_n++;
            }
        }
        if (!sec_grey[sid] || is_water(terrain[i])) {
            continue;
        }
        const u16 px = static_cast<u16>(i % wi);
        const u16 py = static_cast<u16>(i / wi);
        if (!tile_sec_border(terrain, sec_ov, w, h, sid, static_cast<i32>(px), static_cast<i32>(py))) {
            continue;
        }
        border_cnt[sid]++;
        if (tile_adj_glob4(glob, w, h, static_cast<i32>(px), static_cast<i32>(py))) {
            cst_cnt[sid]++;
        }
    }
    u32 border_tot = 0;
    u32 cst_tot = 0;
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        tbl->m_border_off[si] = border_tot;
        border_tot += border_cnt[si];
        tbl->m_cst_off[si] = cst_tot;
        cst_tot += cst_cnt[si];
    }
    tbl->m_border_off[sector_n] = border_tot;
    tbl->m_cst_off[sector_n] = cst_tot;
    if (border_tot > n) {
        return 0;
    }
    u32 border_wr[P1_COASTAL_RIV_SECTOR_CAP];
    u32 cst_wr[P1_COASTAL_RIV_SECTOR_CAP];
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        border_wr[si] = tbl->m_border_off[si];
        cst_wr[si] = tbl->m_cst_off[si];
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = sec_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        if (!sec_grey[sid] || is_water(terrain[i])) {
            continue;
        }
        const u16 tx = static_cast<u16>(i % wi);
        const u16 ty = static_cast<u16>(i / wi);
        if (!tile_sec_border(terrain, sec_ov, w, h, sid, static_cast<i32>(tx), static_cast<i32>(ty))) {
            continue;
        }
        seed_x[border_wr[sid]] = tx;
        seed_y[border_wr[sid]] = ty;
        border_wr[sid]++;
        if (tile_adj_glob4(glob, w, h, static_cast<i32>(tx), static_cast<i32>(ty))) {
            seed_x[cst_wr[sid]] = tx;
            seed_y[cst_wr[sid]] = ty;
            cst_wr[sid]++;
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
    const u16* seed_x,
    const u16* seed_y,
    u32 seed_off,
    u32 seed_n,
    u16* dist,
    u16* gen,
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
        const u16 px = seed_x[seed_off + k];
        const u16 py = seed_y[seed_off + k];
        if (!sec_land_ok(terrain, sec_ov, w, h, sid, static_cast<i32>(px), static_cast<i32>(py))) {
            continue;
        }
        const u32 i = tidx(w, static_cast<i32>(px), static_cast<i32>(py));
        gen[i] = cur_gen;
        dist[i] = 0;
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
        const u16 d = dist[i];
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!sec_land_ok(terrain, sec_ov, w, h, sid, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (gen[ni] == cur_gen) {
                continue;
            }
            gen[ni] = cur_gen;
            dist[ni] = static_cast<u16>(d + 1u);
            bfs_que.push(static_cast<u16>(nx), static_cast<u16>(ny));
            visit_que.push(static_cast<u16>(nx), static_cast<u16>(ny));
        }
    }
    return visit_que.count() > 0u;
}

static bool pick_max_dist (
    u16 w,
    const u16* dist,
    const u16* gen,
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
        const u16 d = dist_at(dist, gen, i, cur_gen);
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
    const u16* seed_x,
    const u16* seed_y,
    u32 seed_off,
    u32 seed_n,
    u16* px,
    u16* py) 
{
    bool ok = false;
    i32 best_d = 0;
    for (u32 k = 0; k < seed_n; ++k) {
        const u16 tx = seed_x[seed_off + k];
        const u16 ty = seed_y[seed_off + k];
        const i32 d = man_d(static_cast<i32>(tx), static_cast<i32>(ty), cx, cy);
        if (!ok || d < best_d) {
            best_d = d;
            *px = tx;
            *py = ty;
            ok = true;
        }
    }
    return ok;
}

static bool try_dn_step (
    const u8* terrain,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    const u16* dist,
    const u16* gen,
    u16 cur_gen,
    i32 x,
    i32 y,
    i32* nx,
    i32* ny) 
{
    const u16 cd = dist_at(dist, gen, tidx(w, x, y), cur_gen);
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
        const u16 nd = dist_at(dist, gen, tidx(w, tx, ty), cur_gen);
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
    const u16* glob,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    const u16* dist,
    const u16* gen,
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
        const u16 cd = dist_at(dist, gen, tidx(w, x, y), cur_gen);
        if (cd <= stop_d) {
            if (cd == stop_d && !paint_from) {
                mark_riv(riv, terrain, glob, w, h, x, y);
            }
            break;
        }
        i32 nx = x;
        i32 ny = y;
        if (!try_dn_step(terrain, sec_ov, w, h, sid, dist, gen, cur_gen, x, y, &nx, &ny)) {
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
    const u16* glob,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    const SecSeedTbl& tbl,
    const u16* seed_x,
    const u16* seed_y,
    u16* dist_bdr,
    u16* dist_cst,
    u16* dist_dep,
    u16* gen,
    WB_QueXY& bfs_que,
    WB_QueXY& visit_que) 
{
    const u32 border_off = tbl.m_border_off[sid];
    const u32 border_n = tbl.m_border_off[sid + 1u] - border_off;
    const u32 cst_off = tbl.m_cst_off[sid];
    const u32 cst_n = tbl.m_cst_off[sid + 1u] - cst_off;
    if (border_n == 0u || cst_n == 0u) {
        return false;
    }
    const u16 gen_bdr = next_flood_gen();
    if (!flood_seeds(
            terrain, sec_ov, w, h, sid, seed_x, seed_y, border_off, border_n,
            dist_bdr, gen, gen_bdr, bfs_que, visit_que)) {
        return false;
    }
    u16 cen_x = 0;
    u16 cen_y = 0;
    if (!pick_max_dist(w, dist_bdr, gen, gen_bdr, visit_que, &cen_x, &cen_y)) {
        return false;
    }
    u16 cst_x = 0;
    u16 cst_y = 0;
    if (!pick_coast_start(w, h, static_cast<i32>(cen_x), static_cast<i32>(cen_y), seed_x, seed_y, cst_off, cst_n, &cst_x, &cst_y)) {
        return false;
    }
    const u16 gen_cen = next_flood_gen();
    if (!flood_seeds(terrain, sec_ov, w, h, sid, &cen_x, &cen_y, 0, 1u, dist_bdr, gen, gen_cen, bfs_que, visit_que)) {
        return false;
    }
    walk_paint_dn(riv, terrain, glob, sec_ov, w, h, sid, dist_bdr, gen, gen_cen, cst_x, cst_y, 0, true);
    const u16 join_x = cen_x;
    const u16 join_y = cen_y;
    const u16 gen_cst = next_flood_gen();
    if (!flood_seeds(
            terrain, sec_ov, w, h, sid, seed_x, seed_y, cst_off, cst_n,
            dist_cst, gen, gen_cst, bfs_que, visit_que)) {
        return false;
    }
    u16 dep_x = cst_x;
    u16 dep_y = cst_y;
    if (!pick_max_dist(w, dist_cst, gen, gen_cst, visit_que, &dep_x, &dep_y)) {
        return false;
    }
    const u16 gen_dep = next_flood_gen();
    if (!flood_seeds(terrain, sec_ov, w, h, sid, &dep_x, &dep_y, 0, 1u, dist_dep, gen, gen_dep, bfs_que, visit_que)) {
        return false;
    }
    walk_paint_dn(riv, terrain, glob, sec_ov, w, h, sid, dist_dep, gen, gen_dep, join_x, join_y, 1, false);
    return true;
}

static u32 sweep_grey_pass (
    const u8* terrain,
    u16 w,
    u16 h,
    u8* riv,
    const u16* glob,
    const u16* sec_ov,
    u16 sector_n,
    const bool* sec_grey,
    const SecSeedTbl& tbl,
    const u16* seed_x,
    const u16* seed_y,
    u16* dist_bdr,
    u16* dist_cst,
    u16* dist_dep,
    u16* gen,
    WB_QueXY& bfs_que,
    WB_QueXY& visit_que) 
{
    u32 path_n = 0;
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        if (!sec_grey[si]) {
            continue;
        }
        if (paint_sec_river(
                riv, terrain, glob, sec_ov, w, h, static_cast<u16>(si),
                tbl, seed_x, seed_y, dist_bdr, dist_cst, dist_dep, gen,
                bfs_que, visit_que)) {
            path_n++;
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
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (sectors.m_ov == nullptr || sectors.m_sector_n == 0) {
        return false;
    }
    if (sectors.m_w != w || sectors.m_h != h) {
        return false;
    }
    const u8* lim_ov = coast_lim.m_limit_ov.data();
    if (lim_ov == nullptr || coast_lim.m_w != w || coast_lim.m_h != h) {
        return false;
    }
    const u16 sector_n = sectors.m_sector_n;
    if (static_cast<u32>(sector_n) > P1_COASTAL_RIV_SECTOR_CAP) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    bool sec_grey[P1_COASTAL_RIV_SECTOR_CAP];
    SecSeedTbl tbl = {};
    Whiteboard_2B wb_glob("P1_Adj_CoastalMtnRivers", "glob", m_prm.m_seed);
    P1_WB_CHK(wb_glob);
    Whiteboard_2B wb_dist_bdr("P1_Adj_CoastalMtnRivers", "dist_bdr", m_prm.m_seed);
    P1_WB_CHK(wb_dist_bdr);
    Whiteboard_2B wb_dist_cst("P1_Adj_CoastalMtnRivers", "dist_cst", m_prm.m_seed);
    P1_WB_CHK(wb_dist_cst);
    Whiteboard_2B wb_dist_dep("P1_Adj_CoastalMtnRivers", "dist_dep", m_prm.m_seed);
    P1_WB_CHK(wb_dist_dep);
    Whiteboard_2B wb_gen("P1_Adj_CoastalMtnRivers", "gen", m_prm.m_seed);
    P1_WB_CHK(wb_gen);
    Whiteboard_2B wb_seed_x("P1_Adj_CoastalMtnRivers", "seed_x", m_prm.m_seed);
    P1_WB_CHK(wb_seed_x);
    Whiteboard_2B wb_seed_y("P1_Adj_CoastalMtnRivers", "seed_y", m_prm.m_seed);
    P1_WB_CHK(wb_seed_y);
    WB_QueXY bfs_que;
    WB_QueXY visit_que;
    if (!bfs_que.ok() || !visit_que.ok()) {
        return false;
    }
    u16* glob = wb_glob.get_iter_ptr();
    u16* dist_bdr = wb_dist_bdr.get_iter_ptr();
    u16* dist_cst = wb_dist_cst.get_iter_ptr();
    u16* dist_dep = wb_dist_dep.get_iter_ptr();
    u16* gen = wb_gen.get_iter_ptr();
    u16* seed_x = wb_seed_x.get_iter_ptr();
    u16* seed_y = wb_seed_y.get_iter_ptr();
    s_flood_gen = 1u;
    if (!build_glob_ocn_mask(terrain, w, h, glob, bfs_que)) {
        return false;
    }
    const u32 grey_n = build_sec_seed_tbl(
        terrain, glob, lim_ov, sectors.m_ov, w, h, sector_n, sec_grey, &tbl, seed_x, seed_y);
    if (grey_n == 0u) {
        return false;
    }
    m_path_n = sweep_grey_pass(
        terrain, w, h, riv, glob, sectors.m_ov, sector_n, sec_grey, tbl, seed_x, seed_y,
        dist_bdr, dist_cst, dist_dep, gen, bfs_que, visit_que);
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
