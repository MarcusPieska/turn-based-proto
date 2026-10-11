//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_coastal_land_overlay.h"

#include "game_map_defs.h"
#include "land_mass_index.h"
#include "p1_wb_util.h"
#include "profile_time.h"
#include "wb_que_xy.h"

#include <cstring>

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};
static const i32 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i32 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

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

static bool is_coast_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool adj_coast_wat (const u8* ter, u16 w, u16 h, u16 x, u16 y) {
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        if (is_coast_wat(ter[tidx(w, static_cast<u16>(nx), static_cast<u16>(ny))])) {
            return true;
        }
    }
    return false;
}

static bool is_coast_land (const u8* ter, u16 w, u16 h, u16 x, u16 y) {
    const u32 i = tidx(w, x, y);
    return is_land(ter[i]) && adj_coast_wat(ter, w, h, x, y);
}

static u8 inland_stop (u16 w) {
    const u16 s = static_cast<u16>(w / 100u);
    return (s == 0u) ? 1u : static_cast<u8>(s > 255u ? 255u : s);
}

static u32 flood_coast_fringe (
    const u8* ter,
    const u16* mass,
    u16 w,
    u16 h,
    u16 mass_id,
    u32 start,
    u8* mark,
    u32 n_max) 
{
    WB_QueXY q;
    if (!q.ok() || mark == nullptr) {
        return 0;
    }
    q.clear();
    std::memset(mark, 0, static_cast<size_t>(n_max));
    const u16 sy = static_cast<u16>(start / static_cast<u32>(w));
    const u16 sx = static_cast<u16>(start - static_cast<u32>(sy) * static_cast<u32>(w));
    if (!is_coast_land(ter, w, h, sx, sy) || mass[start] != mass_id) {
        return 0;
    }
    mark[start] = 1;
    if (!q.push(sx, sy)) {
        return 0;
    }
    u32 cn = 1;
    while (q.count() > 0) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        for (i32 k = 0; k < 8; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx8[k];
            const i32 ny = static_cast<i32>(py) + k_dy8[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const u32 ni = tidx(w, ux, uy);
            if (mark[ni] != 0 || mass[ni] != mass_id || !is_coast_land(ter, w, h, ux, uy)) {
                continue;
            }
            mark[ni] = 1;
            cn++;
            if (!q.push(ux, uy)) {
                return 0;
            }
        }
    }
    return cn;
}

static u32 flood_coast_cap (
    const u8* ter,
    const u16* mass,
    u16 w,
    u16 h,
    u16 mass_id,
    u32 start,
    u32 cap,
    u8* mark,
    u32 n_max) 
{
    WB_QueXY q;
    if (!q.ok() || mark == nullptr || cap == 0) {
        return 0;
    }
    q.clear();
    std::memset(mark, 0, static_cast<size_t>(n_max));
    const u16 sy = static_cast<u16>(start / static_cast<u32>(w));
    const u16 sx = static_cast<u16>(start - static_cast<u32>(sy) * static_cast<u32>(w));
    if (!is_coast_land(ter, w, h, sx, sy) || mass[start] != mass_id) {
        return 0;
    }
    mark[start] = 1;
    if (!q.push(sx, sy)) {
        return 0;
    }
    u32 cn = 1;
    while (q.count() > 0 && cn < cap) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        for (i32 k = 0; k < 8; ++k) {
            if (cn >= cap) {
                break;
            }
            const i32 nx = static_cast<i32>(px) + k_dx8[k];
            const i32 ny = static_cast<i32>(py) + k_dy8[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const u32 ni = tidx(w, ux, uy);
            if (mark[ni] != 0 || mass[ni] != mass_id || !is_coast_land(ter, w, h, ux, uy)) {
                continue;
            }
            mark[ni] = 1;
            cn++;
            if (!q.push(ux, uy)) {
                return 0;
            }
        }
    }
    return cn;
}

static u32 flood_inland (
    const u8* ter,
    u16 w,
    u16 h,
    const u8* seeds,
    u8 stop,
    u8* ov,
    u32 n_max) 
{
    WB_QueXY q;
    if (!q.ok() || ov == nullptr || seeds == nullptr || stop == 0) {
        return 0;
    }
    q.clear();
    std::memset(ov, 0, static_cast<size_t>(n_max));
    u32 fill_n = 0;
    for (u32 i = 0; i < n_max; ++i) {
        if (seeds[i] == 0 || !is_land(ter[i])) {
            continue;
        }
        ov[i] = stop;
        fill_n++;
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        if (!q.push(px, py)) {
            return 0;
        }
    }
    while (q.count() > 0) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        const u32 i = tidx(w, px, py);
        const u8 rem = ov[i];
        if (rem <= 1u) {
            continue;
        }
        const u8 nd = static_cast<u8>(rem - 1u);
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const u32 ni = tidx(w, ux, uy);
            if (ov[ni] != 0 || !is_land(ter[ni])) {
                continue;
            }
            ov[ni] = nd;
            fill_n++;
            if (!q.push(ux, uy)) {
                return 0;
            }
        }
    }
    return fill_n;
}

static void or_mark (u8* dst, const u8* src, u32 n) {
    for (u32 i = 0; i < n; ++i) {
        if (src[i] != 0) {
            dst[i] = 1;
        }
    }
}

static void max_mark (u8* dst, const u8* src, u32 n) {
    for (u32 i = 0; i < n; ++i) {
        if (src[i] > dst[i]) {
            dst[i] = src[i];
        }
    }
}

static u32 count_nz (const u8* a, u32 n) {
    u32 c = 0;
    for (u32 i = 0; i < n; ++i) {
        if (a[i] != 0) {
            c++;
        }
    }
    return c;
}

static u64 dist2_xy (u16 ax, u16 ay, u16 bx, u16 by) {
    const i64 dx = static_cast<i64>(ax) - static_cast<i64>(bx);
    const i64 dy = static_cast<i64>(ay) - static_cast<i64>(by);
    return static_cast<u64>(dx * dx + dy * dy);
}

static bool stalls_from_mark (
    const u8* mark,
    u16 w,
    u16 h,
    u16 cx,
    u16 cy,
    P1_CoastPt* a,
    P1_CoastPt* b) 
{
    if (mark == nullptr || a == nullptr || b == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    bool have_a = false;
    u64 best_a = 0;
    u16 ax = cx;
    u16 ay = cy;
    for (u32 i = 0; i < n; ++i) {
        if (mark[i] == 0) {
            continue;
        }
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u16 x = static_cast<u16>(i - static_cast<u32>(y) * static_cast<u32>(w));
        const u64 d = dist2_xy(x, y, cx, cy);
        if (!have_a || d > best_a) {
            best_a = d;
            ax = x;
            ay = y;
            have_a = true;
        }
    }
    if (!have_a) {
        return false;
    }
    a->m_x = ax;
    a->m_y = ay;
    bool have_b = false;
    u64 best_b = 0;
    u16 bx = ax;
    u16 by = ay;
    for (u32 i = 0; i < n; ++i) {
        if (mark[i] == 0) {
            continue;
        }
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u16 x = static_cast<u16>(i - static_cast<u32>(y) * static_cast<u32>(w));
        if (x == ax && y == ay) {
            continue;
        }
        const u64 d = dist2_xy(x, y, cx, cy);
        if (!have_b || d > best_b) {
            best_b = d;
            bx = x;
            by = y;
            have_b = true;
        }
    }
    if (!have_b) {
        *b = *a;
    } else {
        b->m_x = bx;
        b->m_y = by;
    }
    return true;
}

static bool apply_one_pt (
    const u8* ter,
    const u16* mass,
    u16 w,
    u16 h,
    u16 mass_id,
    u32 start,
    u8 frac,
    u8 stop,
    u8* coast_tmp,
    u8* seed_tmp,
    u8* ov_tmp,
    u8* coast,
    u8* seeds,
    u8* ov,
    u32 n,
    P1_CoastPtTrip* trip) 
{
    const u32 px_limit = static_cast<u32>(w);
    const u32 coast_n = flood_coast_fringe(ter, mass, w, h, mass_id, start, coast_tmp, n);
    if (coast_n == 0u) {
        return false;
    }
    u32 cap = (coast_n * static_cast<u32>(frac)) / 100u;
    if (cap == 0u) {
        cap = 1u;
    }
    if (cap > coast_n) {
        cap = coast_n;
    }
    if (cap > px_limit) {
        cap = px_limit;
    }
    const u32 seed_n = flood_coast_cap(ter, mass, w, h, mass_id, start, cap, seed_tmp, n);
    if (seed_n == 0u) {
        return false;
    }
    const u32 fill_n = flood_inland(ter, w, h, seed_tmp, stop, ov_tmp, n);
    if (fill_n == 0u) {
        return false;
    }
    if (trip != nullptr) {
        const u16 sy = static_cast<u16>(start / static_cast<u32>(w));
        const u16 sx = static_cast<u16>(start - static_cast<u32>(sy) * static_cast<u32>(w));
        if (!stalls_from_mark(ov_tmp, w, h, sx, sy, &trip->m_a, &trip->m_b)) {
            trip->m_a = trip->m_cen;
            trip->m_b = trip->m_cen;
        }
    }
    or_mark(coast, coast_tmp, n);
    or_mark(seeds, seed_tmp, n);
    max_mark(ov, ov_tmp, n);
    return true;
}

static bool build_overlay (
    const u8* ter,
    u16 w,
    u16 h,
    u8 frac,
    P1_CoastPtTrip* trips,
    u32 trips_n,
    P1_Gen_CoastalLandOverlayRslt* out) 
{
    if (out == nullptr || ter == nullptr || trips == nullptr || trips_n == 0 || !p1_map_size_ok(w, h)) {
        return false;
    }
    LandMassIndex lmi;
    if (!lmi.generate(ter, w, h) || !lmi.is_valid()) {
        return false;
    }
    const LandMassIndexRslt& lm = lmi.result();
    if (lm.m_ov == nullptr || lm.m_mass_n == 0u) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (!out->m_coast.resize(w, h) || !out->m_seeds.resize(w, h) || !out->m_ov.resize(w, h)) {
        out->m_coast.clear();
        out->m_seeds.clear();
        out->m_ov.clear();
        return false;
    }
    u8* coast = out->m_coast.data_w();
    u8* seeds = out->m_seeds.data_w();
    u8* ov = out->m_ov.data_w();
    if (coast == nullptr || seeds == nullptr || ov == nullptr) {
        out->m_coast.clear();
        out->m_seeds.clear();
        out->m_ov.clear();
        return false;
    }
    std::memset(coast, 0, static_cast<size_t>(n));
    std::memset(seeds, 0, static_cast<size_t>(n));
    std::memset(ov, 0, static_cast<size_t>(n));
    Whiteboard_1B wb_coast("P1_Gen_CoastalLandOverlay", "coast_tmp", 0u);
    Whiteboard_1B wb_seed("P1_Gen_CoastalLandOverlay", "seed_tmp", 0u);
    Whiteboard_1B wb_ov("P1_Gen_CoastalLandOverlay", "ov_tmp", 0u);
    P1_WB_CHK(wb_coast);
    P1_WB_CHK(wb_seed);
    P1_WB_CHK(wb_ov);
    u8* coast_tmp = wb_coast.raw();
    u8* seed_tmp = wb_seed.raw();
    u8* ov_tmp = wb_ov.raw();
    const u8 stop = inland_stop(w);
    u32 ok_n = 0;
    u16 first_mass = 0;
    for (u32 pi = 0; pi < trips_n; ++pi) {
        const u16 px = trips[pi].m_cen.m_x;
        const u16 py = trips[pi].m_cen.m_y;
        if (!in_map(w, h, static_cast<i32>(px), static_cast<i32>(py))) {
            continue;
        }
        if (!is_coast_land(ter, w, h, px, py)) {
            continue;
        }
        const u32 start = tidx(w, px, py);
        const u16 mass_id = lm.m_ov[start];
        if (mass_id == 0u) {
            continue;
        }
        if (!apply_one_pt(
                ter, lm.m_ov, w, h, mass_id, start, frac, stop,
                coast_tmp, seed_tmp, ov_tmp, coast, seeds, ov, n, &trips[pi])) {
            continue;
        }
        if (ok_n == 0u) {
            first_mass = mass_id;
        }
        ok_n++;
    }
    if (ok_n == 0u) {
        out->m_coast.clear();
        out->m_seeds.clear();
        out->m_ov.clear();
        return false;
    }
    out->m_w = w;
    out->m_h = h;
    out->m_stop = stop;
    out->m_mass = first_mass;
    out->m_pt_n = ok_n;
    out->m_coast_n = count_nz(coast, n);
    out->m_seed_n = count_nz(seeds, n);
    out->m_fill_n = count_nz(ov, n);
    return true;
}

//================================================================================================================================
//=> - P1_Gen_CoastalLandOverlay -
//================================================================================================================================

P1_Gen_CoastalLandOverlay::P1_Gen_CoastalLandOverlay (
    const P1_RunPrm& prm,
    const P1_Gen_CoastalLandOverlayPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_ok(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_stop = 0;
    m_rslt.m_mass = 0;
    m_rslt.m_pt_n = 0;
    m_rslt.m_coast_n = 0;
    m_rslt.m_seed_n = 0;
    m_rslt.m_fill_n = 0;
}

bool P1_Gen_CoastalLandOverlay::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    P1_CoastPtTrip* trips,
    u32 trips_n) 
{
    PTIME_START(__func__);
    m_ok = false;
    m_rslt.m_coast.clear();
    m_rslt.m_seeds.clear();
    m_rslt.m_ov.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_stop = 0;
    m_rslt.m_mass = 0;
    m_rslt.m_pt_n = 0;
    m_rslt.m_coast_n = 0;
    m_rslt.m_seed_n = 0;
    m_rslt.m_fill_n = 0;
    if (terrain == nullptr || trips == nullptr || trips_n == 0
        || w != m_prm.m_w || h != m_prm.m_h || !p1_run_prm_ok(m_prm)) {
        PTIME_STOP(__func__);
        return false;
    }
    u8 frac = m_sp.m_frac;
    if (frac == 0u) {
        frac = 25u;
    }
    if (frac > 100u) {
        frac = 100u;
    }
    if (!build_overlay(terrain, w, h, frac, trips, trips_n, &m_rslt)) {
        m_rslt.m_coast.clear();
        m_rslt.m_seeds.clear();
        m_rslt.m_ov.clear();
        PTIME_STOP(__func__);
        return false;
    }
    m_ok = true;
    PTIME_STOP(__func__);
    return true;
}

bool P1_Gen_CoastalLandOverlay::is_valid () const {
    return m_ok;
}

const P1_Gen_CoastalLandOverlayRslt& P1_Gen_CoastalLandOverlay::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
