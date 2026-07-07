//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_cont_outlines.h"

#include "generator_constants.h"
#include "p1_gen_outline.h"
#include "generate_terrain_rotation.h"
#include "whiteboard.h"

#include <cmath>
#include <cstring>

//================================================================================================================================
//=> - Private placement types -
//================================================================================================================================

static const u8 k_land = WL_OVERLAY_LAND_GRAY;
static const u8 k_water = WL_OVERLAY_WATER_GRAY;
static const u8 k_cont_water = 0u;
static const f64 k_pi = 3.14159265358979323846;
static const i32 k_ray_n = 12;

static u8 cont_id (u8 piece_idx) {
    return static_cast<u8>(piece_idx + 1u);
}

static bool is_cont (u8 v) {
    return v != k_cont_water;
}

static u8* wb_u8 (u16* p) {
    return reinterpret_cast<u8*>(p);
}

static u32 q_get (const u16* lo, const u16* hi, u32 i) {
    return static_cast<u32>(lo[i]) | (static_cast<u32>(hi[i]) << 16);
}

static void q_set (u16* lo, u16* hi, u32 i, u32 v) {
    lo[i] = static_cast<u16>(v);
    hi[i] = static_cast<u16>(v >> 16);
}

static void vis_bump (u16* vis, u32 n, u16* tag) {
    ++(*tag);
    if (*tag == 0u) {
        std::memset(vis, 0, static_cast<size_t>(n) * sizeof(u16));
        *tag = 1u;
    }
}

struct OvBBox {
    u16 m_x0;
    u16 m_y0;
    u16 m_x1;
    u16 m_y1;
    bool m_ok;
};

struct Mt19937 {
    u32 mt[624];
    i32 idx;
};

//================================================================================================================================
//=> - Private rng -
//================================================================================================================================

static void mt_seed (Mt19937* g, u32 seed) {
    g->mt[0] = seed;
    for (i32 i = 1; i < 624; ++i) {
        g->mt[i] = 1812433253u * (g->mt[i - 1] ^ (g->mt[i - 1] >> 30)) + static_cast<u32>(i);
    }
    g->idx = 624;
}

static void mt_twist (Mt19937* g) {
    for (i32 i = 0; i < 624; ++i) {
        const u32 y = (g->mt[i] & 0x80000000u) + (g->mt[(i + 1) % 624] & 0x7fffffffu);
        g->mt[i] = g->mt[(i + 397) % 624] ^ (y >> 1);
        if (y & 1u) {
            g->mt[i] ^= 0x9908b0dfu;
        }
    }
    g->idx = 0;
}

static u32 mt_next (Mt19937* g) {
    if (g->idx >= 624) {
        mt_twist(g);
    }
    u32 y = g->mt[g->idx++];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680u;
    y ^= (y << 15) & 0xefc60000u;
    y ^= y >> 18;
    return y;
}

static f64 rng01 (Mt19937* g) {
    return static_cast<f64>(mt_next(g)) / 4294967296.0;
}

static f64 rng_rot_deg (Mt19937* g) {
    return (rng01(g) * 2.0 - 1.0) * 30.0;
}

//================================================================================================================================
//=> - Private overlay helpers -
//================================================================================================================================

static bool is_land_ov (u8 v) {
    return v == k_land;
}

static OvBBox bbox_land (const u8* ov, u16 w, u16 h) {
    OvBBox bb = {};
    bb.m_x0 = w;
    bb.m_y0 = h;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_land_ov(ov[i])) {
                continue;
            }
            if (px < bb.m_x0) {
                bb.m_x0 = px;
            }
            if (py < bb.m_y0) {
                bb.m_y0 = py;
            }
            if (px > bb.m_x1) {
                bb.m_x1 = px;
            }
            if (py > bb.m_y1) {
                bb.m_y1 = py;
            }
            bb.m_ok = true;
        }
    }
    return bb;
}

static void land_cen (const u8* ov, u16 w, u16 h, f32* cx, f32* cy) {
    f64 sx = 0.0;
    f64 sy = 0.0;
    u32 c = 0u;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_land_ov(ov[i])) {
                continue;
            }
            sx += static_cast<f64>(px);
            sy += static_cast<f64>(py);
            ++c;
        }
    }
    if (c == 0u) {
        if (cx != nullptr) {
            *cx = static_cast<f32>(w) * 0.5f;
        }
        if (cy != nullptr) {
            *cy = static_cast<f32>(h) * 0.5f;
        }
        return;
    }
    if (cx != nullptr) {
        *cx = static_cast<f32>(sx / static_cast<f64>(c));
    }
    if (cy != nullptr) {
        *cy = static_cast<f32>(sy / static_cast<f64>(c));
    }
}

static void map_margins (u16 w, u16 h, u8 margin_floor, i32* mx, i32* my) {
    if (mx == nullptr || my == nullptr) {
        return;
    }
    i32 px = static_cast<i32>((static_cast<u32>(w) * 2u + 99u) / 100u);
    i32 py = static_cast<i32>((static_cast<u32>(h) * 2u + 99u) / 100u);
    if (static_cast<i32>(margin_floor) > px) {
        px = static_cast<i32>(margin_floor);
    }
    if (static_cast<i32>(margin_floor) > py) {
        py = static_cast<i32>(margin_floor);
    }
    *mx = px;
    *my = py;
}

static void clamp_off (const OvBBox& bb, u16 w, u16 h, i32 mx, i32 my, i32* dx, i32* dy) {
    if (dx == nullptr || dy == nullptr) {
        return;
    }
    const i32 hi_x = static_cast<i32>(w) - 1 - mx;
    const i32 hi_y = static_cast<i32>(h) - 1 - my;
    if (static_cast<i32>(bb.m_x0) + *dx < mx) {
        *dx = mx - static_cast<i32>(bb.m_x0);
    }
    if (static_cast<i32>(bb.m_y0) + *dy < my) {
        *dy = my - static_cast<i32>(bb.m_y0);
    }
    if (static_cast<i32>(bb.m_x1) + *dx > hi_x) {
        *dx = hi_x - static_cast<i32>(bb.m_x1);
    }
    if (static_cast<i32>(bb.m_y1) + *dy > hi_y) {
        *dy = hi_y - static_cast<i32>(bb.m_y1);
    }
}

static void norm_dir (f32 vx, f32 vy, f32* dir_x, f32* dir_y) {
    if (dir_x == nullptr || dir_y == nullptr) {
        return;
    }
    const f32 len = std::sqrt(vx * vx + vy * vy);
    if (len < 0.001f) {
        *dir_x = 1.f;
        *dir_y = 0.f;
        return;
    }
    *dir_x = vx / len;
    *dir_y = vy / len;
}

static void step_dir (f32 dir_x, f32 dir_y, i32* dx, i32* dy) {
    if (dx == nullptr || dy == nullptr) {
        return;
    }
    if (dir_x > 0.f) {
        *dx += 1;
    } else if (dir_x < 0.f) {
        *dx -= 1;
    }
    if (dir_y > 0.f) {
        *dy += 1;
    } else if (dir_y < 0.f) {
        *dy -= 1;
    }
}

static const i32 k_drag_step = 20;

struct OvlpFfRslt {
    u32 m_land;
    u32 m_ov;
};

static void step_dir_n (f32 dir_x, f32 dir_y, i32 dist, i32* dx, i32* dy) {
    if (dx == nullptr || dy == nullptr || dist <= 0) {
        return;
    }
    *dx += static_cast<i32>(std::lrint(static_cast<f64>(dir_x) * static_cast<f64>(dist)));
    *dy += static_cast<i32>(std::lrint(static_cast<f64>(dir_y) * static_cast<f64>(dist)));
}

static OvlpFfRslt ovlp_ff_cnt (
    const u8* piece,
    i32 dx,
    i32 dy,
    u16 w,
    u16 h,
    const u8* comp,
    u8 ovlp_id,
    u16* vis,
    u16* vis_tag,
    u16* qlo,
    u16* qhi) 
{
    OvlpFfRslt r = {0u, 0u};
    if (piece == nullptr || vis == nullptr || vis_tag == nullptr || qlo == nullptr || qhi == nullptr || w == 0 || h == 0) {
        return r;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u16 tag = *vis_tag;
    vis_bump(vis, n, vis_tag);
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_land_ov(piece[i]) || vis[i] == tag) {
                continue;
            }
            u32 qh = 0u;
            u32 qt = 0u;
            q_set(qlo, qhi, qt++, i);
            vis[i] = tag;
            while (qh < qt) {
                const u32 ci = q_get(qlo, qhi, qh++);
                const u16 cpx = static_cast<u16>(ci % static_cast<u32>(w));
                const u16 cpy = static_cast<u16>(ci / static_cast<u32>(w));
                ++r.m_land;
                const i32 nx = static_cast<i32>(cpx) + dx;
                const i32 ny = static_cast<i32>(cpy) + dy;
                if (comp != nullptr && nx >= 0 && ny >= 0
                    && nx < static_cast<i32>(w) && ny < static_cast<i32>(h)) {
                    const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                    const u8 cv = comp[j];
                    if (ovlp_id == 0u) {
                        if (is_cont(cv)) {
                            ++r.m_ov;
                        }
                    } else if (cv == ovlp_id) {
                        ++r.m_ov;
                    }
                }
                const i32 ox[4] = {1, -1, 0, 0};
                const i32 oy[4] = {0, 0, 1, -1};
                for (u8 k = 0; k < 4u; ++k) {
                    const i32 npx = static_cast<i32>(cpx) + ox[k];
                    const i32 npy = static_cast<i32>(cpy) + oy[k];
                    if (npx < 0 || npy < 0 || npx >= static_cast<i32>(w) || npy >= static_cast<i32>(h)) {
                        continue;
                    }
                    const u32 ni = static_cast<u32>(npy) * static_cast<u32>(w) + static_cast<u32>(npx);
                    if (!is_land_ov(piece[ni]) || vis[ni] == tag) {
                        continue;
                    }
                    vis[ni] = tag;
                    q_set(qlo, qhi, qt++, ni);
                }
            }
        }
    }
    return r;
}

static void blit_cont (const u8* piece, i32 dx, i32 dy, u16 w, u16 h, u8 cid, u8* comp) {
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_land_ov(piece[i])) {
                continue;
            }
            const i32 nx = static_cast<i32>(px) + dx;
            const i32 ny = static_cast<i32>(py) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            comp[j] = cid;
        }
    }
}

static void clip_plc (u8* plc, i32 dx, i32 dy, u16 w, u16 h, const u8* comp) {
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_land_ov(plc[i])) {
                continue;
            }
            const i32 nx = static_cast<i32>(px) + dx;
            const i32 ny = static_cast<i32>(py) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                plc[i] = k_water;
                continue;
            }
            const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (is_cont(comp[j])) {
                plc[i] = k_water;
            }
        }
    }
}

static void corner_anchor (u8 corner, u16 w, u16 h, i32 mx, i32 my, f32* ax, f32* ay) {
    if (ax == nullptr || ay == nullptr) {
        return;
    }
    const f32 hi_x = static_cast<f32>(w) - 1.f - static_cast<f32>(mx);
    const f32 hi_y = static_cast<f32>(h) - 1.f - static_cast<f32>(my);
    const f32 lo_x = static_cast<f32>(mx);
    const f32 lo_y = static_cast<f32>(my);
    switch (corner % 4u) {
        case 0u:
            *ax = lo_x;
            *ay = lo_y;
            break;
        case 1u:
            *ax = hi_x;
            *ay = lo_y;
            break;
        case 2u:
            *ax = hi_x;
            *ay = hi_y;
            break;
        default:
            *ax = lo_x;
            *ay = hi_y;
            break;
    }
}

static bool place_corner (
    u8 corner,
    u16 w,
    u16 h,
    i32 mx,
    i32 my,
    const OvBBox& bb,
    i32* dx,
    i32* dy) 
{
    if (dx == nullptr || dy == nullptr) {
        return false;
    }
    const i32 hi_x = static_cast<i32>(w) - 1 - mx;
    const i32 hi_y = static_cast<i32>(h) - 1 - my;
    switch (corner % 4u) {
        case 0u:
            *dx = mx - static_cast<i32>(bb.m_x0);
            *dy = my - static_cast<i32>(bb.m_y0);
            break;
        case 1u:
            *dx = hi_x - static_cast<i32>(bb.m_x1);
            *dy = my - static_cast<i32>(bb.m_y0);
            break;
        case 2u:
            *dx = hi_x - static_cast<i32>(bb.m_x1);
            *dy = hi_y - static_cast<i32>(bb.m_y1);
            break;
        default:
            *dx = mx - static_cast<i32>(bb.m_x0);
            *dy = hi_y - static_cast<i32>(bb.m_y1);
            break;
    }
    clamp_off(bb, w, h, mx, my, dx, dy);
    return true;
}

static i32 ray_len (
    const u8* comp,
    u16 w,
    u16 h,
    i32 mx,
    i32 my,
    f32 ox,
    f32 oy,
    f32 dir_x,
    f32 dir_y,
    u8 pass_id) 
{
    if (comp == nullptr || w == 0 || h == 0) {
        return 0;
    }
    const i32 hi_x = static_cast<i32>(w) - 1 - mx;
    const i32 hi_y = static_cast<i32>(h) - 1 - my;
    i32 len = 0;
    for (i32 s = 1; s < static_cast<i32>(w + h); ++s) {
        const i32 px = static_cast<i32>(std::lrint(static_cast<f64>(ox + dir_x * static_cast<f32>(s))));
        const i32 py = static_cast<i32>(std::lrint(static_cast<f64>(oy + dir_y * static_cast<f32>(s))));
        if (px < mx || py < my || px > hi_x || py > hi_y) {
            break;
        }
        const u32 j = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        const u8 v = comp[j];
        if (v != k_cont_water && v != pass_id) {
            break;
        }
        ++len;
    }
    return len;
}

static void ray_drag_dir (
    const u8* comp,
    u16 w,
    u16 h,
    i32 mx,
    i32 my,
    f32 ox,
    f32 oy,
    u8 pass_id,
    f32 pref_x,
    f32 pref_y,
    f32* dir_x,
    f32* dir_y) 
{
    if (dir_x == nullptr || dir_y == nullptr) {
        return;
    }
    i32 best_len = -1;
    f32 bx = pref_x;
    f32 by = pref_y;
    f32 best_pref = -1e9f;
    for (i32 k = 0; k < k_ray_n; ++k) {
        const f64 ang = k_pi * static_cast<f64>(k) / 6.0;
        const f32 vx = static_cast<f32>(std::cos(ang));
        const f32 vy = static_cast<f32>(std::sin(ang));
        const i32 len = ray_len(comp, w, h, mx, my, ox, oy, vx, vy, pass_id);
        const f32 pref = vx * pref_x + vy * pref_y;
        if (len > best_len || (len == best_len && pref > best_pref)) {
            best_len = len;
            best_pref = pref;
            bx = vx;
            by = vy;
        }
    }
    *dir_x = bx;
    *dir_y = by;
}

static void pref_drag_dir (
    u8 idx,
    u8 corner0,
    u8 slot,
    f32 mpx,
    f32 mpy,
    u16 w,
    u16 h,
    i32 mx,
    i32 my,
    f32* dir_x,
    f32* dir_y) 
{
    if (idx == 1u) {
        f32 ax = 0.f;
        f32 ay = 0.f;
        const u8 opp = static_cast<u8>((corner0 + 2u) % 4u);
        corner_anchor(opp, w, h, mx, my, &ax, &ay);
        norm_dir(ax - mpx, ay - mpy, dir_x, dir_y);
        return;
    }
    f32 vx = mpx - static_cast<f32>(w) * 0.5f;
    f32 vy = mpy - static_cast<f32>(h) * 0.5f;
    norm_dir(vx, vy, dir_x, dir_y);
    if ((slot & 1u) != 0u) {
        const f32 tx = *dir_x;
        *dir_x = -(*dir_y);
        *dir_y = tx;
    }
}

static bool rotate_plc_nn (u8* plc, u16 w, u16 h, i32 deg_cw) {
    if (plc == nullptr || w == 0 || h == 0) {
        return false;
    }
    MapArrayTerrain src;
    if (!src.assign_copy(w, h, plc)) {
        return false;
    }
    Generate_TerrainRotation rot;
    if (!rot.generate(src, deg_cw) || !rot.is_valid()) {
        return false;
    }
    const u8* out = rot.terrain().data();
    if (out == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memcpy(plc, out, static_cast<size_t>(n));
    return true;
}

static u8 parent_for (u8 idx) {
    if (idx == 1u) {
        return 0u;
    }
    return static_cast<u8>((idx - 2u) % 2u);
}

static bool gen_src_ov (
    const P1_RunPrm& prm,
    u16 size_pct,
    u8* dst,
    u16 w,
    u16 h) 
{
    u8 pct = size_pct > 255u ? 255u : static_cast<u8>(size_pct);
    if (pct == 0u) {
        pct = 1u;
    }
    P1_Gen_Outline ol(prm);
    if (!ol.generate(pct) || !ol.is_valid()) {
        return false;
    }
    const P1_Gen_OutlineRslt& r = ol.result();
    if (r.m_w != w || r.m_h != h || r.m_ov.data() == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memcpy(dst, r.m_ov.data(), static_cast<size_t>(n));
    return true;
}

static void drag_along (
    const u8* plc,
    OvBBox bb,
    i32* dx,
    i32* dy,
    u16 w,
    u16 h,
    i32 mx,
    i32 my,
    const u8* comp,
    u8 ovlp_id,
    u16* vis,
    u16* vis_tag,
    u16* qlo,
    u16* qhi,
    f32 dir_x,
    f32 dir_y,
    u32 tgt,
    i32* drag_n) 
{
    if (dx == nullptr || dy == nullptr || drag_n == nullptr) {
        return;
    }
    for (;;) {
        const u32 ov = ovlp_ff_cnt(plc, *dx, *dy, w, h, comp, ovlp_id, vis, vis_tag, qlo, qhi).m_ov;
        if (ov <= tgt) {
            return;
        }
        i32 ndx = *dx;
        i32 ndy = *dy;
        step_dir_n(dir_x, dir_y, k_drag_step, &ndx, &ndy);
        clamp_off(bb, w, h, mx, my, &ndx, &ndy);
        if (ndx == *dx && ndy == *dy) {
            break;
        }
        const u32 ov2 = ovlp_ff_cnt(plc, ndx, ndy, w, h, comp, ovlp_id, vis, vis_tag, qlo, qhi).m_ov;
        *dx = ndx;
        *dy = ndy;
        *drag_n += k_drag_step;
        if (ov2 <= tgt) {
            return;
        }
    }
    for (i32 step = 0; step < k_drag_step; ++step) {
        const u32 ov = ovlp_ff_cnt(plc, *dx, *dy, w, h, comp, ovlp_id, vis, vis_tag, qlo, qhi).m_ov;
        if (ov <= tgt) {
            return;
        }
        step_dir(dir_x, dir_y, dx, dy);
        clamp_off(bb, w, h, mx, my, dx, dy);
        ++(*drag_n);
    }
}

static void pull_along (
    OvBBox bb,
    i32* dx,
    i32* dy,
    u16 w,
    u16 h,
    i32 mx,
    i32 my,
    f32 dir_x,
    f32 dir_y,
    i32 pull_n) 
{
    if (dx == nullptr || dy == nullptr || pull_n <= 0) {
        return;
    }
    i32 left = pull_n;
    while (left > 0) {
        const i32 step = left >= k_drag_step ? k_drag_step : left;
        step_dir_n(dir_x, dir_y, step, dx, dy);
        clamp_off(bb, w, h, mx, my, dx, dy);
        left -= step;
    }
}

static bool stack_drag_cut_drag_rotate (
    u8 idx,
    u8 pid,
    u8 corner0,
    u16 w,
    u16 h,
    i32 mx,
    i32 my,
    u8 cut_pct,
    u16 pull_pct,
    const u8* src,
    OvBBox bb,
    const u8* comp,
    i32 pdx,
    i32 pdy,
    const u8* par_src,
    u8* plc,
    i32* dx,
    i32* dy,
    u16* vis,
    u16* vis_tag,
    u16* qlo,
    u16* qhi,
    Mt19937* rng) 
{
    if (dx == nullptr || dy == nullptr || plc == nullptr || rng == nullptr
        || vis == nullptr || vis_tag == nullptr || qlo == nullptr || qhi == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memcpy(plc, src, static_cast<size_t>(n));
    const u8 par_id = cont_id(pid);
    const u32 land_n = ovlp_ff_cnt(plc, 0, 0, w, h, nullptr, 0u, vis, vis_tag, qlo, qhi).m_land;
    if (land_n == 0u) {
        return false;
    }
    f32 mcx = 0.f;
    f32 mcy = 0.f;
    f32 ocx = 0.f;
    f32 ocy = 0.f;
    land_cen(par_src, w, h, &mcx, &mcy);
    land_cen(src, w, h, &ocx, &ocy);
    const f32 mpx = mcx + static_cast<f32>(pdx);
    const f32 mpy = mcy + static_cast<f32>(pdy);
    const u32 tgt = (land_n * static_cast<u32>(cut_pct) + 50u) / 100u;
    *dx = static_cast<i32>(std::lrint(static_cast<f64>(mpx - ocx)));
    *dy = static_cast<i32>(std::lrint(static_cast<f64>(mpy - ocy)));
    clamp_off(bb, w, h, mx, my, dx, dy);
    f32 pref_x = 0.f;
    f32 pref_y = 0.f;
    pref_drag_dir(idx, corner0, static_cast<u8>(idx - 2u), mpx, mpy, w, h, mx, my, &pref_x, &pref_y);
    f32 dir_x = 0.f;
    f32 dir_y = 0.f;
    ray_drag_dir(comp, w, h, mx, my, mpx, mpy, par_id, pref_x, pref_y, &dir_x, &dir_y);
    i32 drag_n = 0;
    drag_along(plc, bb, dx, dy, w, h, mx, my, comp, par_id, vis, vis_tag, qlo, qhi, dir_x, dir_y, tgt, &drag_n);
    clip_plc(plc, *dx, *dy, w, h, comp);
    bb = bbox_land(plc, w, h);
    if (!bb.m_ok) {
        return false;
    }
    i32 pull_n = 0;
    if (pull_pct > 0u && drag_n > 0) {
        pull_n = static_cast<i32>((static_cast<u32>(drag_n) * static_cast<u32>(pull_pct) + 50u) / 100u);
    }
    pull_along(bb, dx, dy, w, h, mx, my, dir_x, dir_y, pull_n);
    const f64 rot = rng_rot_deg(rng);
    const i32 deg_cw = static_cast<i32>(std::lrint(rot));
    if (!rotate_plc_nn(plc, w, h, deg_cw)) {
        return false;
    }
    bb = bbox_land(plc, w, h);
    if (!bb.m_ok) {
        return false;
    }
    clamp_off(bb, w, h, mx, my, dx, dy);
    return true;
}

static void wb_rel (u16*& p) {
    if (p != nullptr) {
        Whiteboard::release(p);
        p = nullptr;
    }
}

//================================================================================================================================
//=> - P1_Gen_ContOutlines -
//================================================================================================================================

P1_Gen_ContOutlines::P1_Gen_ContOutlines (
    const P1_RunPrm& prm,
    const P1_Gen_ContOutlinesPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_n = 0u;
}

bool P1_Gen_ContOutlines::generate () {
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    if (!p1_run_prm_ok(m_prm) || m_sp.m_sz.m_n == 0u || m_sp.m_sz.m_n > P1_CONT_PCT_MAX) {
        return false;
    }
    u8 main_n = m_sp.m_main_n;
    if (main_n == 0u) {
        main_n = 1u;
    }
    if (main_n < 2u && m_sp.m_sz.m_n > 1u) {
        main_n = 2u;
    }
    const u16 w = m_prm.m_w;
    const u16 h = m_prm.m_h;
    i32 mx = 0;
    i32 my = 0;
    map_margins(w, h, m_sp.m_margin, &mx, &my);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const i32 tile_n = static_cast<i32>(n);
    u16* wb_cur = Whiteboard::alloc(tile_n);
    u16* wb_p0 = Whiteboard::alloc(tile_n);
    u16* wb_p1 = Whiteboard::alloc(tile_n);
    u16* wb_vis = Whiteboard::alloc(tile_n);
    u16* wb_qlo = Whiteboard::alloc(tile_n);
    u16* wb_qhi = Whiteboard::alloc(tile_n);
    if (wb_cur == nullptr || wb_p0 == nullptr || wb_p1 == nullptr
        || wb_vis == nullptr || wb_qlo == nullptr || wb_qhi == nullptr) {
        wb_rel(wb_qhi);
        wb_rel(wb_qlo);
        wb_rel(wb_vis);
        wb_rel(wb_p1);
        wb_rel(wb_p0);
        wb_rel(wb_cur);
        return false;
    }
    u8* cur = wb_u8(wb_cur);
    u8* p0 = wb_u8(wb_p0);
    u8* p1 = wb_u8(wb_p1);
    std::memset(wb_vis, 0, static_cast<size_t>(n) * sizeof(u16));
    u16 vis_tag = 1u;
    Mt19937 rng = {};
    mt_seed(&rng, m_prm.m_seed + 7919u);
    const u8 corner0 = static_cast<u8>(mt_next(&rng) % 4u);
    if (!m_rslt.m_ov.resize(w, h)) {
        wb_rel(wb_qhi);
        wb_rel(wb_qlo);
        wb_rel(wb_vis);
        wb_rel(wb_p1);
        wb_rel(wb_p0);
        wb_rel(wb_cur);
        return false;
    }
    u8* comp = m_rslt.m_ov.data_w();
    for (u32 i = 0; i < n; ++i) {
        comp[i] = k_cont_water;
    }
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_rslt.m_n = m_sp.m_sz.m_n;
    i32 off_x[P1_CONT_PCT_MAX];
    i32 off_y[P1_CONT_PCT_MAX];
    for (u8 i = 0; i < m_sp.m_sz.m_n; ++i) {
        off_x[i] = 0;
        off_y[i] = 0;
        m_rslt.m_ent[i] = m_sp.m_sz.m_ent[i];
    }
    P1_RunPrm prm0 = m_prm;
    if (!gen_src_ov(prm0, m_sp.m_sz.m_ent[0].m_pct, cur, w, h)) {
        wb_rel(wb_qhi);
        wb_rel(wb_qlo);
        wb_rel(wb_vis);
        wb_rel(wb_p1);
        wb_rel(wb_p0);
        wb_rel(wb_cur);
        m_rslt.m_ov.clear();
        return false;
    }
    std::memcpy(p0, cur, static_cast<size_t>(n));
    OvBBox bb = bbox_land(cur, w, h);
    if (!bb.m_ok || !place_corner(corner0, w, h, mx, my, bb, &off_x[0], &off_y[0])) {
        wb_rel(wb_qhi);
        wb_rel(wb_qlo);
        wb_rel(wb_vis);
        wb_rel(wb_p1);
        wb_rel(wb_p0);
        wb_rel(wb_cur);
        m_rslt.m_ov.clear();
        return false;
    }
    blit_cont(cur, off_x[0], off_y[0], w, h, cont_id(0u), comp);
    for (u8 i = 1; i < m_sp.m_sz.m_n; ++i) {
        const u8 pid = parent_for(i);
        P1_RunPrm prm_i = m_prm;
        prm_i.m_seed = m_prm.m_seed + static_cast<u32>(i) * 1000u;
        if (!gen_src_ov(prm_i, m_sp.m_sz.m_ent[i].m_pct, cur, w, h)) {
            wb_rel(wb_qhi);
            wb_rel(wb_qlo);
            wb_rel(wb_vis);
            wb_rel(wb_p1);
            wb_rel(wb_p0);
            wb_rel(wb_cur);
            m_rslt.m_ov.clear();
            return false;
        }
        if (i == 1u) {
            std::memcpy(p1, cur, static_cast<size_t>(n));
        }
        bb = bbox_land(cur, w, h);
        if (!bb.m_ok) {
            wb_rel(wb_qhi);
            wb_rel(wb_qlo);
            wb_rel(wb_vis);
            wb_rel(wb_p1);
            wb_rel(wb_p0);
            wb_rel(wb_cur);
            m_rslt.m_ov.clear();
            return false;
        }
        Mt19937 rng_i = {};
        mt_seed(&rng_i, m_prm.m_seed + static_cast<u32>(i) * 1000u + 313u);
        const u8* par_src = pid == 0u ? p0 : p1;
        if (!stack_drag_cut_drag_rotate(
                i,
                pid,
                corner0,
                w,
                h,
                mx,
                my,
                m_sp.m_ovlp_pct,
                m_sp.m_sz.m_ent[i].m_pull_pct,
                cur,
                bb,
                comp,
                off_x[pid],
                off_y[pid],
                par_src,
                cur,
                &off_x[i],
                &off_y[i],
                wb_vis,
                &vis_tag,
                wb_qlo,
                wb_qhi,
                &rng_i)) {
            wb_rel(wb_qhi);
            wb_rel(wb_qlo);
            wb_rel(wb_vis);
            wb_rel(wb_p1);
            wb_rel(wb_p0);
            wb_rel(wb_cur);
            m_rslt.m_ov.clear();
            return false;
        } 
        blit_cont(cur, off_x[i], off_y[i], w, h, cont_id(i), comp);
    }
    wb_rel(wb_qhi);
    wb_rel(wb_qlo);
    wb_rel(wb_vis);
    wb_rel(wb_p1);
    wb_rel(wb_p0);
    wb_rel(wb_cur);
    if (Whiteboard::chkout() == 0u) {
        Whiteboard::dealloc();
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_ContOutlines::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_ContOutlinesRslt& P1_Gen_ContOutlines::result () const {
    return m_rslt;
}

void p1_cont_ov_to_gray (const u8* comp, u8* dst, u16 w, u16 h) {
    if (comp == nullptr || dst == nullptr || w == 0 || h == 0) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        dst[i] = comp[i] == k_cont_water ? k_water : k_land;
    }
}

bool p1_gen_step01_ov (
    const P1_RunPrm& prm,
    MapArrayOverlay* ov_gray,
    const P1_Gen_ContOutlinesPrm& sp) 
{
    if (ov_gray == nullptr) {
        return false;
    }
    P1_Gen_ContOutlines gen(prm, sp);
    if (!gen.generate() || !gen.is_valid()) {
        return false;
    }
    const P1_Gen_ContOutlinesRslt& r = gen.result();
    const u8* comp = r.m_ov.data();
    if (comp == nullptr || r.m_w == 0 || r.m_h == 0) {
        return false;
    }
    if (!ov_gray->resize(r.m_w, r.m_h)) {
        return false;
    }
    u8* dst = ov_gray->data_w();
    if (dst == nullptr) {
        return false;
    }
    p1_cont_ov_to_gray(comp, dst, r.m_w, r.m_h);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
