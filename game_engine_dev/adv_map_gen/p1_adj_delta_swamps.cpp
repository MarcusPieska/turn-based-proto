//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "p1_adj_delta_swamps.h"

#include "game_map_defs.h"
#include "p1_gen_river_network.h"
#include "p1_wb_util.h"
#include "res_placement_defs.h"

//================================================================================================================================
//=> - Delta swamp tuning -
//================================================================================================================================

#define P1_DELTA_SWAMP_SCORE_DESERT 0u
#define P1_DELTA_SWAMP_SCORE_PLAINS 1u
#define P1_DELTA_SWAMP_SCORE_GRASS 2u
#define P1_DELTA_SWAMP_SCORE_BLACK 2u
#define P1_DELTA_SWAMP_SCORE_HILLS 1u
#define P1_DELTA_SWAMP_SCORE_MTN 2u
#define P1_DELTA_SWAMP_WATER_MUL 8u
#define P1_DELTA_SWAMP_WATER_DIV 100u

#define P1_DELTA_SWAMP_COST_NORM 100u
#define P1_DELTA_SWAMP_COST_RIV_UP 25u
#define P1_DELTA_SWAMP_COST_RIV_SIDE 50u
#define P1_DELTA_SWAMP_COST_DIAG_NORM 141u
#define P1_DELTA_SWAMP_COST_DIAG_RIV_UP 35u
#define P1_DELTA_SWAMP_COST_DIAG_RIV_SIDE 71u

//================================================================================================================================
//=> - Private constants -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};
static const i32 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i32 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static const u16 k_rdist_none = 0xFFFFu;
static const u32 k_cost_inf = 0xFFFFFFFFu;

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static u32 wb_rd32 (const u16* lo, const u16* hi, u32 i) {
    return static_cast<u32>(lo[i]) | (static_cast<u32>(hi[i]) << 16);
}

static void wb_wr32 (u16* lo, u16* hi, u32 i, u32 v) {
    lo[i] = static_cast<u16>(v);
    hi[i] = static_cast<u16>(v >> 16);
}

static u32 q_get (const u16* lo, const u16* hi, u32 i) {
    return wb_rd32(lo, hi, i);
}

static void q_set (u16* lo, u16* hi, u32 i, u32 v) {
    wb_wr32(lo, hi, i, v);
}

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

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool is_plains_terr (u8 cls) {
    return cls == TERR_PLAINS[0];
}

static bool tile_adj4_open_water (const u8* terrain, u16 w, u16 h, u16 x, u16 y) {
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

static u32 isqrt_u32 (u32 v) {
    u32 r = 0;
    u32 bit = 1u << 30;
    while (bit > v) {
        bit >>= 2;
    }
    while (bit != 0u) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

static bool in_eucl_rng_sq (u16 mx, u16 my, u16 x, u16 y, u32 max_r_sq) {
    const i32 dx = static_cast<i32>(x) - static_cast<i32>(mx);
    const i32 dy = static_cast<i32>(y) - static_cast<i32>(my);
    const u32 d2 = static_cast<u32>(dx * dx + dy * dy);
    return d2 <= max_r_sq;
}

static bool tile_beside_riv (const u8* riv, u16 w, u16 h, u32 i) {
    if (riv[i] != 0) {
        return false;
    }
    const u16 px = static_cast<u16>(i % static_cast<u32>(w));
    const u16 py = static_cast<u16>(i / static_cast<u32>(w));
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(px) + k_dx4[k];
        const i32 ny = static_cast<i32>(py) + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        if (riv[tidx(w, static_cast<u16>(nx), static_cast<u16>(ny))] != 0) {
            return true;
        }
    }
    return false;
}

static u32 river_reach_n (
    u16 bidx,
    const u16* wshed,
    const u8* riv,
    u16 w,
    u16 h,
    u32 seed_i,
    u8* vis,
    u16* qlo,
    u16* qhi) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 qn = 0;
    u32 cnt = 0;
    vis[seed_i] = 1;
    q_set(qlo, qhi, qn++, seed_i);
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q_get(qlo, qhi, qh);
        cnt++;
        const u16 px = static_cast<u16>(i % static_cast<u32>(w));
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
            if (vis[ni] != 0 || wshed[ni] != bidx || riv[ni] == 0) {
                continue;
            }
            vis[ni] = 1;
            q_set(qlo, qhi, qn++, ni);
            if (qn > n) {
                return cnt;
            }
        }
    }
    return cnt;
}

static void build_river_up_dist (
    u16 bidx,
    const u16* wshed,
    const u8* riv,
    u16 w,
    u16 h,
    u32 mouth_i,
    u16* rdist,
    u16* qlo,
    u16* qhi) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        rdist[i] = k_rdist_none;
    }
    u32 qn = 0;
    rdist[mouth_i] = 0;
    q_set(qlo, qhi, qn++, mouth_i);
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q_get(qlo, qhi, qh);
        const u16 px = static_cast<u16>(i % static_cast<u32>(w));
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
            if (wshed[ni] != bidx || riv[ni] == 0 || rdist[ni] != k_rdist_none) {
                continue;
            }
            rdist[ni] = static_cast<u16>(rdist[i] + 1u);
            q_set(qlo, qhi, qn++, ni);
            if (qn > n) {
                return;
            }
        }
    }
}

static u32 move_cost (
    bool diag,
    u32 cur,
    u32 ni,
    const u8* riv,
    const u16* rdist,
    u16 w,
    u16 h) 
{
    const bool beside_c = tile_beside_riv(riv, w, h, cur);
    const bool beside_n = tile_beside_riv(riv, w, h, ni);
    if (riv[cur] != 0 && riv[ni] != 0 && rdist[cur] != k_rdist_none && rdist[ni] != k_rdist_none
        && rdist[ni] > rdist[cur]) {
        return diag ? static_cast<u32>(P1_DELTA_SWAMP_COST_DIAG_RIV_UP)
            : static_cast<u32>(P1_DELTA_SWAMP_COST_RIV_UP);
    }
    if (riv[cur] != 0 || riv[ni] != 0 || beside_c || beside_n) {
        return diag ? static_cast<u32>(P1_DELTA_SWAMP_COST_DIAG_RIV_SIDE)
            : static_cast<u32>(P1_DELTA_SWAMP_COST_RIV_SIDE);
    }
    return diag ? static_cast<u32>(P1_DELTA_SWAMP_COST_DIAG_NORM)
        : static_cast<u32>(P1_DELTA_SWAMP_COST_NORM);
}

static u8 tile_water_sc (const u8* terrain, const u8* climate, u32 i) {
    const u8 ter = terrain[i];
    if (!is_land(ter)) {
        return 0;
    }
    if (ter == TERR_MOUNTAINS[0]) {
        return static_cast<u8>(P1_DELTA_SWAMP_SCORE_MTN);
    }
    if (ter == TERR_HILLS[0]) {
        return static_cast<u8>(P1_DELTA_SWAMP_SCORE_HILLS);
    }
    const u8 cl = climate[i];
    if (cl == CLIMATE_DESERT) {
        return static_cast<u8>(P1_DELTA_SWAMP_SCORE_DESERT);
    }
    if (cl == CLIMATE_PLAINS) {
        return static_cast<u8>(P1_DELTA_SWAMP_SCORE_PLAINS);
    }
    if (cl == CLIMATE_GRASSLAND) {
        return static_cast<u8>(P1_DELTA_SWAMP_SCORE_GRASS);
    }
    if (cl == CLIMATE_BLACK_SOIL) {
        return static_cast<u8>(P1_DELTA_SWAMP_SCORE_BLACK);
    }
    return 0;
}

static bool res_ov_free (u8 v) {
    return v == 0u || v == static_cast<u8>(OVERLAY_NONE);
}

static bool set_swamp_tile (
    u16 bidx,
    const u16* wshed,
    const u8* terrain,
    u8* climate,
    u8* res_ov,
    u32 i) 
{
    if (wshed[i] != bidx || !is_plains_terr(terrain[i]) || !res_ov_free(res_ov[i])) {
        return false;
    }
    res_ov[i] = static_cast<u8>(RES_OV_SWAMPS);
    if (climate[i] == CLIMATE_DESERT || climate[i] == CLIMATE_PLAINS) {
        climate[i] = CLIMATE_GRASSLAND;
    }
    return true;
}

static u16 max_wshed_bidx (const u16* wshed, u32 n) {
    u16 mx = 0;
    for (u32 i = 0; i < n; ++i) {
        if (wshed[i] > mx) {
            mx = wshed[i];
        }
    }
    return mx;
}

static bool build_basin_sc (
    const u16* wshed,
    const u8* terrain,
    const u8* climate,
    u16 max_b,
    u32 n,
    u16* land_lo,
    u16* land_hi,
    u16* water_lo,
    u16* water_hi,
    u16* swamp_lo,
    u16* swamp_hi,
    u32* tot_land) 
{
    if (wshed == nullptr || terrain == nullptr || climate == nullptr || land_lo == nullptr || land_hi == nullptr
        || water_lo == nullptr || water_hi == nullptr || swamp_lo == nullptr || swamp_hi == nullptr
        || tot_land == nullptr) {
        return false;
    }
    const u32 cap = static_cast<u32>(max_b) + 1u;
    *tot_land = 0;
    for (u32 b = 0; b < cap; ++b) {
        wb_wr32(land_lo, land_hi, b, 0u);
        wb_wr32(water_lo, water_hi, b, 0u);
        wb_wr32(swamp_lo, swamp_hi, b, 0u);
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 bidx = wshed[i];
        if (bidx == static_cast<u16>(P1_RIVER_BASIN_NONE) || bidx > max_b) {
            continue;
        }
        if (!is_land(terrain[i])) {
            continue;
        }
        const u32 lb = static_cast<u32>(bidx);
        wb_wr32(land_lo, land_hi, lb, wb_rd32(land_lo, land_hi, lb) + 1u);
        (*tot_land)++;
        wb_wr32(water_lo, water_hi, lb,
            wb_rd32(water_lo, water_hi, lb) + static_cast<u32>(tile_water_sc(terrain, climate, i)));
    }
    for (u32 b = 1; b < cap; ++b) {
        const u32 land_n = wb_rd32(land_lo, land_hi, b);
        const u32 water = wb_rd32(water_lo, water_hi, b);
        if (land_n == 0u || water == 0u) {
            continue;
        }
        u32 swamp_q = (water * static_cast<u32>(P1_DELTA_SWAMP_WATER_MUL))
            / static_cast<u32>(P1_DELTA_SWAMP_WATER_DIV);
        if (swamp_q == 0u) {
            swamp_q = 1u;
        }
        wb_wr32(swamp_lo, swamp_hi, b, swamp_q);
    }
    return true;
}

static bool find_basin_mouth (
    u16 bidx,
    const u16* wshed,
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u8* vis,
    u16* qlo,
    u16* qhi,
    u32* mouth_i) 
{
    if (wshed == nullptr || terrain == nullptr || riv == nullptr || mouth_i == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 best_i = n;
    u32 best_reach = 0;
    for (u32 i = 0; i < n; ++i) {
        if (wshed[i] != bidx || riv[i] == 0 || !is_plains_terr(terrain[i])) {
            continue;
        }
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (!tile_adj4_open_water(terrain, w, h, x, y)) {
            continue;
        }
        std::memset(vis, 0, n);
        const u32 reach = river_reach_n(bidx, wshed, riv, w, h, i, vis, qlo, qhi);
        if (reach > best_reach || (reach == best_reach && i < best_i)) {
            best_reach = reach;
            best_i = i;
        }
    }
    if (best_i >= n) {
        return false;
    }
    *mouth_i = best_i;
    return true;
}

static u32 wshed_flood_swamps (
    u16 bidx,
    u32 swamp_q,
    const u16* wshed,
    const u8* terrain,
    const u8* riv,
    u8* climate,
    u8* res_ov,
    u16 w,
    u16 h,
    u8* fl,
    u16* qlo,
    u16* qhi,
    u16* clo,
    u16* chi,
    u16* rdist) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 mouth_i = 0;
    if (!find_basin_mouth(bidx, wshed, terrain, riv, w, h, fl, qlo, qhi, &mouth_i)) {
        return 0;
    }
    if (wshed[mouth_i] != bidx || !is_plains_terr(terrain[mouth_i])) {
        return 0;
    }
    const u16 mx = static_cast<u16>(mouth_i % static_cast<u32>(w));
    const u16 my = static_cast<u16>(mouth_i / static_cast<u32>(w));
    const u32 max_r = isqrt_u32(swamp_q) + 2u;
    const u32 max_r_sq = max_r * max_r;
    const u32 max_cost = swamp_q * static_cast<u32>(P1_DELTA_SWAMP_COST_NORM);
    build_river_up_dist(bidx, wshed, riv, w, h, mouth_i, rdist, qlo, qhi);
    for (u32 i = 0; i < n; ++i) {
        wb_wr32(clo, chi, i, k_cost_inf);
        fl[i] = 0;
    }
    u32 ln = 0;
    wb_wr32(clo, chi, mouth_i, 0u);
    fl[mouth_i] = 1;
    q_set(qlo, qhi, ln++, mouth_i);
    u32 placed = 0;
    while (ln > 0u && placed < swamp_q) {
        u32 best_q = 0;
        u32 best_c = k_cost_inf;
        for (u32 li = 0; li < ln; ++li) {
            const u32 ci = q_get(qlo, qhi, li);
            const u32 cc = wb_rd32(clo, chi, ci);
            if (cc < best_c) {
                best_c = cc;
                best_q = li;
            }
        }
        const u32 i = q_get(qlo, qhi, best_q);
        q_set(qlo, qhi, best_q, q_get(qlo, qhi, ln - 1u));
        ln--;
        if (fl[i] == 2u) {
            continue;
        }
        fl[i] = 2;
        if (wshed[i] == bidx && set_swamp_tile(bidx, wshed, terrain, climate, res_ov, i)) {
            placed++;
        }
        const u16 px = static_cast<u16>(i % static_cast<u32>(w));
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        for (i32 k = 0; k < 8; ++k) {
            const bool diag = (k_dx8[k] != 0 && k_dy8[k] != 0);
            const i32 nx = static_cast<i32>(px) + k_dx8[k];
            const i32 ny = static_cast<i32>(py) + k_dy8[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u16 ax = static_cast<u16>(nx);
            const u16 ay = static_cast<u16>(ny);
            if (!in_eucl_rng_sq(mx, my, ax, ay, max_r_sq)) {
                continue;
            }
            const u32 ni = tidx(w, ax, ay);
            if (wshed[ni] != bidx || !is_plains_terr(terrain[ni]) || fl[ni] == 2u) {
                continue;
            }
            const u32 nc = wb_rd32(clo, chi, i) + move_cost(diag, i, ni, riv, rdist, w, h);
            if (nc > max_cost) {
                continue;
            }
            const u32 oc = wb_rd32(clo, chi, ni);
            if (oc == k_cost_inf || nc < oc) {
                wb_wr32(clo, chi, ni, nc);
                if (fl[ni] == 0u) {
                    fl[ni] = 1;
                    q_set(qlo, qhi, ln++, ni);
                    if (ln > n) {
                        return placed;
                    }
                }
            }
        }
    }
    return placed;
}

static void sort_basins_dn (
    u16* ord,
    u32 ord_n,
    const u16* land_lo,
    const u16* land_hi) 
{
    for (u32 i = 0; i + 1u < ord_n; ++i) {
        for (u32 j = i + 1u; j < ord_n; ++j) {
            const u32 li = wb_rd32(land_lo, land_hi, ord[i]);
            const u32 lj = wb_rd32(land_lo, land_hi, ord[j]);
            if (lj > li) {
                const u16 t = ord[i];
                ord[i] = ord[j];
                ord[j] = t;
            }
        }
    }
}

static u32 place_basin_swamps (
    u16 bidx,
    u32 swamp_q,
    const u16* wshed,
    const u8* terrain,
    const u8* riv,
    u8* climate,
    u8* res_ov,
    u16 w,
    u16 h,
    u8* fl,
    u16* qlo,
    u16* qhi,
    u16* clo,
    u16* chi,
    u16* rdist) 
{
    return wshed_flood_swamps(
        bidx, swamp_q, wshed, terrain, riv, climate, res_ov, w, h, fl, qlo, qhi, clo, chi, rdist);
}

static bool apply_delta_swamps (
    const P1_Adj_DeltaSwampsIn& in,
    u16 w,
    u16 h,
    u16 tgt_pct,
    u32* swamp_n,
    u32* delta_n) 
{
    if (in.m_wshed == nullptr || in.m_terrain == nullptr || in.m_climate == nullptr
        || in.m_res_ov == nullptr || in.m_riv == nullptr || swamp_n == nullptr || delta_n == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0) {
        return false;
    }
    const u16 max_b = max_wshed_bidx(in.m_wshed, n);
    if (max_b == 0) {
        *swamp_n = 0;
        *delta_n = 0;
        return true;
    }
    const u32 cap = static_cast<u32>(max_b) + 1u;
    Whiteboard_1B wb_fl("P1_Adj_DeltaSwamps", "fl", 0u);
    P1_WB_CHK(wb_fl);
    Whiteboard_2B wb_qlo("P1_Adj_DeltaSwamps", "qlo", 0u);
    P1_WB_CHK(wb_qlo);
    Whiteboard_2B wb_qhi("P1_Adj_DeltaSwamps", "qhi", 0u);
    P1_WB_CHK(wb_qhi);
    Whiteboard_2B wb_clo("P1_Adj_DeltaSwamps", "clo", 0u);
    P1_WB_CHK(wb_clo);
    Whiteboard_2B wb_chi("P1_Adj_DeltaSwamps", "chi", 0u);
    P1_WB_CHK(wb_chi);
    Whiteboard_2B wb_rdist("P1_Adj_DeltaSwamps", "rdist", 0u);
    P1_WB_CHK(wb_rdist);
    Whiteboard_2B wb_land_lo("P1_Adj_DeltaSwamps", "land_lo", 0u);
    P1_WB_CHK(wb_land_lo);
    Whiteboard_2B wb_land_hi("P1_Adj_DeltaSwamps", "land_hi", 0u);
    P1_WB_CHK(wb_land_hi);
    Whiteboard_2B wb_water_lo("P1_Adj_DeltaSwamps", "water_lo", 0u);
    P1_WB_CHK(wb_water_lo);
    Whiteboard_2B wb_water_hi("P1_Adj_DeltaSwamps", "water_hi", 0u);
    P1_WB_CHK(wb_water_hi);
    Whiteboard_2B wb_swamp_lo("P1_Adj_DeltaSwamps", "swamp_lo", 0u);
    P1_WB_CHK(wb_swamp_lo);
    Whiteboard_2B wb_swamp_hi("P1_Adj_DeltaSwamps", "swamp_hi", 0u);
    P1_WB_CHK(wb_swamp_hi);
    Whiteboard_2B wb_ord("P1_Adj_DeltaSwamps", "ord", 0u);
    P1_WB_CHK(wb_ord);
    u8* fl = wb_fl.get_iter_ptr();
    u16* qlo = wb_qlo.get_iter_ptr();
    u16* qhi = wb_qhi.get_iter_ptr();
    u16* clo = wb_clo.get_iter_ptr();
    u16* chi = wb_chi.get_iter_ptr();
    u16* rdist = wb_rdist.get_iter_ptr();
    u16* land_lo = wb_land_lo.get_iter_ptr();
    u16* land_hi = wb_land_hi.get_iter_ptr();
    u16* water_lo = wb_water_lo.get_iter_ptr();
    u16* water_hi = wb_water_hi.get_iter_ptr();
    u16* swamp_lo = wb_swamp_lo.get_iter_ptr();
    u16* swamp_hi = wb_swamp_hi.get_iter_ptr();
    u16* ord = wb_ord.get_iter_ptr();
    u32 tot_land = 0;
    if (!build_basin_sc(in.m_wshed, in.m_terrain, in.m_climate, max_b, n,
            land_lo, land_hi, water_lo, water_hi, swamp_lo, swamp_hi, &tot_land)) {
        return false;
    }
    const u32 swamp_budget = (tot_land * static_cast<u32>(tgt_pct)) / 100u;
    u32 ord_n = 0;
    for (u32 b = 1; b < cap; ++b) {
        if (wb_rd32(land_lo, land_hi, b) == 0u) {
            continue;
        }
        ord[ord_n++] = static_cast<u16>(b);
    }
    sort_basins_dn(ord, ord_n, land_lo, land_hi);
    u32 tot_sw = 0;
    u32 tot_dl = 0;
    u32 swamp_left = swamp_budget;
    for (u32 k = 0; k < ord_n && swamp_left > 0u; ++k) {
        const u16 bidx = ord[k];
        u32 basin_q = wb_rd32(swamp_lo, swamp_hi, bidx);
        if (basin_q == 0u) {
            continue;
        }
        const u32 land_n = wb_rd32(land_lo, land_hi, bidx);
        if (basin_q > land_n) {
            basin_q = land_n;
        }
        if (basin_q > swamp_left) {
            basin_q = swamp_left;
        }
        std::memset(fl, 0, n);
        const u32 pn = place_basin_swamps(
            bidx, basin_q, in.m_wshed, in.m_terrain, in.m_riv, in.m_climate, in.m_res_ov,
            w, h, fl, qlo, qhi, clo, chi, rdist);
        if (pn == 0u) {
            continue;
        }
        tot_sw += pn;
        tot_dl++;
        if (pn >= swamp_left) {
            swamp_left = 0;
        } else {
            swamp_left -= pn;
        }
    }
    *swamp_n = tot_sw;
    *delta_n = tot_dl;
    return true;
}

//================================================================================================================================
//=> - P1_Adj_DeltaSwamps -
//================================================================================================================================

P1_Adj_DeltaSwamps::P1_Adj_DeltaSwamps (const P1_RunPrm& prm, const P1_Adj_DeltaSwampsPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_adjust(false),
    m_swamp_n(0),
    m_delta_n(0) {
}

bool P1_Adj_DeltaSwamps::adjust (const P1_Adj_DeltaSwampsIn& in, u16 w, u16 h) {
    m_valid_adjust = false;
    m_swamp_n = 0;
    m_delta_n = 0;
    if (!p1_run_prm_ok(m_prm) || w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!apply_delta_swamps(in, w, h, m_sp.m_tgt_pct, &m_swamp_n, &m_delta_n)) {
        return false;
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_DeltaSwamps::is_valid () const {
    return m_valid_adjust;
}

u32 P1_Adj_DeltaSwamps::swamp_n () const {
    return m_swamp_n;
}

u32 P1_Adj_DeltaSwamps::delta_n () const {
    return m_delta_n; 
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
