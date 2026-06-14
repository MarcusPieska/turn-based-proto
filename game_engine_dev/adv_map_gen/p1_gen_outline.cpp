//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_outline.h"

#include <cmath>

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u8 k_ov_water = WL_OVERLAY_WATER_GRAY;
static const u8 k_ov_land = WL_OVERLAY_LAND_GRAY;
static const u8 k_ov_edge = 1;
static const f64 k_pi = 3.14159265358979323846;
static const i32 k_max_pt = 400;

struct OutlineShapeCtx {
    u8* m_ov;
    u16 m_w;
    u16 m_h;
    f64 m_cx;
    f64 m_cy;
    f64 m_stretch_x;
    f64 m_stretch_y;
};

struct PtF64 {
    f64 x;
    f64 y;
};

struct Mt19937 {
    u32 mt[624];
    i32 idx;
};

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

static void set_ov_px (u8* ov, u16 w, u16 h, i32 x, i32 y, u8 c) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    ov[static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)] = c;
}

static void draw_seg_ov (u8* ov, u16 w, u16 h, i32 x1, i32 y1, i32 x2, i32 y2, u8 c) {
    const i32 dx = x1 < x2 ? x2 - x1 : x1 - x2;
    const i32 dy = y1 < y2 ? y2 - y1 : y1 - y2;
    const i32 sx = x1 < x2 ? 1 : -1;
    const i32 sy = y1 < y2 ? 1 : -1;
    i32 err = dx - dy;
    i32 x = x1;
    i32 y = y1;
    for (;;) {
        set_ov_px(ov, w, h, x, y, c);
        if (x == x2 && y == y2) {
            break;
        }
        const i32 e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

static void draw_bent_edge_ov (
    u8* ov,
    u16 w,
    u16 h,
    f64 x1,
    f64 y1,
    f64 x2,
    f64 y2,
    f64 zoom,
    Mt19937* rng) {
    const f64 dx = x2 - x1;
    const f64 dy = y2 - y1;
    const f64 length = std::sqrt(dx * dx + dy * dy) / zoom;
    if (length < 1e-6) {
        return;
    }
    const f64 angle = std::atan2(-dy, dx) * 180.0 / k_pi;
    const f64 bend = rng01(rng);
    const f64 peak = 0.2 + rng01(rng) * 0.6;
    const i32 num_raw = static_cast<i32>(length * zoom);
    const i32 num = num_raw < 50 ? 50 : num_raw;
    const f64 max_bend = length * 0.3 * bend;
    const f64 p1x = length * zoom * peak;
    const f64 p1y = max_bend * zoom;
    const f64 p2x = length * zoom;
    const f64 ang = angle * k_pi / 180.0;
    const f64 ca = std::cos(ang);
    const f64 sa = std::sin(ang);
    const f64 cx = static_cast<f64>(w) * 0.5;
    const f64 cy = static_cast<f64>(h) * 0.5;
    const i32 ox = static_cast<i32>(x1) - static_cast<i32>(w) / 2;
    const i32 oy = static_cast<i32>(y1) - static_cast<i32>(h) / 2;
    i32 px0 = 0;
    i32 py0 = 0;
    bool has0 = false;
    for (i32 i = 0; i <= num; ++i) {
        const f64 t = static_cast<f64>(i) / static_cast<f64>(num);
        const f64 om = 1.0 - t;
        const f64 xl = om * om * 0.0 + 2.0 * om * t * p1x + t * t * p2x;
        const f64 yl = om * om * 0.0 + 2.0 * om * t * p1y + t * t * 0.0;
        const f64 xr = xl * ca - yl * sa;
        const f64 yr = xl * sa + yl * ca;
        const i32 px = static_cast<i32>(cx + xr) + ox;
        const i32 py = static_cast<i32>(cy - yr) + oy;
        if (has0) {
            draw_seg_ov(ov, w, h, px0, py0, px, py, k_ov_edge);
        }
        px0 = px;
        py0 = py;
        has0 = true;
    }
}

static void remove_edge_ov (u8* ov, u32 npx) {
    for (u32 i = 0; i < npx; ++i) {
        if (ov[i] == k_ov_edge) {
            ov[i] = k_ov_water;
        }
    }
}

static bool flood_fill_ov (u8* ov, u16 w, u16 h, i32 sx, i32 sy) {
    if (sx < 0 || sy < 0 || sx >= static_cast<i32>(w) || sy >= static_cast<i32>(h)) {
        return false;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 si = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
    if (ov[si] == k_ov_edge || ov[si] == k_ov_land) {
        return true;
    }
    u32* st = new u32[npx];
    if (st == nullptr) {
        return false;
    }
    u32 sp = 0;
    ov[si] = k_ov_land;
    st[sp++] = si;
    while (sp > 0) {
        const u32 idx = st[--sp];
        const u32 x = idx % static_cast<u32>(w);
        const u32 y = idx / static_cast<u32>(w);
        if (y > 0) {
            const u32 ni = (y - 1) * static_cast<u32>(w) + x;
            if (ov[ni] == k_ov_water && sp < npx) {
                ov[ni] = k_ov_land;
                st[sp++] = ni;
            }
        }
        if (y + 1 < static_cast<u32>(h)) {
            const u32 ni = (y + 1) * static_cast<u32>(w) + x;
            if (ov[ni] == k_ov_water && sp < npx) {
                ov[ni] = k_ov_land;
                st[sp++] = ni;
            }
        }
        if (x > 0) {
            const u32 ni = y * static_cast<u32>(w) + (x - 1);
            if (ov[ni] == k_ov_water && sp < npx) {
                ov[ni] = k_ov_land;
                st[sp++] = ni;
            }
        }
        if (x + 1 < static_cast<u32>(w)) {
            const u32 ni = y * static_cast<u32>(w) + (x + 1);
            if (ov[ni] == k_ov_water && sp < npx) {
                ov[ni] = k_ov_land;
                st[sp++] = ni;
            }
        }
    }
    delete[] st;
    return true;
}

static bool build_outline_shape (OutlineShapeCtx& ctx, u32 seed) {
    Mt19937 rng;
    mt_seed(&rng, seed);
    const f64 min_ang = 1.0;
    const f64 max_ang = 30.0;
    const f64 scale = 1.0;
    const f64 min_depth = 0.3 * scale;
    const f64 max_depth = 0.9 * scale;
    const f64 zoom = 1.0;
    f64 angles[k_max_pt];
    f64 depths[k_max_pt];
    PtF64 points[k_max_pt];
    i32 pt_n = 0;
    f64 cur_ang = 0.0;
    while (cur_ang < 360.0 && pt_n < k_max_pt) {
        const f64 step = min_ang + rng01(&rng) * (max_ang - min_ang);
        cur_ang += step;
        if (cur_ang >= 360.0) {
            break;
        }
        angles[pt_n] = cur_ang;
        depths[pt_n] = min_depth + rng01(&rng) * (max_depth - min_depth);
        pt_n++;
    }
    if (pt_n < 3) {
        return false;
    }
    ctx.m_cx = static_cast<f64>(ctx.m_w) * 0.5;
    ctx.m_cy = static_cast<f64>(ctx.m_h) * 0.5;
    const f64 max_r = ctx.m_cx < ctx.m_cy ? ctx.m_cx * 0.9 : ctx.m_cy * 0.9;
    for (i32 i = 0; i < pt_n; ++i) {
        const f64 rad = max_r * depths[i];
        const f64 ar = angles[i] * k_pi / 180.0;
        f64 px = ctx.m_cx + rad * std::cos(ar);
        f64 py = ctx.m_cy - rad * std::sin(ar);
        px = ctx.m_cx + (px - ctx.m_cx) * ctx.m_stretch_x;
        py = ctx.m_cy + (py - ctx.m_cy) * ctx.m_stretch_y;
        points[i].x = px;
        points[i].y = py;
    }
    for (i32 i = 0; i < pt_n; ++i) {
        const f64 ax = points[i].x * zoom;
        const f64 ay = points[i].y * zoom;
        const f64 bx = points[(i + 1) % pt_n].x * zoom;
        const f64 by = points[(i + 1) % pt_n].y * zoom;
        draw_bent_edge_ov(ctx.m_ov, ctx.m_w, ctx.m_h, ax, ay, bx, by, zoom, &rng);
    }
    return true;
}

static bool fill_outline_overlay (OutlineShapeCtx& ctx, u32 seed) {
    const u32 npx = static_cast<u32>(ctx.m_w) * static_cast<u32>(ctx.m_h);
    for (u32 i = 0; i < npx; ++i) {
        ctx.m_ov[i] = k_ov_water;
    }
    if (!build_outline_shape(ctx, seed)) {
        return false;
    }
    if (!flood_fill_ov(ctx.m_ov, ctx.m_w, ctx.m_h, static_cast<i32>(ctx.m_cx), static_cast<i32>(ctx.m_cy))) {
        return false;
    }
    remove_edge_ov(ctx.m_ov, npx);
    return true;
}

//================================================================================================================================
//=> - P1_Gen_Outline -
//================================================================================================================================

P1_Gen_Outline::P1_Gen_Outline (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_stretch_x = 1.f;
    m_rslt.m_stretch_y = 1.f;
}

bool P1_Gen_Outline::generate () {
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    if (!p1_run_prm_ok(m_prm)) {
        return false;
    }
    m_rslt.m_w = m_prm.m_w;
    m_rslt.m_h = m_prm.m_h;
    m_rslt.m_stretch_x = 1.f;
    m_rslt.m_stretch_y = 1.f;
    if (!m_rslt.m_ov.resize(m_rslt.m_w, m_rslt.m_h)) {
        return false;
    }
    OutlineShapeCtx ctx;
    ctx.m_ov = m_rslt.m_ov.data_w();
    ctx.m_w = m_rslt.m_w;
    ctx.m_h = m_rslt.m_h;
    ctx.m_cx = 0.0;
    ctx.m_cy = 0.0;
    ctx.m_stretch_x = static_cast<f64>(m_rslt.m_stretch_x);
    ctx.m_stretch_y = static_cast<f64>(m_rslt.m_stretch_y);
    if (!fill_outline_overlay(ctx, m_prm.m_seed)) {
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_Outline::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_OutlineRslt& P1_Gen_Outline::result () const {
    return m_rslt;
}

void P1_Gen_Outline::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return;
    }
    m_rslt.m_ov.save(path);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
