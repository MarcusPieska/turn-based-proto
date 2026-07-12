//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_coastal_mtn_limits.h"

#include "p1_wb_util.h"
#include "wb_que_xy.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - Design-time coastal mtn circuit tuning -
//================================================================================================================================

#define P1_COASTAL_MTN_CIRCUIT_MAX 40u
#define P1_COASTAL_MTN_CIRCUIT_TILE_MAX 2000u
#define P1_COASTAL_MTN_LIMIT_DEPTH 0u
#define P1_LIM_PEND 255u
#define P1_COASTAL_MTN_SEL_FRAC_MAX 25u
#define P1_COASTAL_MTN_LONG_DRAW_FRAC 60u
#define P1_COASTAL_MTN_LONG_SKIP 2u
#define P1_COASTAL_MTN_SHORT_PER_LONG 3u
#define P1_COASTAL_MTN_BORDER_MAX 256u

struct P1_CoastalMtnCircuit {
    u16 m_start_x;
    u16 m_start_y;
    u32 m_size;
    u8 m_ov_val;
};

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;
static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1}; 

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w) && static_cast<u32>(y) < static_cast<u32>(h);
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
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    if (!is_water(terrain[i]) || mask[i] != 0) {
        return;
    }
    mask[i] = 1;
    que.push(x, y);
}

static bool build_glob_ocn_mask (const u8* terrain, u16 w, u16 h, u16* mask, WB_QueXY& que) 
{
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

static bool tile_adj_glob_ocn (const u16* glob, u16 w, u16 h, u16 x, u16 y) {
    if (glob == nullptr) {
        return false;
    }
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (glob[j] != 0) {
            return true;
        }
    }
    return false;
}

static bool bfs_sec_coast_dist (
    const u16* glob,
    u16 w,
    u16 h,
    const u16* ov,
    u16 sector_n,
    const P1_RiverSectorNode* nodes,
    u16* sec_dist,
    u16* sec_que,
    u16* max_dist) 
{
    if (glob == nullptr || ov == nullptr || sec_dist == nullptr || sec_que == nullptr || max_dist == nullptr) {
        return false;
    }
    *max_dist = 0;
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        sec_dist[si] = k_inf;
    }
    u32 qh = 0;
    u32 qn = 0;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        if (sec_dist[sid] != k_inf) {
            continue;
        }
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (!tile_adj_glob_ocn(glob, w, h, x, y)) {
            continue;
        }
        sec_dist[sid] = 0;
        sec_que[qn++] = sid;
    }
    while (qh < qn) {
        const u16 sid = sec_que[qh++];
        const u16 cur = sec_dist[sid];
        if (nodes == nullptr || sid >= sector_n) {
            continue;
        }
        const P1_RiverSectorNode& nd = nodes[sid];
        const u16 nxt = static_cast<u16>(cur + 1u);
        for (u16 c = 0; c < nd.m_conn_n; ++c) {
            const u16 nb = nd.m_conn[c];
            if (nb >= sector_n || sec_dist[nb] != k_inf) {
                continue;
            }
            sec_dist[nb] = nxt;
            if (nxt > *max_dist) {
                *max_dist = nxt;
            }
            sec_que[qn++] = nb;
        }
    }
    return true;
}

static bool mark_depth_limit (
    u16 w,
    u16 h,
    const u16* ov,
    u16 sector_n,
    const u16* sec_dist,
    u8* lim,
    u32* lim_n) 
{
    if (ov == nullptr || sec_dist == nullptr || lim == nullptr || lim_n == nullptr) {
        return false;
    }
    const u16 dep_lo = static_cast<u16>(P1_COASTAL_MTN_LIMIT_DEPTH);
    const u16 dep_hi = static_cast<u16>(P1_COASTAL_MTN_LIMIT_DEPTH + 1u);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    *lim_n = 0;
    for (u32 i = 0; i < n; ++i) {
        lim[i] = 0;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            const u16 sa = ov[i];
            if (sa == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sa >= sector_n) {
                continue;
            }
            const u16 da = sec_dist[sa];
            if (da != dep_lo && da != dep_hi) {
                continue;
            }
            for (i32 k = 0; k < 4; ++k) {
                const i32 nx = static_cast<i32>(x) + k_dx4[k];
                const i32 ny = static_cast<i32>(y) + k_dy4[k];
                if (!in_map(w, h, nx, ny)) {
                    continue;
                }
                const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                const u16 sb = ov[j];
                if (sb == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sb >= sector_n || sb == sa) {
                    continue;
                }
                const u16 db = sec_dist[sb];
                if ((da == dep_lo && db == dep_hi) || (da == dep_hi && db == dep_lo)) {
                    if (lim[i] == 0) {
                        lim[i] = P1_LIM_PEND;
                        ++*lim_n;
                    }
                    if (lim[j] == 0) {
                        lim[j] = P1_LIM_PEND;
                        ++*lim_n;
                    }
                }
            }
        }
    }
    return true;
}

static void sort_circuits_by_size (P1_CoastalMtnCircuit* circ, u32 n) {
    for (u32 i = 1; i < n; ++i) {
        P1_CoastalMtnCircuit key = circ[i];
        u32 j = i;
        while (j > 0u && circ[j - 1u].m_size < key.m_size) {
            circ[j] = circ[j - 1u];
            --j;
        }
        circ[j] = key;
    }
}

static void record_circuit (
    P1_CoastalMtnCircuit* circ,
    u32* circ_n,
    u16 start_x,
    u16 start_y,
    u32 sz,
    u8 ov_val) 
{
    if (sz == 0 || *circ_n >= static_cast<u32>(P1_COASTAL_MTN_CIRCUIT_MAX)) {
        return;
    }
    P1_CoastalMtnCircuit* c = &circ[*circ_n];
    c->m_start_x = start_x;
    c->m_start_y = start_y;
    c->m_size = sz;
    c->m_ov_val = ov_val;
    ++*circ_n;
}

static bool flood_label_lim_circuits (
    u16 w,
    u16 h,
    u8* lim,
    P1_CoastalMtnCircuit* circ,
    u32* circ_n,
    u32* circ_found,
    WB_QueXY& que) 
{
    if (lim == nullptr || circ == nullptr || circ_n == nullptr || circ_found == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    *circ_n = 0;
    *circ_found = 0;
    for (u32 i = 0; i < n; ++i) {
        if (lim[i] != P1_LIM_PEND) {
            continue;
        }
        u32 sz = 0;
        u16 start_x = 0;
        u16 start_y = 0;
        u8 ov_val = 0;
        que.clear();
        que.push(static_cast<u16>(i % static_cast<u32>(w)), static_cast<u16>(i / static_cast<u32>(w)));
        while (que.count() > 0u) {
            const u16 cx = que.x_at(0);
            const u16 cy = que.y_at(0);
            que.drop(1);
            const u32 cur = static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx);
            if (lim[cur] != P1_LIM_PEND) {
                continue;
            }
            if (sz == 0) {
                ++*circ_found;
                ov_val = static_cast<u8>((*circ_found < 255u) ? *circ_found : 254u);
                start_x = cx;
                start_y = cy;
            }
            lim[cur] = ov_val;
            ++sz;
            for (i32 k = 0; k < 4; ++k) {
                const i32 nx = static_cast<i32>(cx) + k_dx4[k];
                const i32 ny = static_cast<i32>(cy) + k_dy4[k];
                if (!in_map(w, h, nx, ny)) {
                    continue;
                }
                const u32 nk = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                if (lim[nk] == P1_LIM_PEND) {
                    que.push(static_cast<u16>(nx), static_cast<u16>(ny));
                }
            }
            if (sz >= static_cast<u32>(P1_COASTAL_MTN_CIRCUIT_TILE_MAX)) {
                record_circuit(circ, circ_n, start_x, start_y, sz, ov_val);
                sz = 0;
            }
        }
        if (sz > 0) {
            record_circuit(circ, circ_n, start_x, start_y, sz, ov_val);
        }
    }
    if (*circ_n > 1u) {
        sort_circuits_by_size(circ, *circ_n);
    }
    return true;
}

static bool is_depth_pair (u16 da, u16 db) {
    const u16 dep_lo = static_cast<u16>(P1_COASTAL_MTN_LIMIT_DEPTH);
    const u16 dep_hi = static_cast<u16>(P1_COASTAL_MTN_LIMIT_DEPTH + 1u);
    return (da == dep_lo && db == dep_hi) || (da == dep_hi && db == dep_lo);
}

static bool find_tile_sec_pair (
    u16 w,
    u16 h,
    const u16* ov,
    const u16* sec_dist,
    u32 i,
    u16* pa,
    u16* pb) 
{
    const u16 sa = ov[i];
    if (sa == static_cast<u16>(P1_RIVER_SECTOR_NONE)) {
        return false;
    }
    const u16 cx = static_cast<u16>(i % static_cast<u32>(w));
    const u16 cy = static_cast<u16>(i / static_cast<u32>(w));
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(cx) + k_dx4[k];
        const i32 ny = static_cast<i32>(cy) + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        const u16 sb = ov[j];
        if (sb == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sb == sa) {
            continue;
        }
        if (is_depth_pair(sec_dist[sa], sec_dist[sb])) {
            if (sa < sb) {
                *pa = sa;
                *pb = sb;
            } else {
                *pa = sb;
                *pb = sa;
            }
            return true;
        }
    }
    return false;
}

static bool tile_on_sec_pair (
    u16 w,
    u16 h,
    const u16* ov,
    const u16* sec_dist,
    const u8* lim,
    u8 ov_val,
    u32 i,
    u16 pa,
    u16 pb) 
{
    if (lim[i] != ov_val) {
        return false;
    }
    const u16 sa = ov[i];
    if (sa != pa && sa != pb) {
        return false;
    }
    const u16 cx = static_cast<u16>(i % static_cast<u32>(w));
    const u16 cy = static_cast<u16>(i / static_cast<u32>(w));
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(cx) + k_dx4[k];
        const i32 ny = static_cast<i32>(cy) + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        const u16 sb = ov[j];
        if (lim[j] != ov_val) {
            continue;
        }
        if ((sa == pa && sb == pb) || (sa == pb && sb == pa)) {
            if (is_depth_pair(sec_dist[sa], sec_dist[sb])) {
                return true;
            }
        }
    }
    return false;
}

struct P1_CircBorder {
    u16 m_sa;
    u16 m_sb;
    u32 m_min_bfs;
    u32 m_tile_n;
    u32* m_tiles;
};

struct P1_CircBorderSet {
    u32 m_n;
    P1_CircBorder m_br[P1_COASTAL_MTN_BORDER_MAX];
};

static void free_border_set (P1_CircBorderSet* bs) {
    if (bs == nullptr) {
        return;
    }
    for (u32 i = 0; i < bs->m_n; ++i) {
        delete[] bs->m_br[i].m_tiles;
        bs->m_br[i].m_tiles = nullptr;
        bs->m_br[i].m_tile_n = 0;
    }
    bs->m_n = 0;
}

static void bfs_circ_tile_dist (
    u16 w,
    u16 h,
    const u8* lim,
    u8 ov_val,
    u16 sx,
    u16 sy,
    u16* dist,
    WB_QueXY& que) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_inf;
    }
    const u32 start = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
    if (lim[start] != ov_val) {
        return;
    }
    que.clear();
    dist[start] = 0;
    que.push(sx, sy);
    while (que.count() > 0u) {
        const u16 cx = que.x_at(0);
        const u16 cy = que.y_at(0);
        que.drop(1);
        const u32 cur = static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx);
        const u16 nd = static_cast<u16>(dist[cur] + 1u);
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(cx) + k_dx4[k];
            const i32 ny = static_cast<i32>(cy) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 nk = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (lim[nk] != ov_val || dist[nk] != k_inf) {
                continue;
            }
            dist[nk] = nd;
            que.push(static_cast<u16>(nx), static_cast<u16>(ny));
        }
    }
}

static bool build_circ_borders (
    u16 w,
    u16 h,
    const u16* ov,
    const u16* sec_dist,
    const u8* lim,
    const P1_CoastalMtnCircuit& circ,
    const u16* tile_dist,
    u16* vis,
    WB_QueXY& que,
    P1_CircBorderSet* out) 
{
    if (ov == nullptr || sec_dist == nullptr || lim == nullptr || tile_dist == nullptr || vis == nullptr || out == nullptr) {
        return false;
    }
    out->m_n = 0;
    const u8 ov_val = circ.m_ov_val;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        vis[i] = 0;
    }
    for (u32 i = 0; i < n; ++i) {
        if (lim[i] != ov_val || vis[i] != 0) {
            continue;
        }
        u16 pa = 0;
        u16 pb = 0;
        if (!find_tile_sec_pair(w, h, ov, sec_dist, i, &pa, &pb)) {
            continue;
        }
        if (out->m_n >= static_cast<u32>(P1_COASTAL_MTN_BORDER_MAX)) {
            continue;
        }
        P1_CircBorder* br = &out->m_br[out->m_n];
        br->m_sa = pa;
        br->m_sb = pb;
        br->m_min_bfs = k_inf;
        br->m_tile_n = 0;
        br->m_tiles = new u32[circ.m_size];
        if (br->m_tiles == nullptr) {
            continue;
        }
        que.clear();
        que.push(static_cast<u16>(i % static_cast<u32>(w)), static_cast<u16>(i / static_cast<u32>(w)));
        vis[i] = 1;
        while (que.count() > 0u) {
            const u16 cx = que.x_at(0);
            const u16 cy = que.y_at(0);
            que.drop(1);
            const u32 cur = static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx);
            br->m_tiles[br->m_tile_n++] = cur;
            if (tile_dist[cur] < br->m_min_bfs) {
                br->m_min_bfs = tile_dist[cur];
            }
            for (i32 k = 0; k < 4; ++k) {
                const i32 nx = static_cast<i32>(cx) + k_dx4[k];
                const i32 ny = static_cast<i32>(cy) + k_dy4[k];
                if (!in_map(w, h, nx, ny)) {
                    continue;
                }
                const u32 nk = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                if (vis[nk] != 0 || !tile_on_sec_pair(w, h, ov, sec_dist, lim, ov_val, nk, pa, pb)) {
                    continue;
                }
                vis[nk] = 1;
                que.push(static_cast<u16>(nx), static_cast<u16>(ny));
            }
        }
        ++out->m_n;
    }
    for (u32 i = 1; i < out->m_n; ++i) {
        P1_CircBorder key = out->m_br[i];
        u32 j = i;
        while (j > 0u && out->m_br[j - 1u].m_min_bfs > key.m_min_bfs) {
            out->m_br[j] = out->m_br[j - 1u];
            --j;
        }
        out->m_br[j] = key;
    }
    return true;
}

static u32 mark_sel_tile (u8* sel, u32 i) {
    if (sel[i] != 0) {
        return 0;
    }
    sel[i] = 1;
    return 1;
}

static u32 draw_border_tiles (u8* sel, const P1_CircBorder* br) {
    u32 added = 0;
    if (br == nullptr || br->m_tiles == nullptr) {
        return 0;
    }
    for (u32 i = 0; i < br->m_tile_n; ++i) {
        added += mark_sel_tile(sel, br->m_tiles[i]);
    }
    return added;
}

static u32 draw_short_circuit (
    u32 seed,
    u16 w,
    u16 h,
    const u16* ov,
    const u16* sec_dist,
    const u8* lim,
    const P1_CoastalMtnCircuit& circ,
    u16* dist,
    u16* vis,
    WB_QueXY& que,
    u8* sel) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    bfs_circ_tile_dist(w, h, lim, circ.m_ov_val, circ.m_start_x, circ.m_start_y, dist, que);
    P1_CircBorderSet bs;
    bs.m_n = 0;
    if (!build_circ_borders(w, h, ov, sec_dist, lim, circ, dist, vis, que, &bs)) {
        return 0;
    }
    u32 added = 0;
    if (bs.m_n == 0) {
        for (u32 i = 0; i < n; ++i) {
            if (lim[i] == circ.m_ov_val) {
                added += mark_sel_tile(sel, i);
            }
        }
        free_border_set(&bs);
        return added;
    }
    u32 omit = (bs.m_n > 1u) ? ((seed + static_cast<u32>(circ.m_ov_val)) % bs.m_n) : 0u;
    for (u32 bi = 0; bi < bs.m_n; ++bi) {
        if (bi == omit) {
            continue;
        }
        added += draw_border_tiles(sel, &bs.m_br[bi]);
    }
    free_border_set(&bs);
    return added;
}

static u32 draw_long_circuit (
    u16 w,
    u16 h,
    const u16* ov,
    const u16* sec_dist,
    const u8* lim,
    const P1_CoastalMtnCircuit& circ,
    u16* dist,
    u16* vis,
    WB_QueXY& que,
    u8* sel) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    bfs_circ_tile_dist(w, h, lim, circ.m_ov_val, circ.m_start_x, circ.m_start_y, dist, que);
    P1_CircBorderSet bs;
    bs.m_n = 0;
    if (!build_circ_borders(w, h, ov, sec_dist, lim, circ, dist, vis, que, &bs)) {
        return 0;
    }
    u32 added = 0;
    if (bs.m_n == 0) {
        const u32 tgt = (circ.m_size * static_cast<u32>(P1_COASTAL_MTN_LONG_DRAW_FRAC) + 99u) / 100u;
        for (u32 i = 0; i < n && added < tgt; ++i) {
            if (lim[i] == circ.m_ov_val) {
                added += mark_sel_tile(sel, i);
            }
        }
        free_border_set(&bs);
        return added;
    }
    const u32 tgt = (circ.m_size * static_cast<u32>(P1_COASTAL_MTN_LONG_DRAW_FRAC) + 99u) / 100u;
    u32 drawn = 0;
    const u32 last_i = bs.m_n - 1u;
    drawn += draw_border_tiles(sel, &bs.m_br[0]);
    if (last_i != 0) {
        for (u32 ti = 0; ti < bs.m_br[last_i].m_tile_n; ++ti) {
            const u32 t = bs.m_br[last_i].m_tiles[ti];
            if (sel[t] == 0) {
                drawn += mark_sel_tile(sel, t);
            }
        }
    }
    added = drawn;
    if (added >= tgt || bs.m_n <= 2u) {
        free_border_set(&bs);
        return added;
    }
    for (u32 bi = 1; bi < last_i; ++bi) {
        for (u32 ti = 0; ti < bs.m_br[bi].m_tile_n; ++ti) {
            if (added >= tgt) {
                break;
            }
            added += mark_sel_tile(sel, bs.m_br[bi].m_tiles[ti]);
        }
        if (added >= tgt) {
            break;
        }
    }
    free_border_set(&bs);
    return added;
}

static u32 count_sel_tiles (const u8* sel, u32 n) {
    u32 c = 0;
    for (u32 i = 0; i < n; ++i) {
        if (sel[i] != 0) {
            ++c;
        }
    }
    return c;
}

static i32 find_longest_unused (const P1_CoastalMtnCircuit* circ, u32 circ_n, const bool* used) {
    i32 best = -1;
    u32 best_sz = 0;
    for (u32 i = 0; i < circ_n; ++i) {
        if (used[i]) {
            continue;
        }
        if (best < 0 || circ[i].m_size >= best_sz) {
            best_sz = circ[i].m_size;
            best = static_cast<i32>(i);
        }
    }
    return best;
}

static i32 find_shortest_unused (const P1_CoastalMtnCircuit* circ, u32 circ_n, const bool* used) {
    i32 best = -1;
    u32 best_sz = 0xFFFFFFFFu;
    for (u32 i = 0; i < circ_n; ++i) {
        if (used[i]) {
            continue;
        }
        if (best < 0 || circ[i].m_size <= best_sz) {
            best_sz = circ[i].m_size;
            best = static_cast<i32>(i);
        }
    }
    return best;
}

static bool select_draw_circuits (
    u32 seed,
    u16 w,
    u16 h,
    const u16* ov,
    u16 sector_n,
    const u16* sec_dist,
    const u8* lim,
    const P1_CoastalMtnCircuit* circ,
    u32 circ_n,
    u32 limit_n,
    u16* dist,
    u16* vis,
    WB_QueXY& que,
    u8* sel,
    u32* sel_n) 
{
    if (ov == nullptr || sec_dist == nullptr || lim == nullptr || circ == nullptr
        || dist == nullptr || vis == nullptr || sel == nullptr || sel_n == nullptr || limit_n == 0) {
        return false;
    }
    (void)sector_n;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(sel, 0, n);
    *sel_n = 0;
    if (circ_n == 0) {
        return true;
    }
    bool used[P1_COASTAL_MTN_CIRCUIT_MAX];
    for (u32 i = 0; i < static_cast<u32>(P1_COASTAL_MTN_CIRCUIT_MAX); ++i) {
        used[i] = false;
    }
    while (*sel_n * 100u <= limit_n * static_cast<u32>(P1_COASTAL_MTN_SEL_FRAC_MAX)) {
        const i32 long_i = find_longest_unused(circ, circ_n, used);
        if (long_i < 0) {
            break;
        }
        draw_long_circuit(w, h, ov, sec_dist, lim, circ[static_cast<u32>(long_i)], dist, vis, que, sel);
        used[static_cast<u32>(long_i)] = true;
        *sel_n = count_sel_tiles(sel, n);
        for (u32 sk = 0; sk < static_cast<u32>(P1_COASTAL_MTN_LONG_SKIP); ++sk) {
            const i32 skip_i = find_longest_unused(circ, circ_n, used);
            if (skip_i < 0) {
                break;
            }
            used[static_cast<u32>(skip_i)] = true;
        }
        for (u32 si = 0; si < static_cast<u32>(P1_COASTAL_MTN_SHORT_PER_LONG); ++si) {
            const i32 short_i = find_shortest_unused(circ, circ_n, used);
            if (short_i < 0) {
                break;
            }
            draw_short_circuit(seed, w, h, ov, sec_dist, lim, circ[static_cast<u32>(short_i)], dist, vis, que, sel);
            used[static_cast<u32>(short_i)] = true;
            *sel_n = count_sel_tiles(sel, n);
        }
    }
    return true;
}

static bool build_final_limit_ov (
    u16 w,
    u16 h,
    const u16* ov,
    u16 sector_n,
    const u16* sec_dist,
    const P1_RiverSectorNode* nodes,
    const u8* sel,
    u16* sec_blk,
    u16* sec_z,
    u8* out) 
{
    if (ov == nullptr || sec_dist == nullptr || sel == nullptr || sec_blk == nullptr || out == nullptr) {
        return false;
    }
    const u16 dep_lo = static_cast<u16>(P1_COASTAL_MTN_LIMIT_DEPTH);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(out, 0, n);
    for (u32 i = 0; i < n; ++i) {
        if (sel[i] != 0) {
            out[i] = static_cast<u8>(P1_COASTAL_MTN_OV_SEL);
        }
    }
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        sec_blk[si] = 0;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (sel[i] == 0) {
                continue;
            }
            const u16 sa = ov[i];
            if (sa == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sa >= sector_n) {
                continue;
            }
            if (sec_dist[sa] == dep_lo) {
                sec_blk[sa] = 1;
            }
            for (i32 k = 0; k < 4; ++k) {
                const i32 nx = static_cast<i32>(x) + k_dx4[k];
                const i32 ny = static_cast<i32>(y) + k_dy4[k];
                if (!in_map(w, h, nx, ny)) {
                    continue;
                }
                const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                const u16 sb = ov[j];
                if (sb == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sb >= sector_n) {
                    continue;
                }
                if (sec_dist[sb] == dep_lo && is_depth_pair(sec_dist[sa], sec_dist[sb])) {
                    sec_blk[sb] = 1;
                }
            }
        }
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        if (sec_blk[sid] == 0) {
            continue;
        }
        if (out[i] != static_cast<u8>(P1_COASTAL_MTN_OV_SEL)) {
            out[i] = static_cast<u8>(P1_COASTAL_MTN_OV_BLK);
        }
    }
    if (dep_lo > 0u && nodes != nullptr && sec_z != nullptr) {
        for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
            sec_z[si] = 0;
        }
        for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
            if (sec_blk[si] == 0 || si >= sector_n) {
                continue;
            }
            const P1_RiverSectorNode& nd = nodes[si];
            for (u16 c = 0; c < nd.m_conn_n; ++c) {
                const u16 nb = nd.m_conn[c];
                if (nb >= sector_n) {
                    continue;
                }
                if (sec_dist[nb] == 0) {
                    sec_z[nb] = 1;
                }
            }
        }
        for (u32 i = 0; i < n; ++i) {
            const u16 sid = ov[i];
            if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
                continue;
            }
            if (sec_z[sid] == 0) {
                continue;
            }
            if (out[i] != static_cast<u8>(P1_COASTAL_MTN_OV_SEL)) {
                out[i] = static_cast<u8>(P1_COASTAL_MTN_OV_BLK);
            }
        }
    }
    return true;
}

static bool build_coastal_mtn_limits (
    u32 seed,
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverSectorsRslt& sectors,
    P1_Gen_CoastalMtnLimitsRslt* out) 
{
    if (out == nullptr || terrain == nullptr || sectors.m_ov == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    const u16 sector_n = sectors.m_sector_n;
    if (sector_n == 0) {
        return false;
    }
    const u32 tn = static_cast<u32>(w) * static_cast<u32>(h);
    u16* sec_dist = new u16[static_cast<u32>(sector_n)];
    u16* sec_que = new u16[static_cast<u32>(sector_n)];
    if (sec_dist == nullptr || sec_que == nullptr) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    Whiteboard_2B wb_glob("P1_Gen_CoastalMtnLimits", "glob", seed);
    P1_WB_CHK(wb_glob);
    Whiteboard_2B wb_dist("P1_Gen_CoastalMtnLimits", "dist", seed);
    P1_WB_CHK(wb_dist);
    Whiteboard_2B wb_vis("P1_Gen_CoastalMtnLimits", "vis", seed);
    P1_WB_CHK(wb_vis);
    WB_QueXY que;
    if (!que.ok()) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    u16* glob = wb_glob.get_iter_ptr();
    if (!build_glob_ocn_mask(terrain, w, h, glob, que)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    u16 max_dist = 0;
    if (!bfs_sec_coast_dist(glob, w, h, sectors.m_ov, sector_n, sectors.m_nodes, sec_dist, sec_que, &max_dist)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    MapArrayOverlay lim_work;
    if (!lim_work.resize(w, h)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    u8* lim = lim_work.data_w();
    u32 lim_n = 0;
    if (!mark_depth_limit(w, h, sectors.m_ov, sector_n, sec_dist, lim, &lim_n)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    P1_CoastalMtnCircuit circ[P1_COASTAL_MTN_CIRCUIT_MAX];
    u32 circ_n = 0;
    u32 circ_found = 0;
    if (!flood_label_lim_circuits(w, h, lim, circ, &circ_n, &circ_found, que)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    (void)circ_found;
    MapArrayOverlay sel_work;
    if (!sel_work.resize(w, h)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    u8* sel = sel_work.data_w();
    u32 sel_n = 0;
    if (!select_draw_circuits(seed, w, h, sectors.m_ov, sector_n, sec_dist, lim, circ, circ_n, lim_n,
            wb_dist.get_iter_ptr(), wb_vis.get_iter_ptr(), que, sel, &sel_n)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    if (!out->m_limit_ov.resize(w, h)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    u8* out_ov = out->m_limit_ov.data_w();
    u16* sec_blk = new u16[static_cast<u32>(sector_n)];
    if (sec_blk == nullptr) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    u16* sec_z = nullptr;
    const u16 dep_lo = static_cast<u16>(P1_COASTAL_MTN_LIMIT_DEPTH);
    if (dep_lo > 0u && sectors.m_nodes != nullptr) {
        sec_z = new u16[static_cast<u32>(sector_n)];
        if (sec_z == nullptr) {
            delete[] sec_blk;
            delete[] sec_dist;
            delete[] sec_que;
            return false;
        }
    }
    if (!build_final_limit_ov(w, h, sectors.m_ov, sector_n, sec_dist, sectors.m_nodes, sel,
            sec_blk, sec_z, out_ov)) {
        delete[] sec_z;
        delete[] sec_blk;
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    delete[] sec_z;
    delete[] sec_blk;
    delete[] sec_dist;
    delete[] sec_que;
    sel_n = 0;
    for (u32 i = 0; i < tn; ++i) {
        if (out_ov[i] == static_cast<u8>(P1_COASTAL_MTN_OV_SEL)) {
            ++sel_n;
        }
    }
    out->m_w = w;
    out->m_h = h;
    out->m_sector_n = sector_n;
    out->m_max_sec_dist = max_dist;
    out->m_limit_n = lim_n;
    out->m_sel_n = sel_n;
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

//================================================================================================================================
//=> - P1_Gen_CoastalMtnLimits -
//================================================================================================================================

P1_Gen_CoastalMtnLimits::P1_Gen_CoastalMtnLimits (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() 
{
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_sector_n = 0;
    m_rslt.m_max_sec_dist = 0;
    m_rslt.m_limit_n = 0;
    m_rslt.m_sel_n = 0;
}

bool P1_Gen_CoastalMtnLimits::generate (const u8* terrain, u16 w, u16 h, const P1_Gen_RiverSectorsRslt& sectors) {
    m_valid_generation = false;
    m_rslt.m_limit_ov.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_sector_n = 0;
    m_rslt.m_max_sec_dist = 0;
    m_rslt.m_limit_n = 0;
    m_rslt.m_sel_n = 0;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h || w != sectors.m_w || h != sectors.m_h) {
        return false;
    }
    if (!build_coastal_mtn_limits(m_prm.m_seed, terrain, w, h, sectors, &m_rslt)) {
        m_rslt.m_limit_ov.clear();
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_CoastalMtnLimits::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_CoastalMtnLimitsRslt& P1_Gen_CoastalMtnLimits::result () const {
    return m_rslt;
}

void P1_Gen_CoastalMtnLimits::save_output (cstr path, const u8* terrain) const {
    if (!m_valid_generation || path == nullptr || terrain == nullptr) {
        return;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u8* out_ov = m_rslt.m_limit_ov.data();
    if (w == 0 || h == 0 || out_ov == nullptr) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 40;
        u8 g = 40;
        u8 b = 40;
        const u8 cls = terrain[i];
        if (cls == TERR_OCEAN[0]) {
            r = TERR_OCEAN[1]; g = TERR_OCEAN[2]; b = TERR_OCEAN[3];
        } else if (cls == TERR_SEA[0]) {
            r = TERR_SEA[1]; g = TERR_SEA[2]; b = TERR_SEA[3];
        } else if (cls == TERR_COASTAL[0]) {
            r = TERR_COASTAL[1]; g = TERR_COASTAL[2]; b = TERR_COASTAL[3];
        } else if (cls == TERR_PLAINS[0]) {
            r = TERR_PLAINS[1]; g = TERR_PLAINS[2]; b = TERR_PLAINS[3];
        } else if (cls == TERR_HILLS[0]) {
            r = TERR_HILLS[1]; g = TERR_HILLS[2]; b = TERR_HILLS[3];
        } else if (cls == TERR_MOUNTAINS[0]) {
            r = TERR_MOUNTAINS[1]; g = TERR_MOUNTAINS[2]; b = TERR_MOUNTAINS[3];
        }
        const u8 v = out_ov[i];
        if (v == static_cast<u8>(P1_COASTAL_MTN_OV_BLK)) {
            r = 160;
            g = 80;
            b = 40;
        } else if (v == static_cast<u8>(P1_COASTAL_MTN_OV_SEL)) {
            r = 255;
            g = 255;
            b = 80;
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
