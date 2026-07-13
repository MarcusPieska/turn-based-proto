//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "p1_gen_watershed_mountains.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

struct BorderRec {
    u16 m_lo;
    u16 m_hi;
    u32 m_ti;
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
    for (u16 i = 0; i <= cap; ++i) {
        has[i] = 0;
    }
    if (network.m_downstream == nullptr || network.m_ov == nullptr || !pts.m_que.ok() || network.m_sector_n == 0) {
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

static void sort_border_recs (BorderRec* recs, u32 n) {
    for (u32 i = 1; i < n; ++i) {
        u32 j = i;
        while (j > 0 && border_rec_lt(recs[j], recs[j - 1])) {
            swap_border_rec(&recs[j], &recs[j - 1]);
            --j;
        }
    }
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

static void sort_segs (P1_WatershedBorderSeg* segs, u16 n) {
    for (u16 i = 1; i < n; ++i) {
        u16 j = i;
        while (j > 0 && seg_sort_lt(segs[j], segs[j - 1])) {
            swap_seg(&segs[j], &segs[j - 1]);
            --j;
        }
    }
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

static bool build_watershed_mtns (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverNetworkRslt& network,
    const P1_Gen_RiverPtsRslt& pts,
    const u8* noise,
    const P1_Gen_WatershedMountainsPrm& sp,
    P1_Gen_WatershedMountainsRslt* out) 
{
    if (terrain == nullptr || network.m_ov == nullptr || out == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u16* bov = network.m_ov;
    const u16 cap = max_basin_idx_ov(w, h, bov);
    out->m_w = w;
    out->m_h = h;
    out->m_seg_n = 0;
    out->m_pick_n = 0;
    out->m_border_tile_n = 0;
    out->m_pick_tile_n = 0;
    out->m_segs = nullptr;
    out->m_ov = new u16[n];
    if (out->m_ov == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        out->m_ov[i] = 0;
    }
    if (cap == 0) {
        return true;
    }
    u16* mx = new u16[static_cast<size_t>(cap) + 1u];
    u16* my = new u16[static_cast<size_t>(cap) + 1u];
    u8* has = new u8[static_cast<size_t>(cap) + 1u];
    if (mx == nullptr || my == nullptr || has == nullptr) {
        delete[] has;
        delete[] my;
        delete[] mx;
        return false;
    }
    build_mouth_tbl(network, pts, w, cap, mx, my, has);
    const u32 border_n = count_border_tiles(w, h, bov, mx, my, has, cap);
    if (border_n == 0) {
        delete[] has;
        delete[] my;
        delete[] mx;
        return true;
    }
    BorderRec* recs = new BorderRec[border_n];
    if (recs == nullptr) {
        delete[] has;
        delete[] my;
        delete[] mx;
        return false;
    }
    fill_border_recs(w, h, bov, mx, my, has, cap, recs);
    sort_border_recs(recs, border_n);
    u16 seg_n = 0;
    for (u32 ri = 0; ri < border_n; ++ri) {
        if (ri == 0
            || recs[ri].m_lo != recs[ri - 1].m_lo
            || recs[ri].m_hi != recs[ri - 1].m_hi) {
            seg_n++;
        }
    }
    P1_WatershedBorderSeg* segs = new P1_WatershedBorderSeg[seg_n];
    u16* all_ov = new u16[n];
    if (segs == nullptr || all_ov == nullptr) {
        delete[] all_ov;
        delete[] segs;
        delete[] recs;
        delete[] has;
        delete[] my;
        delete[] mx;
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        all_ov[i] = 0;
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
        s.m_mouth_ax = mx[lo];
        s.m_mouth_ay = my[lo];
        s.m_mouth_bx = mx[hi];
        s.m_mouth_by = my[hi];
        s.m_mouth_dist = mouth_dist_sq(lo, hi, mx, my, has, cap);
        s.m_ov_idx = next_ov++;
        s.m_tile_n = 0;
        s.m_a_plains = 0;
        s.m_a_hills = 0;
        s.m_b_plains = 0;
        s.m_b_hills = 0;
        while (ri < border_n && recs[ri].m_lo == lo && recs[ri].m_hi == hi) {
            const u32 ti = recs[ri].m_ti;
            s.m_tile_n++;
            all_ov[ti] = s.m_ov_idx;
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
    out->m_segs = segs;
    for (u32 i = 0; i < n; ++i) {
        out->m_ov[i] = all_ov[i];
    }
    delete[] all_ov;
    delete[] recs;
    delete[] has;
    delete[] my;
    delete[] mx;
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
    m_rslt() {
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

P1_Gen_WatershedMountains::~P1_Gen_WatershedMountains () {
    clear_rslt();
}

void P1_Gen_WatershedMountains::clear_rslt () {
    delete[] m_rslt.m_segs;
    delete[] m_rslt.m_ov;
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

bool P1_Gen_WatershedMountains::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverNetworkRslt& network,
    const P1_Gen_RiverPtsRslt& pts,
    const u8* noise) 
{
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h) || network.m_ov == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!build_watershed_mtns(terrain, w, h, network, pts, noise, m_sp, &m_rslt)) {
        clear_rslt();
        return false;
    }
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
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 32;
        u8 g = 32;
        u8 b = 32;
        if (terrain != nullptr) {
            terr_rgb(terrain[i], &r, &g, &b);
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const u16 pal_cap = static_cast<u16>(max_basin_idx_ov(w, h, network.m_ov) + 1u);
    u8* pal = new u8[static_cast<size_t>(pal_cap) * 3u];
    bool* pal_set = new bool[pal_cap];
    if (pal == nullptr || pal_set == nullptr) {
        delete[] pal_set;
        delete[] pal;
        delete[] rgb;
        return;
    }
    for (u16 i = 0; i < pal_cap; ++i) {
        pal_set[i] = false;
    }
    Rng32 rng;
    rng_seed(&rng, m_prm.m_seed ^ 0xA5A5A5A5u);
    for (u32 i = 0; i < n; ++i) {
        const u16 bid = network.m_ov[i];
        if (bid == static_cast<u16>(P1_RIVER_BASIN_NONE) || bid >= pal_cap) {
            continue;
        }
        if (!pal_set[bid]) {
            pal[bid * 3u + 0] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[bid * 3u + 1] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[bid * 3u + 2] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal_set[bid] = true;
        }
        const u32 p = i * 3u;
        rgb[p + 0] = pal[bid * 3u + 0];
        rgb[p + 1] = pal[bid * 3u + 1];
        rgb[p + 2] = pal[bid * 3u + 2];
    }
    save_rgb_ppm(path, rgb, w, h);
    delete[] pal_set;
    delete[] pal;
    delete[] rgb;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
