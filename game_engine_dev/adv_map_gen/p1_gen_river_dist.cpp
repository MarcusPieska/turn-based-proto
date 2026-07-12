//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_dist.h"

#include <cstring>

#include "generator_constants.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"

//================================================================================================================================
//=> - Private constants -
//================================================================================================================================

static const u16 k_dep_none = 0xFFFFu;
static const u32 k_land_hd_none = 0xFFFFFFFFu;
static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static u16 dist_eo (u16 dep) {
    return static_cast<u16>(static_cast<u32>(dep) / 2u + 1u);
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
    const u32 i = tidx(w, x, y);
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

static bool tile_adj_cst_sea_ocn (const u8* terrain, u16 w, u16 h, u16 x, u16 y) {
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

static bool tile_basin_flood (
    const u8* terrain,
    const u8* riv,
    const u16* basin_ov,
    const u16* glob,
    u16 w,
    u16 h,
    u16 bidx,
    u16 x,
    u16 y) 
{
    const u32 i = tidx(w, x, y);
    if (basin_ov[i] != bidx) {
        return false;
    }
    if (riv[i] != 0) {
        return true;
    }
    if (!is_water(terrain[i]) || glob[i] != 0) {
        return false;
    }
    return true;
}

static bool is_land_riv (const u8* terrain, const u8* riv, u32 i) {
    return riv[i] != 0 && !is_water(terrain[i]);
}

static void set_land_head (
    u32* land_hd,
    const u8* terrain,
    const u8* riv,
    u32 ti,
    u32 ni) 
{
    if (is_land_riv(terrain, riv, ni)) {
        land_hd[ni] = ni;
        return;
    }
    if (!is_water(terrain[ni])) {
        land_hd[ni] = k_land_hd_none;
        return;
    }
    if (is_land_riv(terrain, riv, ti)) {
        land_hd[ni] = ti;
        return;
    }
    if (is_water(terrain[ti]) && land_hd[ti] != k_land_hd_none) {
        land_hd[ni] = land_hd[ti];
        return;
    }
    land_hd[ni] = k_land_hd_none;
}

static u32 flood_up_basin (
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* riv,
    const u16* basin_ov,
    const u16* glob,
    u16 bidx,
    const u16* mouth_x,
    const u16* mouth_y,
    u32 mouth_n,
    u16* dep,
    u16* dist_up,
    u32* land_hd,
    u32* heads,
    u32 head_cap,
    WB_QueXY& que) 
{
    u32 head_n = 0;
    for (u32 mi = 0; mi < mouth_n; ++mi) {
        const u16 sx = mouth_x[mi];
        const u16 sy = mouth_y[mi];
        const u32 si = tidx(w, sx, sy);
        if (dist_up[si] != 0 || dep[si] != k_dep_none) {
            continue;
        }
        dep[si] = 0;
        dist_up[si] = 1;
        if (is_land_riv(terrain, riv, si)) {
            land_hd[si] = si;
        } else {
            land_hd[si] = k_land_hd_none;
        }
        que.clear();
        que.push(sx, sy);
        u32 qi = 0;
        while (qi < que.count()) {
            const u16 px = que.x_at(qi);
            const u16 py = que.y_at(qi);
            qi++;
            const u32 ti = tidx(w, px, py);
            const u16 cd = dep[ti];
            bool ext = false;
            for (i32 k = 0; k < 4; ++k) {
                const i32 nx = static_cast<i32>(px) + k_dx4[k];
                const i32 ny = static_cast<i32>(py) + k_dy4[k];
                if (!in_map(w, h, nx, ny)) {
                    continue;
                }
                const u16 tx = static_cast<u16>(nx);
                const u16 ty = static_cast<u16>(ny);
                if (!tile_basin_flood(terrain, riv, basin_ov, glob, w, h, bidx, tx, ty)) {
                    continue;
                }
                const u32 ni = tidx(w, tx, ty);
                if (dep[ni] != k_dep_none) {
                    continue;
                }
                dep[ni] = static_cast<u16>(cd + 1u);
                if (riv[ni] != 0) {
                    dist_up[ni] = dist_eo(dep[ni]);
                }
                set_land_head(land_hd, terrain, riv, ti, ni);
                que.push(tx, ty);
                ext = true;
            }
            if (!ext && riv[ti] != 0 && head_n < head_cap) {
                u32 ht = ti;
                if (is_water(terrain[ti])) {
                    if (land_hd[ti] == k_land_hd_none) {
                        continue;
                    }
                    ht = land_hd[ti];
                }
                heads[head_n++] = ht;
            }
        }
    }
    return head_n;
}

static void flood_dn_head (
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* riv,
    const u16* basin_ov,
    const u16* glob,
    u16 bidx,
    u32 head,
    const u16* dep,
    u16* dist_dn,
    u16* step,
    WB_QueXY& que) 
{
    if (dep[head] == k_dep_none) {
        return;
    }
    const u16 hx = static_cast<u16>(head % static_cast<u32>(w));
    const u16 hy = static_cast<u16>(head / static_cast<u32>(w));
    if (dist_dn[head] == 0) {
        dist_dn[head] = 1;
    }
    step[head] = 0;
    que.clear();
    que.push(hx, hy);
    u32 qi = 0;
    while (qi < que.count()) {
        const u16 px = que.x_at(qi);
        const u16 py = que.y_at(qi);
        qi++;
        const u32 ti = tidx(w, px, py);
        const u16 st = step[ti];
        const u16 dp = dep[ti];
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u16 tx = static_cast<u16>(nx);
            const u16 ty = static_cast<u16>(ny);
            if (!tile_basin_flood(terrain, riv, basin_ov, glob, w, h, bidx, tx, ty)) {
                continue;
            }
            const u32 ni = tidx(w, tx, ty);
            if (dep[ni] == k_dep_none || dep[ni] >= dp) {
                continue;
            }
            const u16 nst = static_cast<u16>(st + 1u);
            const u16 nd = dist_eo(nst);
            if (riv[ni] != 0) {
                if (dist_dn[ni] >= nd) {
                    continue;
                }
                dist_dn[ni] = nd;
            } else if (step[ni] != 0 && step[ni] <= nst) {
                continue;
            }
            step[ni] = nst;
            que.push(tx, ty);
        }
    }
}

static void fill_dn_riv_miss (
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* riv,
    const u16* basin_ov,
    const u16* glob,
    u16* dep,
    u16* dist_dn,
    u16* step,
    WB_QueXY& que) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0 || dist_dn[i] != 0 || dep[i] == k_dep_none) {
            continue;
        }
        const u16 bidx = basin_ov[i];
        if (bidx == static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            continue;
        }
        flood_dn_head(w, h, terrain, riv, basin_ov, glob, bidx, i, dep, dist_dn, step, que);
    }
}

static void flood_from_coast_mouths (
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* riv,
    const u16* basin_ov,
    const u16* glob,
    u16* dep,
    u16* dist_up,
    u16* dist_dn,
    u16* step,
    u32* land_hd,
    u32* heads,
    u32 head_cap,
    WB_QueXY& que) 
{
    u16 mouth_x = 0;
    u16 mouth_y = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            if (riv[i] == 0) {
                continue;
            }
            if (!tile_adj_cst_sea_ocn(terrain, w, h, x, y)) {
                continue;
            }
            if (dist_up[i] != 0 || dep[i] != k_dep_none) {
                continue;
            }
            const u16 bidx = basin_ov[i];
            mouth_x = x;
            mouth_y = y;
            const u32 head_n = flood_up_basin(
                w, h, terrain, riv, basin_ov, glob, bidx, &mouth_x, &mouth_y, 1u, dep, dist_up, land_hd,
                heads, head_cap, que);
            for (i32 hi = static_cast<i32>(head_n) - 1; hi >= 0; --hi) {
                flood_dn_head(
                    w, h, terrain, riv, basin_ov, glob, bidx, heads[static_cast<u32>(hi)], dep, dist_dn,
                    step, que);
            }
        }
    }
}

static bool build_river_dist (
    const u8* terrain,
    u16 w,
    u16 h,
    const u8* riv,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverNetworkRslt& network,
    P1_Gen_RiverDistRslt* out) 
{
    (void)sectors;
    if (out == nullptr || terrain == nullptr || riv == nullptr || network.m_ov == nullptr) {
        return false;
    }
    if (!p1_map_size_ok(w, h) || network.m_w != w || network.m_h != h) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (!out->m_up.resize(w, h) || !out->m_dn.resize(w, h)) {
        return false;
    }
    u16* up = out->m_up.data_w();
    u16* dn = out->m_dn.data_w();
    if (up == nullptr || dn == nullptr) {
        return false;
    }
    std::memset(up, 0, n * sizeof(u16));
    std::memset(dn, 0, n * sizeof(u16));
    Whiteboard_2B wb_glob("P1_Gen_RiverDist", "glob", 0u);
    P1_WB_CHK(wb_glob);
    Whiteboard_2B wb_dep("P1_Gen_RiverDist", "dep", 0u);
    P1_WB_CHK(wb_dep);
    Whiteboard_2B wb_step("P1_Gen_RiverDist", "step", 0u);
    P1_WB_CHK(wb_step);
    WB_QueXY que;
    if (!que.ok()) {
        return false;
    }
    u16* glob = wb_glob.get_iter_ptr();
    u16* dep = wb_dep.get_iter_ptr();
    u16* step = wb_step.get_iter_ptr();
    if (!build_glob_ocn_mask(terrain, w, h, glob, que)) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        dep[i] = k_dep_none;
    }
    u32 heads[P1_RIVER_DIST_HEAD_MAX];
    u32* land_hd = new u32[n];
    if (land_hd == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        land_hd[i] = k_land_hd_none;
    }
    u16 max_up = 0;
    u16 max_dn = 0;
    flood_from_coast_mouths(
        w, h, terrain, riv, network.m_ov, glob, dep, up, dn, step, land_hd, heads,
        P1_RIVER_DIST_HEAD_MAX, que);
    fill_dn_riv_miss(w, h, terrain, riv, network.m_ov, glob, dep, dn, step, que);
    delete[] land_hd;
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0) {
            up[i] = 0;
            dn[i] = 0;
            continue;
        }
        if (up[i] > max_up) {
            max_up = up[i];
        }
        if (dn[i] > max_dn) {
            max_dn = dn[i];
        }
    }
    out->m_w = w;
    out->m_h = h;
    out->m_max_up = max_up;
    out->m_max_dn = max_dn;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverDist -
//================================================================================================================================

P1_Gen_RiverDist::P1_Gen_RiverDist (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_max_up = 0;
    m_rslt.m_max_dn = 0;
}

bool P1_Gen_RiverDist::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const u8* riv,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverNetworkRslt& network) 
{
    m_valid_generation = false;
    m_rslt.m_up.clear();
    m_rslt.m_dn.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_max_up = 0;
    m_rslt.m_max_dn = 0;
    if (terrain == nullptr || riv == nullptr || w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!build_river_dist(terrain, w, h, riv, sectors, network, &m_rslt)) {
        m_rslt.m_up.clear();
        m_rslt.m_dn.clear();
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RiverDist::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverDistRslt& P1_Gen_RiverDist::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
