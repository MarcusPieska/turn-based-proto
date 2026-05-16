//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>
#include <vector>

#include "generate_distance_land_to_water.h"
#include "generate_overlay_water_land.h"
#include "generate_terrain_cont_outline.h"
#include "perlin_noise.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u8 k_edge = TERR_COASTAL[0];
static const u8 k_land = TERR_PLAINS[0];
static const u8 k_sea = TERR_OCEAN[0];
static const u16 k_dist_inf = 0xFFFFu;

static inline bool wl_is_water (const u8* wl, u32 i) {
    return wl[i] == WL_OVERLAY_WATER_GRAY;
}

static u32 mix_layer_seed (u32 base, i32 k) {
    return base ^ (static_cast<u32>(k + 1) * 2654435769u);
}

static size_t frac_idx (u32 n, f64 pop, f64 cap) {
    if (n == 0u) {
        return 0u;
    }
    if (pop <= 0.0) {
        return 0u;
    }
    if (pop >= cap) {
        return static_cast<size_t>(n - 1u);
    }
    const size_t j = static_cast<size_t>(std::floor(pop * static_cast<f64>(n)));
    return j >= static_cast<size_t>(n) ? static_cast<size_t>(n - 1u) : j;
}

static u8 terr_cls (f64 v, const f64 thr[5]) {
    if (v <= thr[0]) {
        return TERR_OCEAN[0];
    }
    if (v <= thr[1]) {
        return TERR_SEA[0];
    }
    if (v <= thr[2]) {
        return TERR_COASTAL[0];
    }
    if (v <= thr[3]) {
        return TERR_PLAINS[0];
    }
    if (v <= thr[4]) {
        return TERR_HILLS[0];
    }
    return TERR_MOUNTAINS[0];
}

static f32 land_dist_norm (u16 d, u16 d_max) {
    if (d == k_dist_inf) {
        return 0.f;
    }
    return d_max > 0 ? static_cast<f32>(d) / static_cast<f32>(d_max) : 0.f;
}

static f32 land_dist_sub (u16 d, u16 d_max, f32 lim) {
    if (d == k_dist_inf) {
        return 1.f;
    }
    const f32 t = 1.f - land_dist_norm(d, d_max);
    if (t <= lim) {
        return 0.f;
    }
    if (t >= 1.f) {
        return 1.f;
    }
    if (lim < 1.f - 1e-12f) {
        return (t - lim) / (1.f - lim);
    }
    return 1.f;
}

static void build_interior_mask_u8 (const u16* l2w, u32 n, u16 d_max, u8* out) {
    for (u32 i = 0; i < n; ++i) {
        f32 k = land_dist_norm(l2w[i], d_max);
        if (k < 0.f) {
            k = 0.f;
        }
        if (k > 1.f) {
            k = 1.f;
        }
        out[i] = static_cast<u8>(std::lrint(k * 255.f));
    }
}

static bool build_pn_terrain_land_dist (
    const TerrainContPnParams& p,
    const u16* l2w,
    u16 w,
    u16 h,
    std::vector<u8>& out_cls) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0 || l2w == nullptr) {
        return false;
    }
    u16 d_max = 0;
    for (u32 i = 0; i < n; ++i) {
        const u16 d = l2w[i];
        if (d != k_dist_inf && d > d_max) {
            d_max = d;
        }
    }
    const i32 lc = p.m_layer_count < 1 ? 1 : p.m_layer_count;
    f32* combo = new f32[n];
    for (u32 i = 0; i < n; ++i) {
        combo[i] = 0.f;
    }
    PerlinImgParams lp;
    lp.m_w = w;
    lp.m_h = h;
    lp.m_lacunarity = p.m_lacunarity;
    f32 freq = p.m_layer_freq_base;
    for (int k = 0; k < lc; ++k) {
        lp.m_frequency = freq;
        if (!accumulate_perlin_field_f32(combo, p.m_layer_weight, lp, mix_layer_seed(p.m_seed, k))) {
            delete[] combo;
            return false;
        }
        freq *= p.m_layer_freq_step;
    }
    const f32 lim = p.m_inner_grad_limit;
    f32 vmin = 0.f;
    f32 vmax = 0.f;
    bool first = true;
    for (u32 i = 0; i < n; ++i) {
        combo[i] -= land_dist_sub(l2w[i], d_max, lim);
        const f32 v = combo[i];
        if (first) {
            vmin = vmax = v;
            first = false;
        } else {
            if (v < vmin) {
                vmin = v;
            }
            if (v > vmax) {
                vmax = v;
            }
        }
    }
    f32 denom = vmax - vmin;
    if (denom < 1e-12f) {
        denom = 1.f;
    }
    std::vector<f32> norm(static_cast<size_t>(n));
    for (u32 i = 0; i < n; ++i) {
        norm[i] = (combo[i] - vmin) / denom;
    }
    delete[] combo;
    const f64 cap = p.m_terr_lim_mountains;
    const size_t j0 = frac_idx(n, p.m_terr_lim_ocean, cap);
    const size_t j1 = frac_idx(n, p.m_terr_lim_sea, cap);
    const size_t j2 = frac_idx(n, p.m_terr_lim_coastal, cap);
    const size_t j3 = frac_idx(n, p.m_terr_lim_plains, cap);
    const size_t j4 = frac_idx(n, p.m_terr_lim_hills, cap);
    std::vector<f32> work(norm);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j0), work.end());
    f64 thr[5];
    thr[0] = static_cast<f64>(work[j0]);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j1), work.end());
    thr[1] = static_cast<f64>(work[j1]);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j2), work.end());
    thr[2] = static_cast<f64>(work[j2]);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j3), work.end());
    thr[3] = static_cast<f64>(work[j3]);
    std::nth_element(work.begin(), work.begin() + static_cast<std::ptrdiff_t>(j4), work.end());
    thr[4] = static_cast<f64>(work[j4]);
    out_cls.resize(static_cast<size_t>(n));
    for (u32 i = 0; i < n; ++i) {
        f64 vd = static_cast<f64>(norm[i]);
        if (vd < 0.0) {
            vd = 0.0;
        }
        if (vd > 1.0) {
            vd = 1.0;
        }
        out_cls[i] = terr_cls(vd, thr);
    }
    return true;
}

struct OutlineShapeCtx {
    u8* m_ter;
    u16 m_w;
    u16 m_h;
    f64 m_cx;
    f64 m_cy;
};

static void set_ter_px (u8* ter, u16 w, u16 h, i32 x, i32 y, u8 c) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    ter[static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)] = c;
}

static void draw_seg_ter (u8* ter, u16 w, u16 h, i32 x1, i32 y1, i32 x2, i32 y2, u8 c) {
    const i32 dx = std::abs(x2 - x1);
    const i32 dy = std::abs(y2 - y1);
    const i32 sx = x1 < x2 ? 1 : -1;
    const i32 sy = y1 < y2 ? 1 : -1;
    i32 err = dx - dy;
    i32 x = x1;
    i32 y = y1;
    for (;;) {
        set_ter_px(ter, w, h, x, y, c);
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

static void draw_bent_edge_ter (
    u8* ter,
    u16 w,
    u16 h,
    f64 x1,
    f64 y1,
    f64 x2,
    f64 y2,
    f64 zoom,
    std::mt19937& rng,
    std::uniform_real_distribution<f64>& dist01) {
    const f64 dx = x2 - x1;
    const f64 dy = y2 - y1;
    const f64 length = std::sqrt(dx * dx + dy * dy) / zoom;
    if (length < 1e-6) {
        return;
    }
    const f64 angle = std::atan2(-dy, dx) * 180.0 / 3.14159265358979323846;
    const f64 bend = dist01(rng);
    const f64 peak = 0.2 + dist01(rng) * 0.6;
    const i32 num = std::max(50, static_cast<i32>(length * zoom));
    const f64 max_bend = length * 0.3 * bend;
    const f64 p1x = length * zoom * peak;
    const f64 p1y = max_bend * zoom;
    const f64 p2x = length * zoom;
    const f64 ang = angle * 3.14159265358979323846 / 180.0;
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
            draw_seg_ter(ter, w, h, px0, py0, px, py, k_edge);
        }
        px0 = px;
        py0 = py;
        has0 = true;
    }
}

static void flood_fill_ter (u8* ter, u16 w, u16 h, i32 sx, i32 sy) {
    if (sx < 0 || sy < 0 || sx >= static_cast<i32>(w) || sy >= static_cast<i32>(h)) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 si = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
    if (ter[si] == k_edge || ter[si] == k_land) {
        return;
    }
    std::vector<u8> vis(n, 0);
    std::vector<u32> st;
    st.push_back(si);
    vis[si] = 1;
    while (!st.empty()) {
        const u32 idx = st.back();
        st.pop_back();
        const u32 x = idx % static_cast<u32>(w);
        const u32 y = idx / static_cast<u32>(w);
        if (ter[idx] == k_edge || ter[idx] == k_land) {
            continue;
        }
        ter[idx] = k_land;
        if (y > 0) {
            const u32 ni = (y - 1) * static_cast<u32>(w) + x;
            if (!vis[ni]) {
                vis[ni] = 1;
                st.push_back(ni);
            }
        }
        if (y + 1 < static_cast<u32>(h)) {
            const u32 ni = (y + 1) * static_cast<u32>(w) + x;
            if (!vis[ni]) {
                vis[ni] = 1;
                st.push_back(ni);
            }
        }
        if (x > 0) {
            const u32 ni = y * static_cast<u32>(w) + (x - 1);
            if (!vis[ni]) {
                vis[ni] = 1;
                st.push_back(ni);
            }
        }
        if (x + 1 < static_cast<u32>(w)) {
            const u32 ni = y * static_cast<u32>(w) + (x + 1);
            if (!vis[ni]) {
                vis[ni] = 1;
                st.push_back(ni);
            }
        }
    }
}

static void remove_edge_ter (u8* ter, u32 npx) {
    for (u32 i = 0; i < npx; ++i) {
        if (ter[i] == k_edge) {
            ter[i] = k_sea;
        }
    }
}

static bool build_outline_shape (OutlineShapeCtx& ctx, u32 seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<f64> dist01(0.0, 1.0);
    const f64 min_ang = 1.0;
    const f64 max_ang = 30.0;
    const f64 scale = 1.0;
    const f64 min_depth = 0.3 * scale;
    const f64 max_depth = 0.9 * scale;
    const f64 zoom = 1.0;
    std::vector<f64> angles;
    std::vector<f64> depths;
    std::vector<std::pair<f64, f64>> points;
    f64 cur_ang = 0.0;
    while (cur_ang < 360.0) {
        const f64 step = min_ang + dist01(rng) * (max_ang - min_ang);
        cur_ang += step;
        if (cur_ang >= 360.0) {
            break;
        }
        angles.push_back(cur_ang);
    }
    if (angles.size() < 3) {
        return false;
    }
    depths.reserve(angles.size());
    for (size_t i = 0; i < angles.size(); ++i) {
        depths.push_back(min_depth + dist01(rng) * (max_depth - min_depth));
    }
    ctx.m_cx = static_cast<f64>(ctx.m_w) * 0.5;
    ctx.m_cy = static_cast<f64>(ctx.m_h) * 0.5;
    const f64 max_r = std::min(ctx.m_cx, ctx.m_cy) * 0.9;
    points.reserve(angles.size());
    for (size_t i = 0; i < angles.size(); ++i) {
        const f64 rad = max_r * depths[i];
        const f64 ar = angles[i] * 3.14159265358979323846 / 180.0;
        points.push_back(std::pair<f64, f64>(ctx.m_cx + rad * std::cos(ar), ctx.m_cy - rad * std::sin(ar)));
    }
    const size_t np = points.size();
    for (size_t i = 0; i < np; ++i) {
        const f64 ax = points[i].first * zoom;
        const f64 ay = points[i].second * zoom;
        const f64 bx = points[(i + 1) % np].first * zoom;
        const f64 by = points[(i + 1) % np].second * zoom;
        draw_bent_edge_ter(ctx.m_ter, ctx.m_w, ctx.m_h, ax, ay, bx, by, zoom, rng, dist01);
    }
    return true;
}

static void fill_mode_full (OutlineShapeCtx& ctx) {
    const u32 npx = static_cast<u32>(ctx.m_w) * static_cast<u32>(ctx.m_h);
    flood_fill_ter(ctx.m_ter, ctx.m_w, ctx.m_h, static_cast<i32>(ctx.m_cx), static_cast<i32>(ctx.m_cy));
    remove_edge_ter(ctx.m_ter, npx);
}

static bool fill_mode_perlin_noise (
    OutlineShapeCtx& ctx,
    const TerrainContOutlineParams& op,
    MapArrayTerrain& ter,
    MapArrayDistance& out_l2w,
    std::vector<u8>& out_mask) {
    fill_mode_full(ctx);
    Generate_OverlayWaterLand wl;
    if (!wl.generate(ter) || !wl.is_valid()) {
        return false;
    }
    const u8* wg = wl.water_land_gray();
    if (wg == nullptr) {
        return false;
    }
    Generate_DistanceLandToWater dist(0);
    if (!dist.generate(wg, ter.width(), ter.height())) {
        return false;
    }
    if (!out_l2w.assign_copy(ter.width(), ter.height(), dist.distance().data())) {
        return false;
    }
    const u16* l2w = out_l2w.data();
    const u32 npx = static_cast<u32>(ctx.m_w) * static_cast<u32>(ctx.m_h);
    u16 d_max = 0;
    for (u32 i = 0; i < npx; ++i) {
        const u16 d = l2w[i];
        if (d != k_dist_inf && d > d_max) {
            d_max = d;
        }
    }
    out_mask.resize(static_cast<size_t>(npx));
    build_interior_mask_u8(l2w, npx, d_max, out_mask.data());
    TerrainContPnParams pp = op.m_pn_params;
    pp.m_seed = op.m_seed;
    pp.m_width = ctx.m_w;
    pp.m_height = ctx.m_h;
    std::vector<u8> pn_cls;
    if (!build_pn_terrain_land_dist(pp, l2w, ctx.m_w, ctx.m_h, pn_cls)) {
        return false;
    }
    u8* dst = ter.data_w();
    for (u32 i = 0; i < npx; ++i) {
        if (wl_is_water(wg, i)) {
            dst[i] = k_sea;
        } else {
            dst[i] = pn_cls[i];
        }
    }
    return true;
}

static void fill_mode_pn_mix (OutlineShapeCtx& ctx) {
    fill_mode_full(ctx);
}

//================================================================================================================================
//=> - TerrainContOutline -
//================================================================================================================================

Generate_TerrainContOutline::Generate_TerrainContOutline (const TerrainContOutlineParams& params) :
    m_params(params),
    m_valid_generation(false),
    m_has_land_to_water(false),
    m_terrain(),
    m_land_to_water(),
    m_interior_mask() {
}

Generate_TerrainContOutline::Generate_TerrainContOutline (u32 seed, u16 w, u16 h) :
    m_params(),
    m_valid_generation(false),
    m_has_land_to_water(false),
    m_terrain(),
    m_land_to_water(),
    m_interior_mask() {
    m_params.m_seed = seed;
    m_params.m_width = w;
    m_params.m_height = h;
}

bool Generate_TerrainContOutline::generate () {
    m_valid_generation = false;
    m_has_land_to_water = false;
    m_terrain.clear();
    m_land_to_water.clear();
    m_interior_mask.clear();
    const u16 w = m_params.m_width;
    const u16 h = m_params.m_height;
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<u8> ocean(static_cast<size_t>(npx), k_sea);
    if (!m_terrain.assign_copy(w, h, ocean.data())) {
        return false;
    }
    OutlineShapeCtx ctx;
    ctx.m_ter = m_terrain.data_w();
    ctx.m_w = w;
    ctx.m_h = h;
    ctx.m_cx = 0.0;
    ctx.m_cy = 0.0;
    if (!build_outline_shape(ctx, m_params.m_seed)) {
        return false;
    }
    if (m_params.m_fill_mode == TERR_OUTLINE_FILL_MODE_PERLIN_NOISE) {
        if (!fill_mode_perlin_noise(ctx, m_params, m_terrain, m_land_to_water, m_interior_mask)) {
            return false;
        }
        m_has_land_to_water = true;
    } else if (m_params.m_fill_mode == TERR_OUTLINE_FILL_MODE_PN_MIX) {
        fill_mode_pn_mix(ctx);
    } else {
        fill_mode_full(ctx);
    }
    m_valid_generation = true;
    return true;
}

bool Generate_TerrainContOutline::is_valid () const {
    return m_valid_generation;
}

bool Generate_TerrainContOutline::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return false;
    }
    return m_terrain.save(path);
}

bool Generate_TerrainContOutline::save_land_to_water_distance (cstr path) const {
    if (!m_valid_generation || !m_has_land_to_water || path == nullptr) {
        return false;
    }
    Generate_OverlayWaterLand wl;
    if (!wl.generate(m_terrain) || !wl.is_valid()) {
        return false;
    }
    const u8* g = wl.water_land_gray();
    if (g == nullptr) {
        return false;
    }
    Generate_DistanceLandToWater d(0);
    if (!d.generate(g, m_terrain.width(), m_terrain.height())) {
        return false;
    }
    return d.save_output(path);
}

bool Generate_TerrainContOutline::save_interior_grad_mask (cstr path) const {
    if (!m_valid_generation || !m_has_land_to_water || path == nullptr || m_interior_mask.empty()) {
        return false;
    }
    return save_perlin_gray_pgm(path, m_interior_mask.data(), m_terrain.width(), m_terrain.height());
}

u16 Generate_TerrainContOutline::width () const {
    return m_terrain.width() != 0 ? m_terrain.width() : m_params.m_width;
}

u16 Generate_TerrainContOutline::height () const {
    return m_terrain.height() != 0 ? m_terrain.height() : m_params.m_height;
}

const MapArrayTerrain& Generate_TerrainContOutline::terrain () const {
    return m_terrain;
}

const MapArrayDistance& Generate_TerrainContOutline::land_to_water_distance () const {
    return m_land_to_water;
}

bool Generate_TerrainContOutline::has_land_to_water_distance () const {
    return m_has_land_to_water;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
