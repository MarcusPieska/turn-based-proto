//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_coastal_protrusion.h"

#include "game_map_defs.h"
#include "land_mass_index.h"
#include "p1_gen_coastal_land_overlay.h"
#include "p1_gen_fine_perlin.h"
#include "p1_wb_util.h"
#include "profile_time.h"

#include <cmath>
#include <cstring>

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

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

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]
        || cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static void or_mark (u8* dst, const u8* src, u32 n) {
    for (u32 i = 0; i < n; ++i) {
        if (src[i] != 0) {
            dst[i] = 1;
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

static u32 count_coast (const u8* coast, u32 n) {
    return count_nz(coast, n);
}

static u8 flood_stop (u16 w) {
    const u16 s = static_cast<u16>(w / 100u);
    return (s == 0u) ? 1u : static_cast<u8>(s > 255u ? 255u : s);
}

static bool mass_centroid (
    const u16* mass,
    u16 mass_id,
    u16 w,
    u16 h,
    f32* out_x,
    f32* out_y) 
{
    if (mass == nullptr || out_x == nullptr || out_y == nullptr || mass_id == 0u) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    i64 sx = 0;
    i64 sy = 0;
    u32 cn = 0;
    for (u32 i = 0; i < n; ++i) {
        if (mass[i] != mass_id) {
            continue;
        }
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u16 x = static_cast<u16>(i - static_cast<u32>(y) * static_cast<u32>(w));
        sx += static_cast<i64>(x);
        sy += static_cast<i64>(y);
        cn++;
    }
    if (cn == 0u) {
        return false;
    }
    *out_x = static_cast<f32>(sx) / static_cast<f32>(cn);
    *out_y = static_cast<f32>(sy) / static_cast<f32>(cn);
    return true;
}

static bool seaward_pull (
    u16 px,
    u16 py,
    f32 cx,
    f32 cy,
    i32 dist,
    i32* ox,
    i32* oy) 
{
    if (ox == nullptr || oy == nullptr || dist <= 0) {
        return false;
    }
    f32 fx = static_cast<f32>(px) - cx;
    f32 fy = static_cast<f32>(py) - cy;
    f32 len = std::sqrt(fx * fx + fy * fy);
    if (len < 1e-3f) {
        fx = 1.f;
        fy = 0.f;
        len = 1.f;
    }
    *ox = static_cast<i32>(std::lrint(static_cast<f32>(dist) * fx / len));
    *oy = static_cast<i32>(std::lrint(static_cast<f32>(dist) * fy / len));
    return *ox != 0 || *oy != 0 || dist == 0;
}

static u32 translate_coast (
    const u8* coast,
    u16 w,
    u16 h,
    i32 ox,
    i32 oy,
    u8* out) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(out, 0, static_cast<size_t>(n));
    u32 cn = 0;
    for (u32 i = 0; i < n; ++i) {
        if (coast[i] == 0) {
            continue;
        }
        const u16 sy = static_cast<u16>(i / static_cast<u32>(w));
        const u16 sx = static_cast<u16>(i - static_cast<u32>(sy) * static_cast<u32>(w));
        const i32 dx = static_cast<i32>(sx) + ox;
        const i32 dy = static_cast<i32>(sy) + oy;
        if (!in_map(w, h, dx, dy)) {
            continue;
        }
        const u32 di = tidx(w, static_cast<u16>(dx), static_cast<u16>(dy));
        if (out[di] == 0) {
            out[di] = 1;
            cn++;
        }
    }
    return cn;
}

static i32 pick_deg (i32 abs_deg, u32 seed) {
    i32 d = abs_deg;
    if (d < 0) {
        d = -d;
    }
    if (d == 0) {
        d = 20;
    }
    Rng32 rng;
    rng_seed(&rng, seed ^ 0xA11CE7u);
    return (rng_next(&rng) & 1u) != 0u ? d : -d;
}

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

struct RotBBox {
    i32 m_x0;
    i32 m_y0;
    i32 m_x1;
    i32 m_y1;
};

static bool coast_bbox (const u8* coast, u16 w, u16 h, RotBBox* bb) {
    if (coast == nullptr || bb == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    i32 x0 = static_cast<i32>(w);
    i32 y0 = static_cast<i32>(h);
    i32 x1 = -1;
    i32 y1 = -1;
    for (u32 i = 0; i < n; ++i) {
        if (coast[i] == 0) {
            continue;
        }
        const i32 y = static_cast<i32>(i / static_cast<u32>(w));
        const i32 x = static_cast<i32>(i - static_cast<u32>(y) * static_cast<u32>(w));
        if (x < x0) {
            x0 = x;
        }
        if (y < y0) {
            y0 = y;
        }
        if (x > x1) {
            x1 = x;
        }
        if (y > y1) {
            y1 = y;
        }
    }
    if (x1 < 0) {
        return false;
    }
    bb->m_x0 = x0;
    bb->m_y0 = y0;
    bb->m_x1 = x1;
    bb->m_y1 = y1;
    return true;
}

static void rot_pt_cw (f32 sx, f32 sy, f32 cx, f32 cy, f32 co, f32 si, f32* dx, f32* dy) {
    const f32 rx = sx - cx;
    const f32 ry = sy - cy;
    *dx = co * rx + si * ry + cx;
    *dy = -si * rx + co * ry + cy;
}

static void inv_pt_cw (f32 dx, f32 dy, f32 cx, f32 cy, f32 co, f32 si, f32* sx, f32* sy) {
    const f32 rx = dx - cx;
    const f32 ry = dy - cy;
    *sx = co * rx - si * ry + cx;
    *sy = si * rx + co * ry + cy;
}

static void rot_corners_aabb (
    const RotBBox& src,
    u16 px,
    u16 py,
    f32 co,
    f32 si,
    i32* minx,
    i32* maxx,
    i32* miny,
    i32* maxy) 
{
    const f32 cx = static_cast<f32>(px);
    const f32 cy = static_cast<f32>(py);
    const f32 xs[4] = {
        static_cast<f32>(src.m_x0), static_cast<f32>(src.m_x1),
        static_cast<f32>(src.m_x0), static_cast<f32>(src.m_x1)};
    const f32 ys[4] = {
        static_cast<f32>(src.m_y0), static_cast<f32>(src.m_y0),
        static_cast<f32>(src.m_y1), static_cast<f32>(src.m_y1)};
    f32 dx = 0.f;
    f32 dy = 0.f;
    rot_pt_cw(xs[0], ys[0], cx, cy, co, si, &dx, &dy);
    f32 x0 = dx;
    f32 x1 = dx;
    f32 y0 = dy;
    f32 y1 = dy;
    for (i32 k = 1; k < 4; ++k) {
        rot_pt_cw(xs[k], ys[k], cx, cy, co, si, &dx, &dy);
        if (dx < x0) {
            x0 = dx;
        }
        if (dx > x1) {
            x1 = dx;
        }
        if (dy < y0) {
            y0 = dy;
        }
        if (dy > y1) {
            y1 = dy;
        }
    }
    *minx = static_cast<i32>(std::floor(x0)) - 1;
    *miny = static_cast<i32>(std::floor(y0)) - 1;
    *maxx = static_cast<i32>(std::ceil(x1)) + 1;
    *maxy = static_cast<i32>(std::ceil(y1)) + 1;
}

static bool rot_bbox_in_margin (
    const RotBBox& src,
    u16 px,
    u16 py,
    i32 deg,
    u16 w,
    u16 h,
    i32 marg) 
{
    const i32 x_lo = marg;
    const i32 y_lo = marg;
    const i32 x_hi = static_cast<i32>(w) - 1 - marg;
    const i32 y_hi = static_cast<i32>(h) - 1 - marg;
    if (x_hi < x_lo || y_hi < y_lo) {
        return false;
    }
    const f32 rad = static_cast<f32>(deg) * static_cast<f32>(M_PI) / 180.f;
    i32 minx = 0;
    i32 maxx = 0;
    i32 miny = 0;
    i32 maxy = 0;
    rot_corners_aabb(src, px, py, std::cos(rad), std::sin(rad), &minx, &maxx, &miny, &maxy);
    return minx >= x_lo && miny >= y_lo && maxx <= x_hi && maxy <= y_hi;
}

static i32 limit_deg_margin (
    const RotBBox& src,
    u16 px,
    u16 py,
    i32 deg,
    u16 w,
    u16 h,
    i32 marg) 
{
    if (rot_bbox_in_margin(src, px, py, deg, w, h, marg)) {
        return deg;
    }
    const i32 sign = (deg >= 0) ? 1 : -1;
    i32 abs_d = deg;
    if (abs_d < 0) {
        abs_d = -abs_d;
    }
    i32 lo = 0;
    i32 hi = abs_d;
    i32 best = 0;
    while (lo <= hi) {
        const i32 mid = (lo + hi) / 2;
        const i32 trial = sign * mid;
        if (rot_bbox_in_margin(src, px, py, trial, w, h, marg)) {
            best = trial;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return best;
}

static RotBBox rot_src_bbox (const RotBBox& src, u16 px, u16 py, f32 co, f32 si, u16 w, u16 h) {
    i32 minx = 0;
    i32 maxx = 0;
    i32 miny = 0;
    i32 maxy = 0;
    rot_corners_aabb(src, px, py, co, si, &minx, &maxx, &miny, &maxy);
    RotBBox d;
    d.m_x0 = minx;
    d.m_y0 = miny;
    d.m_x1 = maxx;
    d.m_y1 = maxy;
    if (d.m_x0 < 0) {
        d.m_x0 = 0;
    }
    if (d.m_y0 < 0) {
        d.m_y0 = 0;
    }
    if (d.m_x1 >= static_cast<i32>(w)) {
        d.m_x1 = static_cast<i32>(w) - 1;
    }
    if (d.m_y1 >= static_cast<i32>(h)) {
        d.m_y1 = static_cast<i32>(h) - 1;
    }
    return d;
}

static u32 rot_coast_fwd (
    const u8* coast,
    u16 w,
    u16 h,
    u16 px,
    u16 py,
    f32 co,
    f32 si,
    u8* out) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(out, 0, static_cast<size_t>(n));
    const f32 cx = static_cast<f32>(px);
    const f32 cy = static_cast<f32>(py);
    u32 cn = 0;
    for (u32 i = 0; i < n; ++i) {
        if (coast[i] == 0) {
            continue;
        }
        const u16 sy = static_cast<u16>(i / static_cast<u32>(w));
        const u16 sx = static_cast<u16>(i - static_cast<u32>(sy) * static_cast<u32>(w));
        f32 dx = 0.f;
        f32 dy = 0.f;
        rot_pt_cw(static_cast<f32>(sx), static_cast<f32>(sy), cx, cy, co, si, &dx, &dy);
        const i32 ix = static_cast<i32>(std::lrint(dx));
        const i32 iy = static_cast<i32>(std::lrint(dy));
        if (!in_map(w, h, ix, iy)) {
            continue;
        }
        const u32 di = tidx(w, static_cast<u16>(ix), static_cast<u16>(iy));
        if (out[di] == 0) {
            out[di] = 1;
            cn++;
        }
    }
    return cn;
}

static u32 rot_coast_inv (
    const u8* coast,
    u16 w,
    u16 h,
    u16 px,
    u16 py,
    f32 co,
    f32 si,
    u8* out) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(out, 0, static_cast<size_t>(n));
    RotBBox src;
    if (!coast_bbox(coast, w, h, &src)) {
        return 0;
    }
    const RotBBox dst = rot_src_bbox(src, px, py, co, si, w, h);
    if (dst.m_x1 < dst.m_x0 || dst.m_y1 < dst.m_y0) {
        return 0;
    }
    const f32 cx = static_cast<f32>(px);
    const f32 cy = static_cast<f32>(py);
    u32 cn = 0;
    for (i32 y = dst.m_y0; y <= dst.m_y1; ++y) {
        for (i32 x = dst.m_x0; x <= dst.m_x1; ++x) {
            f32 sx = 0.f;
            f32 sy = 0.f;
            inv_pt_cw(static_cast<f32>(x), static_cast<f32>(y), cx, cy, co, si, &sx, &sy);
            const i32 ix = static_cast<i32>(std::lrint(sx));
            const i32 iy = static_cast<i32>(std::lrint(sy));
            if (!in_map(w, h, ix, iy)) {
                continue;
            }
            if (coast[tidx(w, static_cast<u16>(ix), static_cast<u16>(iy))] == 0) {
                continue;
            }
            const u32 di = tidx(w, static_cast<u16>(x), static_cast<u16>(y));
            if (out[di] == 0) {
                out[di] = 1;
                cn++;
            }
        }
    }
    return cn;
}

static u32 plug_flood1 (u8* mark, u16 w, u16 h) {
    if (mark == nullptr) {
        return 0;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_1B wb_add("P1_Gen_CoastalProtrusion", "plug_add", 0u);
    P1_WB_CHK(wb_add);
    u8* add = wb_add.raw();
    std::memset(add, 0, static_cast<size_t>(n));
    u32 cn = 0;
    for (u32 i = 0; i < n; ++i) {
        if (mark[i] == 0) {
            continue;
        }
        cn++; 
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
            if (mark[ni] == 0) {
                add[ni] = 1;
            }
        }
    }
    for (u32 i = 0; i < n; ++i) {
        if (add[i] == 0 || mark[i] != 0) {
            continue;
        }
        mark[i] = 1;
        cn++;
    }
    return cn;
}

static u32 rot_coast (
    const u8* coast,
    u16 w,
    u16 h,
    u16 px,
    u16 py,
    i32 deg_cw,
    u8 rot_mode,
    u8 plug_mode,
    u8* out) 
{
    const f32 rad = static_cast<f32>(deg_cw) * static_cast<f32>(M_PI) / 180.f;
    const f32 co = std::cos(rad);
    const f32 si = std::sin(rad);
    u32 cn = 0;
    if (rot_mode == P1_COAST_ROT_INV) {
        cn = rot_coast_inv(coast, w, h, px, py, co, si, out);
    } else {
        cn = rot_coast_fwd(coast, w, h, px, py, co, si, out);
    }
    if (cn == 0u) {
        return 0;
    }
    if (plug_mode == P1_COAST_PLUG_FLOOD1) {
        cn = plug_flood1(out, w, h);
    }
    return cn;
}

static u32 clip_over_wat (const u8* ter, const u8* rot, u16 w, u16 h, u8* out) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(out, 0, static_cast<size_t>(n));
    u32 cn = 0;
    for (u32 i = 0; i < n; ++i) {
        if (rot[i] == 0 || !is_wat(ter[i])) {
            continue;
        }
        out[i] = 1;
        cn++;
    }
    return cn;
}

static const u32 k_buck_n = 100u;

static u32 buck_idx (f32 v, f32 vmin, f32 denom) {
    f32 t = (v - vmin) / denom;
    if (t < 0.f) {
        t = 0.f;
    }
    if (t > 1.f) {
        t = 1.f;
    }
    u32 b = static_cast<u32>(t * static_cast<f32>(k_buck_n));
    if (b >= k_buck_n) {
        b = k_buck_n - 1u;
    }
    return b;
}

static bool cull_top_perlin (
    const u8* clip,
    const f32* perlin,
    u16 w,
    u16 h,
    u8 keep_pct,
    u8* out,
    bool or_in,
    u32* keep_n,
    f32* thr_out) 
{
    if (clip == nullptr || perlin == nullptr || out == nullptr || keep_n == nullptr || thr_out == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (!or_in) {
        std::memset(out, 0, static_cast<size_t>(n));
    }
    u32 cand_n = 0;
    f32 vmin = 0.f;
    f32 vmax = 0.f;
    bool have = false;
    for (u32 i = 0; i < n; ++i) {
        if (clip[i] == 0) {
            continue;
        }
        const f32 v = perlin[i];
        if (!have) {
            vmin = v;
            vmax = v;
            have = true;
        } else {
            if (v < vmin) {
                vmin = v;
            }
            if (v > vmax) {
                vmax = v;
            }
        }
        cand_n++;
    }
    if (cand_n == 0u || !have) {
        *keep_n = 0;
        *thr_out = 0.f;
        return false;
    }
    f32 denom = vmax - vmin;
    if (denom < 1e-12f) {
        denom = 1.f;
    }
    u32 buck[k_buck_n];
    std::memset(buck, 0, sizeof(buck));
    for (u32 i = 0; i < n; ++i) {
        if (clip[i] == 0) {
            continue;
        }
        buck[buck_idx(perlin[i], vmin, denom)]++;
    }
    u8 pct = keep_pct;
    if (pct > 100u) {
        pct = 100u;
    }
    const u32 want = (cand_n * static_cast<u32>(pct) + 50u) / 100u;
    u32 cum = 0;
    u32 cut_b = 0u;
    for (u32 bi = k_buck_n; bi > 0u;) {
        --bi;
        cum += buck[bi];
        if (cum >= want) {
            cut_b = bi;
            break;
        }
    }
    const f32 thr = vmin + static_cast<f32>(cut_b) * denom / static_cast<f32>(k_buck_n);
    u32 kn = 0;
    for (u32 i = 0; i < n; ++i) {
        if (clip[i] == 0) {
            continue;
        }
        if (buck_idx(perlin[i], vmin, denom) < cut_b) {
            continue;
        }
        out[i] = 1;
        kn++;
    }
    if (or_in) {
        kn = 0;
        for (u32 i = 0; i < n; ++i) {
            if (out[i] != 0) {
                kn++;
            }
        }
    }
    *keep_n = kn;
    *thr_out = thr;
    return kn > 0u;
}

static bool cull_dual_perlin (
    const u8* clip,
    const f32* rough,
    const f32* fine,
    u16 w,
    u16 h,
    u8 keep_rough,
    u8 keep_fine,
    u8* out,
    u32* keep_n,
    f32* thr_out) 
{
    if (rough == nullptr || fine == nullptr) {
        return false;
    }
    u8 kr = keep_rough;
    u8 kf = keep_fine;
    if (kr == 0u) {
        kr = 30u;
    }
    if (kf == 0u) {
        kf = 70u;
    }
    u32 kn = 0;
    f32 thr = 0.f;
    if (!cull_top_perlin(clip, rough, w, h, kr, out, false, &kn, &thr)) {
        return false;
    }
    u32 kn_f = 0;
    f32 thr_f = 0.f;
    if (!cull_top_perlin(clip, fine, w, h, kf, out, true, &kn_f, &thr_f)) {
        return false;
    }
    *keep_n = kn_f;
    *thr_out = thr;
    return kn_f > 0u;
}

static bool build_protrusion (
    const P1_RunPrm& prm,
    const u8* ter,
    const f32* perlin,
    u16 w,
    u16 h,
    u32 seed,
    const P1_Gen_CoastalProtrusionPrm& sp,
    P1_CoastPtTrip* trips,
    u32 trips_n,
    P1_Gen_CoastalProtrusionRslt* out) 
{
    if (out == nullptr || ter == nullptr || perlin == nullptr || trips == nullptr || trips_n == 0
        || !p1_map_size_ok(w, h)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (!out->m_rot.resize(w, h) || !out->m_clip.resize(w, h) || !out->m_ov.resize(w, h)) {
        out->m_rot.clear();
        out->m_clip.clear();
        out->m_ov.clear();
        return false;
    }
    u8* rot = out->m_rot.data_w();
    u8* clip = out->m_clip.data_w();
    u8* ov = out->m_ov.data_w();
    if (rot == nullptr || clip == nullptr || ov == nullptr) {
        out->m_rot.clear();
        out->m_clip.clear();
        out->m_ov.clear();
        return false;
    }
    std::memset(rot, 0, static_cast<size_t>(n));
    std::memset(clip, 0, static_cast<size_t>(n));
    std::memset(ov, 0, static_cast<size_t>(n));
    Whiteboard_1B wb_rot("P1_Gen_CoastalProtrusion", "rot_tmp", seed);
    Whiteboard_1B wb_clip("P1_Gen_CoastalProtrusion", "clip_tmp", seed);
    Whiteboard_1B wb_ov("P1_Gen_CoastalProtrusion", "ov_tmp", seed);
    Whiteboard_1B wb_pulled("P1_Gen_CoastalProtrusion", "pulled", seed);
    Whiteboard_4B wb_fine("P1_Gen_CoastalProtrusion", "fine", seed);
    P1_WB_CHK(wb_rot);
    P1_WB_CHK(wb_clip);
    P1_WB_CHK(wb_ov);
    P1_WB_CHK(wb_pulled);
    P1_WB_CHK(wb_fine);
    u8* rot_tmp = wb_rot.raw();
    u8* clip_tmp = wb_clip.raw();
    u8* ov_tmp = wb_ov.raw();
    u8* pulled = wb_pulled.raw();
    f32* fine = reinterpret_cast<f32*>(wb_fine.get_iter_ptr());
    P1_Gen_FinePerlin fine_gen(prm);
    if (!fine_gen.generate(perlin, w, h, fine) || !fine_gen.is_valid()) {
        out->m_rot.clear();
        out->m_clip.clear();
        out->m_ov.clear();
        return false;
    }
    LandMassIndex lmi;
    const u16* mass_ov = nullptr;
    if (lmi.generate(ter, w, h) && lmi.is_valid()) {
        mass_ov = lmi.result().m_ov;
    }
    const u8 stop = flood_stop(w);
    u8 pull_mul = sp.m_pull_mul;
    if (pull_mul == 0u) {
        pull_mul = 5u;
    }
    const i32 pull_dist = static_cast<i32>(stop) * static_cast<i32>(pull_mul);
    u8 swing = sp.m_swing_mode;
    if (swing > P1_COAST_SWING_ALT) {
        swing = P1_COAST_SWING_ALT;
    }
    u32 ok_n = 0;
    u32 src_sum = 0;
    i32 last_deg = 0;
    u16 last_px = 0;
    u16 last_py = 0;
    f32 last_thr = 0.f;

    const i32 marg = static_cast<i32>((w < h ? w : h) / 100u);

    auto apply_swing = [&] (
        const u8* coast_src,
        u16 piv_x,
        u16 piv_y,
        i32 deg_abs,
        u32 pseed,
        i32* out_deg,
        f32* out_thr) -> bool {
        RotBBox src_bb;
        if (!coast_bbox(coast_src, w, h, &src_bb)) {
            return false;
        }
        i32 deg = pick_deg(deg_abs, pseed);
        deg = limit_deg_margin(src_bb, piv_x, piv_y, deg, w, h, marg);
        if (!rot_bbox_in_margin(src_bb, piv_x, piv_y, deg, w, h, marg)) {
            return false;
        }
        const u32 rot_n = rot_coast(
            coast_src, w, h, piv_x, piv_y, deg, sp.m_rot_mode, sp.m_plug_mode, rot_tmp);
        if (rot_n == 0u) {
            return false;
        }
        const u32 clip_n = clip_over_wat(ter, rot_tmp, w, h, clip_tmp);
        if (clip_n == 0u) {
            return false;
        }
        u32 keep_n = 0;
        f32 thr = 0.f;
        if (!cull_dual_perlin(
                clip_tmp, perlin, fine, w, h, sp.m_keep_rough, sp.m_keep_fine, ov_tmp, &keep_n, &thr)
            || keep_n == 0u) {
            return false;
        }
        or_mark(rot, rot_tmp, n);
        or_mark(clip, clip_tmp, n);
        or_mark(ov, ov_tmp, n);
        if (out_deg != nullptr) {
            *out_deg = deg;
        }
        if (out_thr != nullptr) {
            *out_thr = thr;
        }
        return true;
    };

    for (u32 pi = 0; pi < trips_n; ++pi) {
        P1_CoastPtTrip& trip = trips[pi];
        P1_Gen_CoastalLandOverlay coast_gen(prm);
        if (!coast_gen.generate(ter, w, h, &trip, 1u) || !coast_gen.is_valid()) {
            continue;
        }
        const u8* coast = coast_gen.result().m_ov.data();
        if (coast == nullptr) {
            continue;
        }
        src_sum += count_coast(coast, n);
        const u32 pseed = seed ^ (0x9E3779B9u * (pi + 1u));
        u8 use = swing;
        if (swing == P1_COAST_SWING_ALT) {
            use = (pi & 1u) == 0u ? P1_COAST_SWING_ROT20 : P1_COAST_SWING_PULL10;
        }
        bool any = false;
        if (use == P1_COAST_SWING_ROT20) {
            Rng32 rng;
            rng_seed(&rng, pseed ^ 0xE27E11u);
            const P1_CoastPt piv = ((rng_next(&rng) & 1u) != 0u) ? trip.m_a : trip.m_b;
            i32 deg = 0;
            f32 thr = 0.f;
            if (apply_swing(coast, piv.m_x, piv.m_y, sp.m_deg, pseed, &deg, &thr)) {
                any = true;
                last_deg = deg;
                last_px = piv.m_x;
                last_py = piv.m_y;
                last_thr = thr;
            }
        } else {
            const P1_CoastPt pt = trip.m_cen;
            i32 ox = 0;
            i32 oy = 0;
            bool have_dir = false;
            if (mass_ov != nullptr) {
                const u32 pi_i = tidx(w, pt.m_x, pt.m_y);
                const u16 mid = mass_ov[pi_i];
                f32 cx = 0.f;
                f32 cy = 0.f;
                if (mid != 0u && mass_centroid(mass_ov, mid, w, h, &cx, &cy)) {
                    have_dir = seaward_pull(pt.m_x, pt.m_y, cx, cy, pull_dist, &ox, &oy);
                }
            }
            if (!have_dir) {
                ox = pull_dist;
                oy = 0;
            }
            if (translate_coast(coast, w, h, ox, oy, pulled) != 0u) {
                const i32 piv_x = static_cast<i32>(pt.m_x) + ox;
                const i32 piv_y = static_cast<i32>(pt.m_y) + oy;
                if (in_map(w, h, piv_x, piv_y)) {
                    i32 deg = 0;
                    f32 thr = 0.f;
                    const u32 pseed2 = pseed ^ 0x51F15EEDu;
                    if (apply_swing(
                            pulled,
                            static_cast<u16>(piv_x),
                            static_cast<u16>(piv_y),
                            sp.m_pull_deg,
                            pseed2,
                            &deg,
                            &thr)) {
                        any = true;
                        last_deg = deg;
                        last_px = static_cast<u16>(piv_x);
                        last_py = static_cast<u16>(piv_y);
                        last_thr = thr;
                    }
                }
            }
        }
        if (any) {
            ok_n++;
        }
    }
    if (ok_n == 0u) {
        out->m_rot.clear();
        out->m_clip.clear();
        out->m_ov.clear();
        return false;
    }
    out->m_w = w;
    out->m_h = h;
    out->m_deg = last_deg;
    out->m_px = last_px;
    out->m_py = last_py;
    out->m_pt_n = ok_n;
    out->m_src_n = src_sum;
    out->m_rot_n = count_nz(rot, n);
    out->m_clip_n = count_nz(clip, n);
    out->m_keep_n = count_nz(ov, n);
    out->m_rot_mode = sp.m_rot_mode;
    out->m_plug_mode = sp.m_plug_mode;
    out->m_swing_mode = swing;
    out->m_thr = last_thr;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_CoastalProtrusion -
//================================================================================================================================

P1_Gen_CoastalProtrusion::P1_Gen_CoastalProtrusion (
    const P1_RunPrm& prm,
    const P1_Gen_CoastalProtrusionPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_ok(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_deg = 0;
    m_rslt.m_px = 0;
    m_rslt.m_py = 0;
    m_rslt.m_pt_n = 0;
    m_rslt.m_src_n = 0;
    m_rslt.m_rot_n = 0;
    m_rslt.m_clip_n = 0;
    m_rslt.m_keep_n = 0;
    m_rslt.m_rot_mode = 0;
    m_rslt.m_plug_mode = 0;
    m_rslt.m_swing_mode = 0;
    m_rslt.m_thr = 0.f;
}

bool P1_Gen_CoastalProtrusion::generate (
    const u8* terrain,
    const f32* perlin,
    u16 w,
    u16 h,
    P1_CoastPtTrip* trips,
    u32 trips_n) 
{
    PTIME_START(__func__);
    m_ok = false;
    m_rslt.m_rot.clear();
    m_rslt.m_clip.clear();
    m_rslt.m_ov.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_deg = 0;
    m_rslt.m_px = 0;
    m_rslt.m_py = 0;
    m_rslt.m_pt_n = 0;
    m_rslt.m_src_n = 0;
    m_rslt.m_rot_n = 0;
    m_rslt.m_clip_n = 0;
    m_rslt.m_keep_n = 0;
    m_rslt.m_rot_mode = 0;
    m_rslt.m_plug_mode = 0;
    m_rslt.m_swing_mode = 0;
    m_rslt.m_thr = 0.f;
    if (terrain == nullptr || perlin == nullptr || trips == nullptr || trips_n == 0
        || w != m_prm.m_w || h != m_prm.m_h || !p1_run_prm_ok(m_prm)) {
        PTIME_STOP(__func__);
        return false;
    }
    if (!build_protrusion(m_prm, terrain, perlin, w, h, m_prm.m_seed, m_sp, trips, trips_n, &m_rslt)) {
        m_rslt.m_rot.clear();
        m_rslt.m_clip.clear();
        m_rslt.m_ov.clear();
        PTIME_STOP(__func__);
        return false;
    }
    m_ok = true;
    PTIME_STOP(__func__);
    return true;
}

bool P1_Gen_CoastalProtrusion::is_valid () const {
    return m_ok;
}

const P1_Gen_CoastalProtrusionRslt& P1_Gen_CoastalProtrusion::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
