//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <vector>

#include "p1_adj_delta_swamps.h"
#include "game_map_defs.h"
#include "p1_gen_river_network.h"
#include "p1_wb_util.h"
#include "profile_time.h"

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
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w) && static_cast<u32>(y) < static_cast<u32>(h);
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
    PTIME_START(__func__);
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        if (is_water(terrain[tidx(w, static_cast<u16>(nx), static_cast<u16>(ny))])) {
            PTIME_STOP(__func__);
            return true;
        }
    }
    PTIME_STOP(__func__);
    return false;
}

static u32 isqrt_u32 (u32 v) {
    PTIME_START(__func__);
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
    PTIME_STOP(__func__);
    return r;
}

static bool in_eucl_rng_sq (u16 mx, u16 my, u16 x, u16 y, u32 max_r_sq) {
    PTIME_START(__func__);
    const i32 dx = static_cast<i32>(x) - static_cast<i32>(mx);
    const i32 dy = static_cast<i32>(y) - static_cast<i32>(my);
    const u32 d2 = static_cast<u32>(dx * dx + dy * dy);
    PTIME_STOP(__func__);
    return d2 <= max_r_sq;
}

static void build_beside_riv (const u8* riv, u16 w, u16 h, u32 n, u8* beside) {
    PTIME_START(__func__);
    for (u32 i = 0; i < n; ++i) {
        beside[i] = 0;
    }
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0) {
            continue;
        }
        const u16 px = static_cast<u16>(i % static_cast<u32>(w));
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
            if (riv[ni] == 0) {
                beside[ni] = 1;
            }
        }
    }
    PTIME_STOP(__func__);
}

static void hswap (u16* qlo, u16* qhi, u32 base, u32 a, u32 b) {
    const u32 ta = q_get(qlo, qhi, base + a);
    q_set(qlo, qhi, base + a, q_get(qlo, qhi, base + b));
    q_set(qlo, qhi, base + b, ta);
}

static u32 hcost (const u16* clo, const u16* chi, const u16* qlo, const u16* qhi, u32 base, u32 k) {
    return wb_rd32(clo, chi, q_get(qlo, qhi, base + k));
}

static void hsift_up (u16* qlo, u16* qhi, u16* clo, u16* chi, u32 base, u32 k) {
    while (k > 0u) {
        const u32 p = (k - 1u) >> 1;
        if (hcost(clo, chi, qlo, qhi, base, k) >= hcost(clo, chi, qlo, qhi, base, p)) {
            break;
        }
        hswap(qlo, qhi, base, k, p);
        k = p;
    }
}

static void hsift_dn (u16* qlo, u16* qhi, u16* clo, u16* chi, u32 base, u32 k, u32 sz) {
    for (;;) {
        u32 m = k;
        const u32 l = (k << 1) + 1u;
        const u32 r = l + 1u;
        if (l < sz && hcost(clo, chi, qlo, qhi, base, l) < hcost(clo, chi, qlo, qhi, base, m)) {
            m = l;
        }
        if (r < sz && hcost(clo, chi, qlo, qhi, base, r) < hcost(clo, chi, qlo, qhi, base, m)) {
            m = r;
        }
        if (m == k) {
            break;
        }
        hswap(qlo, qhi, base, k, m);
        k = m;
    }
}

static void hpush (u16* qlo, u16* qhi, u16* clo, u16* chi, u32 base, u32* sz, u32 tile) {
    q_set(qlo, qhi, base + *sz, tile);
    hsift_up(qlo, qhi, clo, chi, base, *sz);
    (*sz)++;
}

static u32 hpop (u16* qlo, u16* qhi, u16* clo, u16* chi, u32 base, u32* sz) {
    const u32 tile = q_get(qlo, qhi, base);
    (*sz)--;
    if (*sz > 0u) {
        q_set(qlo, qhi, base, q_get(qlo, qhi, base + *sz));
        hsift_dn(qlo, qhi, clo, chi, base, 0u, *sz);
    }
    return tile;
}

static bool tile_beside_riv (const u8* riv, u16 w, u16 h, u32 i) {
    PTIME_START(__func__);
    if (riv[i] != 0) {
        PTIME_STOP(__func__);
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
            PTIME_STOP(__func__);
            return true;
        }
    }
    PTIME_STOP(__func__);
    return false;
}

static u32 river_reach_n (
    u16 bidx,
    const u16* wshed,
    const u8* riv,
    u16 w,
    u16 h,
    u32 seed_i,
    u32 qbase,
    u16* vis_st,
    u16 vis_gen,
    u16* qlo,
    u16* qhi) 
{
    PTIME_START(__func__);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 qn = 0;
    u32 cnt = 0;
    vis_st[seed_i] = vis_gen;
    q_set(qlo, qhi, qbase + qn++, seed_i);
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q_get(qlo, qhi, qbase + qh);
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
            if (vis_st[ni] == vis_gen || wshed[ni] != bidx || riv[ni] == 0) {
                continue;
            }
            vis_st[ni] = vis_gen;
            q_set(qlo, qhi, qbase + qn++, ni);
            if (qn + qbase > n) {
                PTIME_STOP(__func__);
                return cnt;
            }
        }
    }
    PTIME_STOP(__func__);
    return cnt;
}

static void build_river_up_dist (
    u16 bidx,
    const u16* wshed,
    const u8* riv,
    u16 w,
    u16 h,
    u32 mouth_i,
    u32 qbase,
    u16 rd_gen,
    u16* rd_st,
    u16* rdist,
    u16* qlo,
    u16* qhi) 
{
    PTIME_START(__func__);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 qn = 0;
    rdist[mouth_i] = 0;
    rd_st[mouth_i] = rd_gen;
    q_set(qlo, qhi, qbase + qn++, mouth_i);
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q_get(qlo, qhi, qbase + qh);
        const u16 px = static_cast<u16>(i % static_cast<u32>(w));
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
            if (wshed[ni] != bidx || riv[ni] == 0 || rd_st[ni] == rd_gen) {
                continue;
            }
            rd_st[ni] = rd_gen;
            rdist[ni] = static_cast<u16>(rdist[i] + 1u);
            q_set(qlo, qhi, qbase + qn++, ni);
            if (qn + qbase > n) {
                PTIME_STOP(__func__);
                return;
            }
        }
    }
    PTIME_STOP(__func__);
}

static u32 move_cost (
    bool diag,
    u32 cur,
    u32 ni,
    const u8* riv,
    const u8* beside,
    u16 rd_gen,
    const u16* rd_st,
    const u16* rdist) 
{
    PTIME_START(__func__);
    const bool beside_c = beside[cur] != 0;
    const bool beside_n = beside[ni] != 0;
    const bool rd_c = rd_st[cur] == rd_gen;
    const bool rd_n = rd_st[ni] == rd_gen;
    if (riv[cur] != 0 && riv[ni] != 0 && rd_c && rd_n && rdist[ni] > rdist[cur]) {
        PTIME_STOP(__func__);
        return diag ? static_cast<u32>(P1_DELTA_SWAMP_COST_DIAG_RIV_UP) : static_cast<u32>(P1_DELTA_SWAMP_COST_RIV_UP);
    }
    if (riv[cur] != 0 || riv[ni] != 0 || beside_c || beside_n) {
        PTIME_STOP(__func__);
        return diag ? static_cast<u32>(P1_DELTA_SWAMP_COST_DIAG_RIV_SIDE) : static_cast<u32>(P1_DELTA_SWAMP_COST_RIV_SIDE);
    }
    PTIME_STOP(__func__);
    return diag ? static_cast<u32>(P1_DELTA_SWAMP_COST_DIAG_NORM) : static_cast<u32>(P1_DELTA_SWAMP_COST_NORM);
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
    PTIME_START(__func__);
    if (wshed[i] != bidx || !is_plains_terr(terrain[i]) || !res_ov_free(res_ov[i])) {
        PTIME_STOP(__func__);
        return false;
    }
    res_ov[i] = OV_SWAMP[0];
    if (climate[i] == CLIMATE_DESERT || climate[i] == CLIMATE_PLAINS) {
        climate[i] = CLIMATE_GRASSLAND;
    }
    PTIME_STOP(__func__);
    return true;
}

static u16 max_wshed_bidx (const u16* wshed, u32 n) {
    PTIME_START(__func__);
    u16 mx = 0;
    for (u32 i = 0; i < n; ++i) {
        if (wshed[i] > mx) {
            mx = wshed[i];
        }
    }
    PTIME_STOP(__func__);
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
    PTIME_START(__func__);
    if (wshed == nullptr || terrain == nullptr || climate == nullptr || land_lo == nullptr || land_hi == nullptr
        || water_lo == nullptr || water_hi == nullptr || swamp_lo == nullptr || swamp_hi == nullptr
        || tot_land == nullptr) {
        PTIME_STOP(__func__);
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
    PTIME_STOP(__func__);
    return true;
}

static u32 build_mouth_cands (
    const u16* wshed,
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u32 cap,
    u32 n,
    u16* qlo,
    u16* qhi,
    u16* mc_off_lo,
    u16* mc_off_hi,
    u16* mc_cnt_lo,
    u16* mc_cnt_hi) 
{
    PTIME_START(__func__);
    for (u32 b = 0; b < cap; ++b) {
        wb_wr32(mc_off_lo, mc_off_hi, b, 0u);
        wb_wr32(mc_cnt_lo, mc_cnt_hi, b, 0u);
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 bidx = wshed[i];
        if (bidx == static_cast<u16>(P1_RIVER_BASIN_NONE) || static_cast<u32>(bidx) >= cap) {
            continue;
        }
        if (riv[i] == 0 || !is_plains_terr(terrain[i])) {
            continue;
        }
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (!tile_adj4_open_water(terrain, w, h, x, y)) {
            continue;
        }
        const u32 lb = static_cast<u32>(bidx);
        wb_wr32(mc_cnt_lo, mc_cnt_hi, lb, wb_rd32(mc_cnt_lo, mc_cnt_hi, lb) + 1u);
    }
    u32 tot = 0;
    for (u32 b = 0; b < cap; ++b) {
        wb_wr32(mc_off_lo, mc_off_hi, b, tot);
        tot += wb_rd32(mc_cnt_lo, mc_cnt_hi, b);
    }
    for (u32 b = 0; b < cap; ++b) {
        wb_wr32(mc_cnt_lo, mc_cnt_hi, b, 0u);
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 bidx = wshed[i];
        if (bidx == static_cast<u16>(P1_RIVER_BASIN_NONE) || static_cast<u32>(bidx) >= cap) {
            continue;
        }
        if (riv[i] == 0 || !is_plains_terr(terrain[i])) {
            continue;
        }
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (!tile_adj4_open_water(terrain, w, h, x, y)) {
            continue;
        }
        const u32 lb = static_cast<u32>(bidx);
        const u32 slot = wb_rd32(mc_off_lo, mc_off_hi, lb) + wb_rd32(mc_cnt_lo, mc_cnt_hi, lb);
        q_set(qlo, qhi, slot, i);
        wb_wr32(mc_cnt_lo, mc_cnt_hi, lb, wb_rd32(mc_cnt_lo, mc_cnt_hi, lb) + 1u);
    }
    PTIME_STOP(__func__);
    return tot;
}

static bool find_basin_mouth (
    u16 bidx,
    const u16* wshed,
    const u8* riv,
    u16 w,
    u16 h,
    u32 mc_qbase,
    u16* vis_st,
    u16* qlo,
    u16* qhi,
    const u16* mc_off_lo,
    const u16* mc_off_hi,
    const u16* mc_cnt_lo,
    const u16* mc_cnt_hi,
    u32* mouth_i) 
{
    PTIME_START(__func__);
    if (wshed == nullptr || riv == nullptr || mouth_i == nullptr) {
        PTIME_STOP(__func__);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 off = wb_rd32(mc_off_lo, mc_off_hi, static_cast<u32>(bidx));
    const u32 cnt = wb_rd32(mc_cnt_lo, mc_cnt_hi, static_cast<u32>(bidx));
    u32 best_i = n;
    u32 best_reach = 0;
    u16 vis_gen = 1;
    for (u32 k = 0; k < cnt; ++k) {
        const u32 i = q_get(qlo, qhi, off + k);
        const u32 reach = river_reach_n(bidx, wshed, riv, w, h, i, mc_qbase, vis_st, vis_gen++, qlo, qhi);
        if (reach > best_reach || (reach == best_reach && i < best_i)) {
            best_reach = reach;
            best_i = i;
        }
    }
    if (best_i >= n) {
        PTIME_STOP(__func__);
        return false;
    }
    *mouth_i = best_i;
    PTIME_STOP(__func__);
    return true;
}

static u32 wshed_flood_swamps (
    u16 bidx,
    u32 swamp_q,
    const u16* wshed,
    const u8* terrain,
    const u8* riv,
    const u8* beside,
    u8* climate,
    u8* res_ov,
    u16 w,
    u16 h,
    u32 mc_qbase,
    u16 fl_gen,
    u16* fl_st,
    u16* qlo,
    u16* qhi,
    u16* clo,
    u16* chi,
    u16* rdist,
    const u16* mc_off_lo,
    const u16* mc_off_hi,
    const u16* mc_cnt_lo,
    const u16* mc_cnt_hi) 
{
    PTIME_START(__func__);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u16 fl_done = static_cast<u16>(fl_gen + 1u);
    u32 mouth_i = 0;
    if (!find_basin_mouth(bidx, wshed, riv, w, h, mc_qbase, chi, qlo, qhi,
            mc_off_lo, mc_off_hi, mc_cnt_lo, mc_cnt_hi, &mouth_i)) {
        PTIME_STOP(__func__);
        return 0;
    }
    if (wshed[mouth_i] != bidx || !is_plains_terr(terrain[mouth_i])) {
        PTIME_STOP(__func__);
        return 0;
    }
    const u16 mx = static_cast<u16>(mouth_i % static_cast<u32>(w));
    const u16 my = static_cast<u16>(mouth_i / static_cast<u32>(w));
    const u32 max_r = isqrt_u32(swamp_q) + 2u;
    const u32 max_r_sq = max_r * max_r;
    const u32 max_cost = swamp_q * static_cast<u32>(P1_DELTA_SWAMP_COST_NORM);
    const u16 rd_gen = 1;
    build_river_up_dist(bidx, wshed, riv, w, h, mouth_i, mc_qbase, rd_gen, chi, rdist, qlo, qhi);
    u32 hsz = 0;
    wb_wr32(clo, chi, mouth_i, 0u);
    fl_st[mouth_i] = fl_gen;
    hpush(qlo, qhi, clo, chi, mc_qbase, &hsz, mouth_i);
    u32 placed = 0;
    while (hsz > 0u && placed < swamp_q) {
        const u32 i = hpop(qlo, qhi, clo, chi, mc_qbase, &hsz);
        if (fl_st[i] == fl_done) {
            continue;
        }
        if (fl_st[i] != fl_gen) {
            continue;
        }
        fl_st[i] = fl_done;
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
            if (wshed[ni] != bidx || !is_plains_terr(terrain[ni]) || fl_st[ni] == fl_done) {
                continue;
            }
            const u32 nc = wb_rd32(clo, chi, i) + move_cost(diag, i, ni, riv, beside, rd_gen, chi, rdist);
            if (nc > max_cost) {
                continue;
            }
            const u32 oc = (fl_st[ni] != fl_gen) ? k_cost_inf : wb_rd32(clo, chi, ni);
            if (nc < oc) {
                wb_wr32(clo, chi, ni, nc);
                if (fl_st[ni] != fl_gen) {
                    fl_st[ni] = fl_gen;
                }
                hpush(qlo, qhi, clo, chi, mc_qbase, &hsz, ni);
                if (hsz + mc_qbase > n) {
                    PTIME_STOP(__func__);
                    return placed;
                }
            }
        }
    }
    PTIME_STOP(__func__);
    return placed;
}

static bool basin_land_gt (u16 a, u16 b, const u16* land_lo, const u16* land_hi) {
    return wb_rd32(land_lo, land_hi, a) > wb_rd32(land_lo, land_hi, b);
}

static void qsort_basins_dn (u16* ord, u32 lo, u32 hi, const u16* land_lo, const u16* land_hi) {
    if (lo >= hi) {
        return;
    }
    const u16 piv = ord[hi];
    u32 i = lo;
    for (u32 j = lo; j < hi; ++j) {
        if (basin_land_gt(ord[j], piv, land_lo, land_hi)) {
            const u16 t = ord[i];
            ord[i] = ord[j];
            ord[j] = t;
            i++;
        }
    }
    const u16 t = ord[i];
    ord[i] = ord[hi];
    ord[hi] = t;
    if (i > 0u) {
        qsort_basins_dn(ord, lo, i - 1u, land_lo, land_hi);
    }
    qsort_basins_dn(ord, i + 1u, hi, land_lo, land_hi);
}

static void sort_basins_dn (
    u16* ord,
    u32 ord_n,
    const u16* land_lo,
    const u16* land_hi) 
{
    PTIME_START(__func__);
    if (ord_n <= 1u) {
        PTIME_STOP(__func__);
        return;
    }
    qsort_basins_dn(ord, 0u, ord_n - 1u, land_lo, land_hi);
    PTIME_STOP(__func__);
}

static u32 place_basin_swamps (
    u16 bidx,
    u32 swamp_q,
    const u16* wshed,
    const u8* terrain,
    const u8* riv,
    const u8* beside,
    u8* climate,
    u8* res_ov,
    u16 w,
    u16 h,
    u32 mc_qbase,
    u16 fl_gen,
    u16* fl_st,
    u16* qlo,
    u16* qhi,
    u16* clo,
    u16* chi,
    u16* rdist,
    const u16* mc_off_lo,
    const u16* mc_off_hi,
    const u16* mc_cnt_lo,
    const u16* mc_cnt_hi) 
{
    return wshed_flood_swamps(
        bidx, swamp_q, wshed, terrain, riv, beside, climate, res_ov, w, h, mc_qbase, fl_gen, fl_st,
        qlo, qhi, clo, chi, rdist, mc_off_lo, mc_off_hi, mc_cnt_lo, mc_cnt_hi);
}

static bool apply_delta_swamps (
    const P1_Adj_DeltaSwampsIn& in,
    u16 w,
    u16 h,
    u16 tgt_pct,
    u32 seed,
    u32* swamp_n,
    u32* delta_n) 
{
    PTIME_START(__func__);
    if (in.m_wshed == nullptr || in.m_terrain == nullptr || in.m_climate == nullptr
        || in.m_res_ov == nullptr || in.m_riv == nullptr || swamp_n == nullptr || delta_n == nullptr) {
        PTIME_STOP(__func__);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0) {
        PTIME_STOP(__func__);
        return false;
    }
    const u16 max_b = max_wshed_bidx(in.m_wshed, n);
    if (max_b == 0) {
        *swamp_n = 0;
        *delta_n = 0;
        PTIME_STOP(__func__);
        return true;
    }
    const u32 cap = static_cast<u32>(max_b) + 1u;
    Whiteboard_1B wb_fl("P1_Adj_DeltaSwamps", "fl", seed);
    P1_WB_CHK(wb_fl);
    Whiteboard_2B wb_qlo("P1_Adj_DeltaSwamps", "qlo", seed);
    P1_WB_CHK(wb_qlo);
    Whiteboard_2B wb_qhi("P1_Adj_DeltaSwamps", "qhi", seed);
    P1_WB_CHK(wb_qhi);
    Whiteboard_2B wb_clo("P1_Adj_DeltaSwamps", "clo", seed);
    P1_WB_CHK(wb_clo);
    Whiteboard_2B wb_chi("P1_Adj_DeltaSwamps", "chi", seed);
    P1_WB_CHK(wb_chi);
    Whiteboard_2B wb_rdist("P1_Adj_DeltaSwamps", "rdist", seed);
    P1_WB_CHK(wb_rdist);
    Whiteboard_2B wb_land_lo("P1_Adj_DeltaSwamps", "land_lo", seed);
    P1_WB_CHK(wb_land_lo);
    Whiteboard_2B wb_land_hi("P1_Adj_DeltaSwamps", "land_hi", seed);
    P1_WB_CHK(wb_land_hi);
    Whiteboard_2B wb_water_lo("P1_Adj_DeltaSwamps", "water_lo", seed);
    P1_WB_CHK(wb_water_lo);
    Whiteboard_2B wb_water_hi("P1_Adj_DeltaSwamps", "water_hi", seed);
    P1_WB_CHK(wb_water_hi);
    Whiteboard_2B wb_swamp_lo("P1_Adj_DeltaSwamps", "swamp_lo", seed);
    P1_WB_CHK(wb_swamp_lo);
    Whiteboard_2B wb_swamp_hi("P1_Adj_DeltaSwamps", "swamp_hi", seed);
    P1_WB_CHK(wb_swamp_hi);
    Whiteboard_2B wb_ord("P1_Adj_DeltaSwamps", "ord", seed);
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
    build_beside_riv(in.m_riv, w, h, n, fl);
    std::vector<u16> fl_st(n, 0);
    u32 tot_land = 0;
    if (!build_basin_sc(in.m_wshed, in.m_terrain, in.m_climate, max_b, n,
            land_lo, land_hi, water_lo, water_hi, swamp_lo, swamp_hi, &tot_land)) {
        PTIME_STOP(__func__);
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
    std::vector<u32> land_snap(cap);
    for (u32 b = 0; b < cap; ++b) {
        land_snap[b] = wb_rd32(land_lo, land_hi, b);
    }
    const u32 mc_qbase = build_mouth_cands(in.m_wshed, in.m_terrain, in.m_riv, w, h, cap, n, qlo, qhi, land_lo, land_hi, water_lo, water_hi);
    u32 tot_sw = 0;
    u32 tot_dl = 0;
    u32 swamp_left = swamp_budget;
    u16 fl_gen = 2;
    for (u32 k = 0; k < ord_n && swamp_left > 0u; ++k) {
        const u16 bidx = ord[k];
        u32 basin_q = wb_rd32(swamp_lo, swamp_hi, bidx);
        if (basin_q == 0u) {
            continue;
        }
        const u32 land_n = land_snap[static_cast<u32>(bidx)];
        if (basin_q > land_n) {
            basin_q = land_n;
        }
        if (basin_q > swamp_left) {
            basin_q = swamp_left;
        }
        const u32 pn = place_basin_swamps(
            bidx, basin_q, in.m_wshed, in.m_terrain, in.m_riv, fl, in.m_climate, in.m_res_ov,
            w, h, mc_qbase, fl_gen, fl_st.data(), qlo, qhi, clo, chi, rdist, land_lo, land_hi, water_lo, water_hi);
        fl_gen = static_cast<u16>(fl_gen + 2u);
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
    PTIME_STOP(__func__);
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
    PTIME_INIT();
    PTIME_START(__func__);
    m_valid_adjust = false;
    m_swamp_n = 0;
    m_delta_n = 0;
    if (!p1_run_prm_ok(m_prm) || w != m_prm.m_w || h != m_prm.m_h) {
        PTIME_STOP(__func__);
        return false;
    }
    if (!apply_delta_swamps(in, w, h, m_sp.m_tgt_pct, m_prm.m_seed, &m_swamp_n, &m_delta_n)) {
        PTIME_STOP(__func__);
        return false;
    }
    PTIME_STOP(__func__);
    PTIME_PRINT();
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

