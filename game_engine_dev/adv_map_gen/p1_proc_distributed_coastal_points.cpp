//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_proc_distributed_coastal_points.h"

#include "game_map_defs.h"
#include "land_mass_index.h"
#include "p1_wb_util.h"
#include "profile_time.h"

#include <cstring>

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};
static const u32 k_max_mass = 64u;
static const u32 k_min_mass_pct = 10u;

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_land (u8 cls) {
    return cls != TERR_NONE[0] && !overlay_is_water_terr(cls);
}

static bool is_shore_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool adj_shore (const u8* ter, u16 w, u16 h, u16 x, u16 y) {
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        if (is_shore_wat(ter[tidx(w, static_cast<u16>(nx), static_cast<u16>(ny))])) {
            return true;
        }
    }
    return false;
}

static bool is_coast_land (const u8* ter, u16 w, u16 h, u16 x, u16 y) {
    return is_land(ter[tidx(w, x, y)]) && adj_shore(ter, w, h, x, y);
}

static u64 dist2 (u16 ax, u16 ay, u16 bx, u16 by) {
    const i64 dx = static_cast<i64>(ax) - static_cast<i64>(bx);
    const i64 dy = static_cast<i64>(ay) - static_cast<i64>(by);
    return static_cast<u64>(dx * dx + dy * dy);
}

struct MassRank {
    u16 m_id;
    u32 m_sz;
};

static void sort_mass_desc (MassRank* a, u32 n) {
    for (u32 i = 1; i < n; ++i) {
        MassRank key = a[i];
        i32 j = static_cast<i32>(i) - 1;
        while (j >= 0 && a[static_cast<u32>(j)].m_sz < key.m_sz) {
            a[static_cast<u32>(j + 1)] = a[static_cast<u32>(j)];
            --j;
        }
        a[static_cast<u32>(j + 1)] = key;
    }
}

static u32 gather_coast (
    const u8* ter,
    const u16* mass,
    u16 w,
    u16 h,
    u16 mass_id,
    u32* out,
    u32 cap) 
{
    if (ter == nullptr || mass == nullptr || out == nullptr || cap == 0) {
        return 0;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 cn = 0;
    for (u32 i = 0; i < n && cn < cap; ++i) {
        if (mass[i] != mass_id) {
            continue;
        }
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u16 x = static_cast<u16>(i - static_cast<u32>(y) * static_cast<u32>(w));
        if (!is_coast_land(ter, w, h, x, y)) {
            continue;
        }
        out[cn++] = i;
    }
    return cn;
}

static void idx_xy (u32 i, u16 w, u16* x, u16* y) {
    *y = static_cast<u16>(i / static_cast<u32>(w));
    *x = static_cast<u16>(i - static_cast<u32>(*y) * static_cast<u32>(w));
}

static bool farthest_from_pt (
    const u32* coast,
    u32 coast_n,
    u16 w,
    u16 rx,
    u16 ry,
    u16* out_x,
    u16* out_y) 
{
    if (coast == nullptr || coast_n == 0 || out_x == nullptr || out_y == nullptr) {
        return false;
    }
    u64 best = 0;
    u32 bi = coast[0];
    for (u32 k = 0; k < coast_n; ++k) {
        u16 x = 0;
        u16 y = 0;
        idx_xy(coast[k], w, &x, &y);
        const u64 d = dist2(x, y, rx, ry);
        if (d > best) {
            best = d;
            bi = coast[k];
        }
    }
    idx_xy(bi, w, out_x, out_y);
    return true;
}

static bool opposite_pair (
    const u32* coast,
    u32 coast_n,
    u16 w,
    P1_CoastPt* a,
    P1_CoastPt* b) 
{
    if (coast == nullptr || coast_n == 0 || a == nullptr || b == nullptr) {
        return false;
    }
    i64 sx = 0;
    i64 sy = 0;
    for (u32 k = 0; k < coast_n; ++k) {
        u16 x = 0;
        u16 y = 0;
        idx_xy(coast[k], w, &x, &y);
        sx += static_cast<i64>(x);
        sy += static_cast<i64>(y);
    }
    const u16 cx = static_cast<u16>(sx / static_cast<i64>(coast_n));
    const u16 cy = static_cast<u16>(sy / static_cast<i64>(coast_n));
    if (!farthest_from_pt(coast, coast_n, w, cx, cy, &a->m_x, &a->m_y)) {
        return false;
    }
    if (!farthest_from_pt(coast, coast_n, w, a->m_x, a->m_y, &b->m_x, &b->m_y)) {
        return false;
    }
    return true;
}

static u32 pick_pts (
    const u8* ter,
    u16 w,
    u16 h,
    P1_CoastPtTrip* out,
    u32 cap) 
{
    if (ter == nullptr || out == nullptr || cap == 0 || !p1_map_size_ok(w, h)) {
        return 0;
    }
    LandMassIndex lmi;
    if (!lmi.generate(ter, w, h) || !lmi.is_valid()) {
        return 0;
    }
    const LandMassIndexRslt& lm = lmi.result();
    if (lm.m_ov == nullptr || lm.m_mass_n == 0u) {
        return 0;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_4B wb_sz("P1_Proc_DistributedCoastalPoints", "mass_sz", 0u);
    P1_WB_CHK(wb_sz);
    u32* sz = wb_sz.get_iter_ptr();
    std::memset(sz, 0, (static_cast<u32>(lm.m_mass_n) + 1u) * sizeof(u32));
    u32 max_sz = 0;
    for (u32 i = 0; i < n; ++i) {
        const u16 mi = lm.m_ov[i];
        if (mi == 0u || mi > lm.m_mass_n) {
            continue;
        }
        sz[mi]++;
        if (sz[mi] > max_sz) {
            max_sz = sz[mi];
        }
    }
    if (max_sz == 0u) {
        return 0;
    }
    const u32 thr = (max_sz * k_min_mass_pct) / 100u;
    MassRank rank[k_max_mass];
    u32 rn = 0;
    for (u16 id = 1u; id <= lm.m_mass_n && rn < k_max_mass; ++id) {
        if (sz[id] < thr || sz[id] == 0u) {
            continue;
        }
        rank[rn].m_id = id;
        rank[rn].m_sz = sz[id];
        rn++;
    }
    if (rn == 0u) {
        return 0;
    }
    sort_mass_desc(rank, rn);
    Whiteboard_4B wb_coast("P1_Proc_DistributedCoastalPoints", "coast_idx", 0u);
    P1_WB_CHK(wb_coast);
    u32* coast = wb_coast.get_iter_ptr();
    u32 pn = 0;
    for (u32 mi = 0; mi < rn && pn < cap; ++mi) {
        const u32 cn = gather_coast(ter, lm.m_ov, w, h, rank[mi].m_id, coast, n);
        if (cn == 0u) {
            continue;
        }
        P1_CoastPt a;
        P1_CoastPt b;
        if (!opposite_pair(coast, cn, w, &a, &b)) {
            continue;
        }
        out[pn].m_cen = a;
        out[pn].m_a = a;
        out[pn].m_b = a;
        pn++;
        if (pn < cap && (a.m_x != b.m_x || a.m_y != b.m_y)) {
            out[pn].m_cen = b;
            out[pn].m_a = b;
            out[pn].m_b = b;
            pn++;
        }
    }
    return pn;
}

//================================================================================================================================
//=> - P1_Proc_DistributedCoastalPoints -
//================================================================================================================================

P1_Proc_DistributedCoastalPoints::P1_Proc_DistributedCoastalPoints (const P1_RunPrm& prm) :
    m_prm(prm) {
}

u32 P1_Proc_DistributedCoastalPoints::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    P1_CoastPtTrip* out,
    u32 cap) 
{
    PTIME_START(__func__);
    if (terrain == nullptr || out == nullptr || cap == 0
        || w != m_prm.m_w || h != m_prm.m_h || !p1_run_prm_ok(m_prm)) {
        PTIME_STOP(__func__);
        return 0;
    }
    const u32 n = pick_pts(terrain, w, h, out, cap);
    PTIME_STOP(__func__);
    return n;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
