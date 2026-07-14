//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "p1_gen_watershed_mountains.h"
#include "generator_constants.h"
#include "p1_wb_util.h"
#include "profile_time.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

#define P1_WSHED_STK_LIM 10240u
#define P1_WSHED_MOUTH_STK_N 2048u

struct BorderRec {
    u16 m_lo;
    u16 m_hi;
    u32 m_ti;
};

struct MouthTbl {
    u16* m_mx;
    u16* m_my;
    u8* m_has;
};

static u16 basin_lo (u16 a, u16 b) {
    return (a < b) ? a : b;
}

static u16 basin_hi (u16 a, u16 b) {
    return (a < b) ? b : a;
}

static u32 mouth_dist_sq (u16 a, u16 b, const u16* mx, const u16* my, const u8* has, u16 cap) {
    if (a == 0 || b == 0 || a >= cap || b >= cap || has[a] == 0 || has[b] == 0) {
        return 0;
    }
    const i32 dx = static_cast<i32>(mx[a]) - static_cast<i32>(mx[b]);
    const i32 dy = static_cast<i32>(my[a]) - static_cast<i32>(my[b]);
    return static_cast<u32>(dx * dx + dy * dy);
}

static u16 max_basin_idx_ov (u16 w, u16 h, const u16* ov) {
    if (ov == nullptr || w == 0 || h == 0) {
        return 0;
    }
    u16 mx = 0;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (ov[i] > mx) {
            mx = ov[i];
        }
    }
    return mx;
}

static void build_mouth_tbl (
    const P1_Gen_RiverNetworkRslt& network,
    const P1_Gen_RiverPtsRslt& pts,
    u16 w,
    u16 cap,
    u16* mx,
    u16* my,
    u8* has) 
{
    PTIME_START(__func__);
    for (u16 i = 0; i <= cap; ++i) {
        has[i] = 0;
    }
    if (network.m_downstream == nullptr || network.m_ov == nullptr || !pts.m_que.ok() || network.m_sector_n == 0) {
        PTIME_STOP(__func__);
        return;
    }
    for (u16 si = 0; si < network.m_sector_n; ++si) {
        if (network.m_downstream[si] != static_cast<u16>(P1_RIVER_DOWN_MOUTH)) {
            continue;
        }
        const u16 x = pts.m_que.x_at(static_cast<u32>(si));
        const u16 y = pts.m_que.y_at(static_cast<u32>(si));
        const u32 ti = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
        const u16 bid = network.m_ov[ti];
        if (bid == 0 || bid > cap) {
            continue;
        }
        has[bid] = 1;
        mx[bid] = x;
        my[bid] = y;
    }
    PTIME_STOP(__func__);
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_plains (u8 cls) {
    return cls == TERR_PLAINS[0];
}

static bool is_hills (u8 cls) {
    return cls == TERR_HILLS[0];
}

static void count_side_terr (
    u8 cls,
    u32 ti,
    const u8* noise_ov,
    u8 noise_thresh,
    u16* pln,
    u16* hil) 
{
    if (is_hills(cls)) {
        (*hil)++;
    } else if (is_plains(cls)) {
        if (noise_ov != nullptr) {
            if (noise_ov[ti] >= noise_thresh) {
                (*hil)++;
            } else {
                (*pln)++;
            }
        } else {
            (*pln)++;
        }
    } else if (noise_ov != nullptr && !is_water(cls)) {
        if (noise_ov[ti] >= noise_thresh) {
            (*hil)++;
        } else {
            (*pln)++;
        }
    }
}

static u16 pick_nb (
    u16 a,
    u16 w,
    u16 h,
    u32 ti,
    const u16* bov,
    const u16* mx,
    const u16* my,
    const u8* has,
    u16 cap) 
{
    const u16 px = static_cast<u16>(ti % static_cast<u32>(w));
    const u16 py = static_cast<u16>(ti / static_cast<u32>(w));
    u16 best = 0;
    u32 best_d = 0;
    if (px > 0) {
        const u16 b = bov[ti - 1u];
        if (b != a && b != static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            const u32 d = mouth_dist_sq(a, b, mx, my, has, cap);
            if (d > best_d) {
                best_d = d;
                best = b;
            }
        }
    }
    if (px + 1u < w) {
        const u16 b = bov[ti + 1u];
        if (b != a && b != static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            const u32 d = mouth_dist_sq(a, b, mx, my, has, cap);
            if (d > best_d) {
                best_d = d;
                best = b;
            }
        }
    }
    if (py > 0) {
        const u16 b = bov[ti - static_cast<u32>(w)];
        if (b != a && b != static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            const u32 d = mouth_dist_sq(a, b, mx, my, has, cap);
            if (d > best_d) {
                best_d = d;
                best = b;
            }
        }
    }
    if (py + 1u < h) {
        const u16 b = bov[ti + static_cast<u32>(w)];
        if (b != a && b != static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            const u32 d = mouth_dist_sq(a, b, mx, my, has, cap);
            if (d > best_d) {
                best_d = d;
                best = b;
            }
        }
    }
    return best;
}

static u32 count_border_tiles (
    u16 w,
    u16 h,
    const u16* bov,
    const u16* mx,
    const u16* my,
    const u8* has,
    u16 cap) 
{
    PTIME_START(__func__);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 cnt = 0;
    for (u32 ti = 0; ti < n; ++ti) {
        const u16 a = bov[ti];
        if (a == static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            continue;
        }
        if (pick_nb(a, w, h, ti, bov, mx, my, has, cap) != 0) {
            cnt++;
        }
    }
    PTIME_STOP(__func__);
    return cnt;
}

static void fill_border_recs (
    u16 w,
    u16 h,
    const u16* bov,
    const u16* mx,
    const u16* my,
    const u8* has,
    u16 cap,
    BorderRec* recs) 
{
    PTIME_START(__func__);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 ri = 0;
    for (u32 ti = 0; ti < n; ++ti) {
        const u16 a = bov[ti];
        if (a == static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            continue;
        }
        const u16 nb = pick_nb(a, w, h, ti, bov, mx, my, has, cap);
        if (nb == 0) {
            continue;
        }
        recs[ri].m_lo = basin_lo(a, nb);
        recs[ri].m_hi = basin_hi(a, nb);
        recs[ri].m_ti = ti;
        ri++;
    }
    PTIME_STOP(__func__);
}

static void swap_border_rec (BorderRec* a, BorderRec* b) {
    BorderRec t = *a;
    *a = *b;
    *b = t;
}

static bool border_rec_lt (const BorderRec& a, const BorderRec& b) {
    if (a.m_lo != b.m_lo) {
        return a.m_lo < b.m_lo;
    }
    if (a.m_hi != b.m_hi) {
        return a.m_hi < b.m_hi;
    }
    return a.m_ti < b.m_ti;
}

static void qsort_border_recs (BorderRec* recs, u32 lo, u32 hi) {
    if (lo >= hi) {
        return;
    }
    const BorderRec piv = recs[hi];
    u32 i = lo;
    for (u32 j = lo; j < hi; ++j) {
        if (border_rec_lt(recs[j], piv)) {
            swap_border_rec(&recs[i], &recs[j]);
            i++;
        }
    }
    swap_border_rec(&recs[i], &recs[hi]);
    if (i > 0u) {
        qsort_border_recs(recs, lo, i - 1u);
    }
    qsort_border_recs(recs, i + 1u, hi);
}

static void sort_border_recs (BorderRec* recs, u32 n) {
    PTIME_START(__func__);
    if (n <= 1u) {
        PTIME_STOP(__func__);
        return;
    }
    qsort_border_recs(recs, 0u, n - 1u);
    PTIME_STOP(__func__);
}

static void swap_seg (P1_WatershedBorderSeg* a, P1_WatershedBorderSeg* b) {
    P1_WatershedBorderSeg t = *a;
    *a = *b;
    *b = t;
}

static bool seg_sort_lt (const P1_WatershedBorderSeg& a, const P1_WatershedBorderSeg& b) {
    if (a.m_mouth_dist != b.m_mouth_dist) {
        return a.m_mouth_dist < b.m_mouth_dist;
    }
    if (a.m_basin_a != b.m_basin_a) {
        return a.m_basin_a < b.m_basin_a;
    }
    return a.m_basin_b < b.m_basin_b;
}

static void qsort_segs (P1_WatershedBorderSeg* segs, u32 lo, u32 hi) {
    if (lo >= hi) {
        return;
    }
    const P1_WatershedBorderSeg piv = segs[hi];
    u32 i = lo;
    for (u32 j = lo; j < hi; ++j) {
        if (seg_sort_lt(segs[j], piv)) {
            swap_seg(&segs[i], &segs[j]);
            i++;
        }
    }
    swap_seg(&segs[i], &segs[hi]);
    if (i > 0u) {
        qsort_segs(segs, lo, i - 1u);
    }
    qsort_segs(segs, i + 1u, hi);
}

static void sort_segs (P1_WatershedBorderSeg* segs, u16 n) {
    PTIME_START(__func__);
    if (n <= 1u) {
        PTIME_STOP(__func__);
        return;
    }
    qsort_segs(segs, 0u, static_cast<u32>(n) - 1u);
    PTIME_STOP(__func__);
}

static bool side_hills_ok (u16 pln, u16 hil, u8 perc) {
    const u32 tot = static_cast<u32>(pln) + static_cast<u32>(hil);
    if (tot == 0) {
        return false;
    }
    return static_cast<u32>(hil) * 100u >= tot * static_cast<u32>(perc);
}

static bool seg_hills_ok (const P1_WatershedBorderSeg& s, u8 perc) {
    return side_hills_ok(s.m_a_plains, s.m_a_hills, perc)
        || side_hills_ok(s.m_b_plains, s.m_b_hills, perc);
}

static u32 tile_cap_n (u32 border_tile_n, u8 perc) {
    if (border_tile_n == 0 || perc == 0) {
        return 0;
    }
    if (perc >= 100) {
        return border_tile_n;
    }
    return (border_tile_n * static_cast<u32>(perc) + 99u) / 100u;
}

static u32 seg_land_tile_n (
    const u8* terrain,
    u16 w,
    u16 h,
    const u16* ov,
    u16 oid) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 cnt = 0;
    for (u32 i = 0; i < n; ++i) {
        if (ov[i] != oid) {
            continue;
        }
        if (is_water(terrain[i])) {
            continue;
        }
        if (!is_plains(terrain[i]) && !is_hills(terrain[i])) {
            continue;
        }
        cnt++;
    }
    return cnt;
}

static u32 max_mouth_dist (const P1_WatershedBorderSeg* segs, u16 seg_n) {
    if (segs == nullptr || seg_n == 0) {
        return 0;
    }
    return segs[seg_n - 1].m_mouth_dist;
}

static bool seg_dist_too_short (u32 mouth_dist, u32 max_dist, u8 min_perc) {
    if (max_dist == 0 || min_perc == 0) {
        return false;
    }
    if (min_perc >= 100) {
        return false;
    }
    return mouth_dist * 100u < max_dist * static_cast<u32>(min_perc);
}

static bool build_watershed_mtns_core (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverNetworkRslt& network,
    const P1_Gen_RiverPtsRslt& pts,
    const u8* noise,
    const P1_Gen_WatershedMountainsPrm& sp,
    u32 seed,
    u16 cap,
    u16* ov,
    P1_WatershedBorderSeg* segs,
    P1_Gen_WatershedMountainsRslt* out,
    const MouthTbl& mt) 
{
    PTIME_START(__func__);
    const u16* bov = network.m_ov;
    build_mouth_tbl(network, pts, w, cap, mt.m_mx, mt.m_my, mt.m_has);
    const u32 border_n = count_border_tiles(w, h, bov, mt.m_mx, mt.m_my, mt.m_has, cap);
    if (border_n == 0) {
        PTIME_STOP(__func__);
        return true;
    }
    if (border_n > WhiteboardMng::tile_n()) {
        PTIME_STOP(__func__);
        return false;
    }
    Whiteboard_8B wb_recs("P1_Gen_WatershedMountains", "recs", seed);
    P1_WB_CHK(wb_recs);
    BorderRec* recs = reinterpret_cast<BorderRec*>(wb_recs.get_iter_ptr());
    if (recs == nullptr) {
        PTIME_STOP(__func__);
        return false;
    }
    fill_border_recs(w, h, bov, mt.m_mx, mt.m_my, mt.m_has, cap, recs);
    sort_border_recs(recs, border_n);
    u16 seg_n = 0;
    for (u32 ri = 0; ri < border_n; ++ri) {
        if (ri == 0
            || recs[ri].m_lo != recs[ri - 1].m_lo
            || recs[ri].m_hi != recs[ri - 1].m_hi) {
            seg_n++;
        }
    }
    if (segs == nullptr) {
        PTIME_STOP(__func__);
        return false;
    }
    u16 si = 0;
    u16 next_ov = 1;
    u32 ri = 0;
    while (ri < border_n) {
        const u16 lo = recs[ri].m_lo;
        const u16 hi = recs[ri].m_hi;
        P1_WatershedBorderSeg s = {};
        s.m_basin_a = lo;
        s.m_basin_b = hi;
        s.m_mouth_ax = mt.m_mx[lo];
        s.m_mouth_ay = mt.m_my[lo];
        s.m_mouth_bx = mt.m_mx[hi];
        s.m_mouth_by = mt.m_my[hi];
        s.m_mouth_dist = mouth_dist_sq(lo, hi, mt.m_mx, mt.m_my, mt.m_has, cap);
        s.m_ov_idx = next_ov++;
        s.m_tile_n = 0;
        s.m_a_plains = 0;
        s.m_a_hills = 0;
        s.m_b_plains = 0;
        s.m_b_hills = 0;
        while (ri < border_n && recs[ri].m_lo == lo && recs[ri].m_hi == hi) {
            const u32 ti = recs[ri].m_ti;
            s.m_tile_n++;
            ov[ti] = s.m_ov_idx;
            const u16 bid = bov[ti];
            const u8 t = terrain[ti];
            if (bid == lo) {
                count_side_terr(t, ti, noise, sp.m_noise_thresh, &s.m_a_plains, &s.m_a_hills);
            } else if (bid == hi) {
                count_side_terr(t, ti, noise, sp.m_noise_thresh, &s.m_b_plains, &s.m_b_hills);
            }
            ri++;
        }
        segs[si++] = s;
    }
    sort_segs(segs, seg_n);
    out->m_border_tile_n = border_n;
    out->m_seg_n = seg_n;
    out->m_pick_n = 0;
    out->m_pick_tile_n = 0;
    PTIME_STOP(__func__);
    return true;
}

static bool build_watershed_mtns (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverNetworkRslt& network,
    const P1_Gen_RiverPtsRslt& pts,
    const u8* noise,
    const P1_Gen_WatershedMountainsPrm& sp,
    u32 seed,
    u16* ov,
    P1_WatershedBorderSeg* segs,
    P1_Gen_WatershedMountainsRslt* out) 
{
    PTIME_START(__func__);
    if (terrain == nullptr || network.m_ov == nullptr || out == nullptr || ov == nullptr) {
        PTIME_STOP(__func__);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u16 cap = max_basin_idx_ov(w, h, network.m_ov);
    out->m_w = w;
    out->m_h = h;
    out->m_seg_n = 0;
    out->m_pick_n = 0;
    out->m_border_tile_n = 0;
    out->m_pick_tile_n = 0;
    out->m_segs = segs;
    out->m_ov = ov;
    for (u32 i = 0; i < n; ++i) {
        ov[i] = 0;
    }
    if (cap == 0) {
        PTIME_STOP(__func__);
        return true;
    }
    const u32 tbl_n = static_cast<u32>(cap) + 1u;
    u16 mx_stk[P1_WSHED_MOUTH_STK_N];
    u16 my_stk[P1_WSHED_MOUTH_STK_N];
    u8 has_stk[P1_WSHED_MOUTH_STK_N];
    if (tbl_n * 5u <= P1_WSHED_STK_LIM && tbl_n <= P1_WSHED_MOUTH_STK_N) {
        const MouthTbl mt = { mx_stk, my_stk, has_stk };
        PTIME_STOP(__func__);
        return build_watershed_mtns_core(
            terrain, w, h, network, pts, noise, sp, seed, cap, ov, segs, out, mt);
    }
    Whiteboard_2B wb_mx("P1_Gen_WatershedMountains", "mx", seed);
    Whiteboard_2B wb_my("P1_Gen_WatershedMountains", "my", seed);
    Whiteboard_1B wb_has("P1_Gen_WatershedMountains", "has", seed);
    P1_WB_CHK(wb_mx);
    P1_WB_CHK(wb_my);
    P1_WB_CHK(wb_has);
    const MouthTbl mt = { wb_mx.get_iter_ptr(), wb_my.get_iter_ptr(), wb_has.raw() };
    if (mt.m_mx == nullptr || mt.m_my == nullptr || mt.m_has == nullptr) {
        PTIME_STOP(__func__);
        return false;
    }
    PTIME_STOP(__func__);
    return build_watershed_mtns_core(
        terrain, w, h, network, pts, noise, sp, seed, cap, ov, segs, out, mt);
}

static bool save_rgb_ppm (cstr path, const u8* r, const u8* g, const u8* b, u16 wi, u16 hi) {
    if (path == nullptr || r == nullptr || g == nullptr || b == nullptr || wi == 0 || hi == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const u32 n = static_cast<u32>(wi) * static_cast<u32>(hi);
    u8 px[3];
    for (u32 i = 0; i < n; ++i) {
        px[0] = r[i];
        px[1] = g[i];
        px[2] = b[i];
        if (std::fwrite(px, 1, 3, fp) != 3) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 32;
    *g = 32;
    *b = 32;
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

//================================================================================================================================
//=> - P1_Gen_WatershedMountains -
//================================================================================================================================

P1_Gen_WatershedMountains::P1_Gen_WatershedMountains (
    const P1_RunPrm& prm,
    const P1_Gen_WatershedMountainsPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false),
    m_rslt(),
    m_wb_ov(nullptr),
    m_wb_segs(nullptr) {
    std::memset(&m_rslt, 0, sizeof(m_rslt));
    m_wb_ov = new Whiteboard_2B("P1_Gen_WatershedMountains", "ov", prm.m_seed);
    m_wb_segs = new Whiteboard_8B("P1_Gen_WatershedMountains", "segs", prm.m_seed);
    P1_WB_CHK(*m_wb_ov);
    P1_WB_CHK(*m_wb_segs);
}

P1_Gen_WatershedMountains::~P1_Gen_WatershedMountains () {
    clear_rslt();
    delete m_wb_segs;
    delete m_wb_ov;
    m_wb_segs = nullptr;
    m_wb_ov = nullptr;
}

void P1_Gen_WatershedMountains::clear_rslt () {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_seg_n = 0;
    m_rslt.m_pick_n = 0;
    m_rslt.m_border_tile_n = 0;
    m_rslt.m_pick_tile_n = 0;
    m_rslt.m_segs = nullptr;
    m_rslt.m_ov = nullptr;
    m_valid_generation = false;
}

bool P1_Gen_WatershedMountains::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverNetworkRslt& network,
    const P1_Gen_RiverPtsRslt& pts,
    const u8* noise) 
{
    PTIME_INIT();
    PTIME_START(__func__);
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h) || network.m_ov == nullptr) {
        PTIME_STOP(__func__);
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        PTIME_STOP(__func__);
        return false;
    }
    if (m_wb_ov == nullptr || m_wb_segs == nullptr || !m_wb_ov->ok() || !m_wb_segs->ok()) {
        PTIME_STOP(__func__);
        return false;
    }
    u16* ov = m_wb_ov->get_iter_ptr();
    P1_WatershedBorderSeg* segs = reinterpret_cast<P1_WatershedBorderSeg*>(m_wb_segs->get_iter_ptr());
    if (ov == nullptr || segs == nullptr) {
        PTIME_STOP(__func__);
        return false;
    }
    const bool ok = build_watershed_mtns(terrain, w, h, network, pts, noise, m_sp, m_prm.m_seed, ov, segs, &m_rslt);
    if (!ok) {
        clear_rslt();
        PTIME_STOP(__func__);
        return false;
    }
    PTIME_STOP(__func__);
    PTIME_PRINT();
    m_valid_generation = true;
    return true;
}

bool P1_Gen_WatershedMountains::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_WatershedMountainsRslt& P1_Gen_WatershedMountains::result () const {
    return m_rslt;
}

void P1_Gen_WatershedMountains::save_output (
    cstr path,
    const P1_Gen_RiverNetworkRslt& network,
    const u8* terrain) const 
{
    if (!m_valid_generation || path == nullptr || network.m_ov == nullptr || m_rslt.m_ov == nullptr) {
        return;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_1B wb_r("P1_Gen_WatershedMountains", "rgb_r", m_prm.m_seed);
    Whiteboard_1B wb_g("P1_Gen_WatershedMountains", "rgb_g", m_prm.m_seed);
    Whiteboard_1B wb_b("P1_Gen_WatershedMountains", "rgb_b", m_prm.m_seed);
    P1_WB_CHK(wb_r);
    P1_WB_CHK(wb_g);
    P1_WB_CHK(wb_b);
    u8* rr = wb_r.raw();
    u8* gg = wb_g.raw();
    u8* bb = wb_b.raw();
    if (rr == nullptr || gg == nullptr || bb == nullptr) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 32;
        u8 g = 32;
        u8 b = 32;
        if (terrain != nullptr) {
            terr_rgb(terrain[i], &r, &g, &b);
        }
        rr[i] = r;
        gg[i] = g;
        bb[i] = b;
    }
    u16 max_oid = 0;
    for (u32 i = 0; i < n; ++i) {
        if (m_rslt.m_ov[i] > max_oid) {
            max_oid = m_rslt.m_ov[i];
        }
    }
    if (max_oid == 0) {
        save_rgb_ppm(path, rr, gg, bb, w, h);
        return;
    }
    const u32 pal_n = static_cast<u32>(max_oid) + 1u;
    const u32 pal_b = pal_n * 3u;
    u8* pal = nullptr;
    u8 pal_stk[3072];
    Whiteboard_1B wb_pal("P1_Gen_WatershedMountains", "pal", m_prm.m_seed);
    if (pal_b <= sizeof(pal_stk)) {
        pal = pal_stk;
    } else {
        P1_WB_CHK(wb_pal);
        pal = wb_pal.raw();
    }
    u8 has_stk[1024];
    u8* pal_set = nullptr;
    Whiteboard_1B wb_pal_set("P1_Gen_WatershedMountains", "pal_set", m_prm.m_seed);
    if (pal_n <= sizeof(has_stk)) {
        pal_set = has_stk;
    } else {
        P1_WB_CHK(wb_pal_set);
        pal_set = wb_pal_set.raw();
    }
    if (pal == nullptr || pal_set == nullptr) {
        return;
    }
    for (u16 i = 0; i <= max_oid; ++i) {
        pal_set[i] = 0;
    }
    Rng32 rng;
    rng_seed(&rng, m_prm.m_seed ^ 0xA5A5A5A5u);
    for (u32 i = 0; i < n; ++i) {
        const u16 oid = m_rslt.m_ov[i];
        if (oid == 0) {
            continue;
        }
        if (pal_set[oid] == 0) {
            pal[oid * 3u + 0] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[oid * 3u + 1] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[oid * 3u + 2] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal_set[oid] = 1;
        }
        rr[i] = pal[oid * 3u + 0];
        gg[i] = pal[oid * 3u + 1];
        bb[i] = pal[oid * 3u + 2];
    }
    save_rgb_ppm(path, rr, gg, bb, w, h);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

