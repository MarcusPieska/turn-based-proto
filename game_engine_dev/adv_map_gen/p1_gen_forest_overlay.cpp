//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "p1_gen_forest_overlay.h"

#include "game_map_defs.h"
#include "perlin_noise.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"

//================================================================================================================================
//=> - Design-time forest overlay tuning -
//================================================================================================================================

static const u16 k_grass_chance_base = 50u;
static const u16 k_grass_chance_step = 10u;
static const u16 k_chance_cap_dist = 4u;
static const u16 k_plains_chance_base = 0u;
static const u16 k_plains_chance_step = 10u;
static const u16 k_plains_deep_chance = 50u;

static const u8 k_perlin_mod_span = 100u;
static const u8 k_perlin_mod_center = 50u;
static const u32 k_perlin_tile_sz = 200u;
static const f32 k_perlin_freq = 2.f;

//================================================================================================================================
//=> - Private rng -
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

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;
static const i32 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i32 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w) && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_grass_like (u8 cl) {
    return cl == CLIMATE_GRASSLAND || cl == CLIMATE_BLACK_SOIL;
}

static bool is_forest_terr_p1 (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

static bool is_forest_terr_p2 (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

static bool clim_adj (const u8* climate, u16 w, u16 h, u16 x, u16 y, u8 want) {
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx8[k];
        const i32 ny = static_cast<i32>(y) + k_dy8[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (climate[j] == want) {
            return true;
        }
    }
    return false;
}

static bool bfs_clim_dist (
    u16 w,
    u16 h,
    const u8* climate,
    u16* dist,
    WB_QueXY& q,
    u16* max_dist,
    bool (*in_reg) (u8),
    u8 adj_clim) 
{
    if (climate == nullptr || dist == nullptr || !q.ok() || max_dist == nullptr || in_reg == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    *max_dist = 0;
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_inf;
    }
    q.clear();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (!in_reg(climate[i]) || !clim_adj(climate, w, h, x, y, adj_clim)) {
                continue;
            }
            dist[i] = 0;
            if (!q.push(x, y)) {
                return false;
            }
        }
    }
    while (q.count() > 0) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        const u16 cur = dist[i];
        if (cur > *max_dist) {
            *max_dist = cur;
        }
        const u16 nxt = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (in_reg(climate[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                if (!q.push(static_cast<u16>(px - 1), py)) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (in_reg(climate[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                if (!q.push(static_cast<u16>(px + 1), py)) {
                    return false;
                }
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (in_reg(climate[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                if (!q.push(px, static_cast<u16>(py - 1))) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (in_reg(climate[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                if (!q.push(px, static_cast<u16>(py + 1))) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool in_grass_reg (u8 cl) {
    return is_grass_like(cl);
}

static bool in_plains_reg (u8 cl) {
    return cl == CLIMATE_PLAINS;
}

static u8 scale_dist_viz (u16 d) {
    if (d == k_inf) {
        return 0;
    }
    u32 cap = static_cast<u32>(k_chance_cap_dist) + 1u;
    u32 dv = static_cast<u32>(d);
    if (dv > cap) {
        dv = cap;
    }
    return static_cast<u8>((dv * 255u) / cap);
}

static u16 grass_forest_chance (u16 d) {
    if (d == k_inf) {
        return 0;
    }
    if (d > k_chance_cap_dist) {
        return 100;
    }
    return static_cast<u16>(k_grass_chance_base + k_grass_chance_step * d);
}

static u16 plains_forest_chance (u16 d) {
    if (d == k_inf) {
        return 0;
    }
    if (d > k_chance_cap_dist) {
        return k_plains_deep_chance;
    }
    return static_cast<u16>(k_plains_chance_base + k_plains_chance_step * d);
}

static bool roll_forest (Rng32* rng, u16 ch) {
    if (ch == 0) {
        return false;
    }
    return ch >= 100 || (rng_next(rng) % 100u) < static_cast<u32>(ch);
}

static u16 plains_fill_dist (u16 max_desert) {
    if (max_desert > k_chance_cap_dist) {
        return max_desert;
    }
    return static_cast<u16>(k_chance_cap_dist + 1u);
}

static u8 gray_to_mod (u8 g) {
    return static_cast<u8>((static_cast<u32>(g) * static_cast<u32>(k_perlin_mod_span) + 127u) / 255u);
}

static u16 forest_chance_adj (u16 base, u8 mod) {
    i32 v = static_cast<i32>(base) + static_cast<i32>(mod) - static_cast<i32>(k_perlin_mod_center);
    if (v < 0) {
        return 0;
    }
    if (v > 100) {
        return 100;
    }
    return static_cast<u16>(v);
}

static u8 perlin_col_px (const u8* tile_a, u16 ts, u16 lx, u16 ly) {
    const u16 yb = ly / ts;
    const u16 ay = ly % ts;
    const u16 sy = (yb & 1u) == 0u ? ay : (ts - 1u - ay);
    return tile_a[static_cast<u32>(sy) * static_cast<u32>(ts) + static_cast<u32>(lx)];
}

static bool gen_perlin_mod_tiled (u8* out, u16 w, u16 h, u32 seed, u8* tile_a, u32 tile_cap) {
    if (out == nullptr || tile_a == nullptr || w == 0 || h == 0 || k_perlin_tile_sz == 0u) {
        return false;
    }
    const u16 ts = static_cast<u16>(k_perlin_tile_sz);
    const u32 tile_n = static_cast<u32>(ts) * static_cast<u32>(ts);
    if (tile_cap < tile_n) {
        return false;
    }
    PerlinImgParams pp;
    pp.m_w = ts;
    pp.m_h = ts;
    pp.m_frequency = k_perlin_freq;
    pp.m_lacunarity = 2.f;
    if (!render_perlin_gray_u8(tile_a, pp, seed ^ 0xC1F3A5E9u)) {
        return false;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        tile_a[i] = gray_to_mod(tile_a[i]);
    }
    const u32 period = static_cast<u32>(ts) * 2u;
    for (u16 my = 0; my < h; ++my) {
        const u16 ly = static_cast<u16>(static_cast<u32>(my) % period);
        for (u16 mx = 0; mx < w; ++mx) {
            const u32 rx = static_cast<u32>(w - 1u - mx) % period;
            const u16 xb = static_cast<u16>(rx / static_cast<u32>(ts));
            const u16 lx = static_cast<u16>(rx % static_cast<u32>(ts));
            const u16 ax = (xb & 1u) == 0u ? static_cast<u16>(ts - 1u - lx) : lx;
            out[static_cast<u32>(my) * static_cast<u32>(w) + static_cast<u32>(mx)] =
                perlin_col_px(tile_a, ts, ax, ly);
        }
    }
    return true;
}

static u32 fill_unreached_plains (u16 w, u16 h, const u8* climate, u16* dist, u16 max_desert) {
    const u16 fill = plains_fill_dist(max_desert);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 cnt = 0;
    for (u32 i = 0; i < n; ++i) {
        if (climate[i] != CLIMATE_PLAINS || dist[i] != k_inf) {
            continue;
        }
        dist[i] = fill;
        ++cnt;
    }
    return cnt;
}

static bool res_ov_free (u8 v) {
    return v == 0u || v == static_cast<u8>(OVERLAY_NONE);
}

static bool build_forest_overlay (
    const u8* terrain,
    const u8* climate,
    u8* res_ov,
    u16 w,
    u16 h,
    u32 seed,
    P1_Gen_ForestOverlayRslt* out) 
{
    if (out == nullptr || terrain == nullptr || climate == nullptr || res_ov == nullptr
        || !p1_map_size_ok(w, h)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_dist("P1_Gen_ForestOverlay", "dist", seed);
    P1_WB_CHK(wb_dist);
    Whiteboard_2B wb_grass("P1_Gen_ForestOverlay", "grass", seed);
    P1_WB_CHK(wb_grass);
    Whiteboard_1B wb_tile("P1_Gen_ForestOverlay", "tile", seed);
    P1_WB_CHK(wb_tile);
    WB_QueXY que;
    if (!que.ok()) {
        return false;
    }
    u16* dist = wb_dist.get_iter_ptr();
    u16* grass_dist = wb_grass.get_iter_ptr();
    u16 max_grass = 0;
    if (!bfs_clim_dist(w, h, climate, dist, que, &max_grass, in_grass_reg, CLIMATE_PLAINS)) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        grass_dist[i] = dist[i];
    }
    u16 max_desert = 0;
    if (!bfs_clim_dist(w, h, climate, dist, que, &max_desert, in_plains_reg, CLIMATE_DESERT)) {
        return false;
    }
    const u16 fill_dist = plains_fill_dist(max_desert);
    const u32 plains_fill_n = fill_unreached_plains(w, h, climate, dist, max_desert);
    if (!out->m_dist_ov.resize(w, h) || !out->m_dist_desert_ov.resize(w, h)
        || !out->m_perlin_mod_ov.resize(w, h)) {
        return false;
    }
    u8* grass_pix = out->m_dist_ov.data_w();
    u8* desert_pix = out->m_dist_desert_ov.data_w();
    u8* mod_pix = out->m_perlin_mod_ov.data_w();
    u8* tile_scratch = wb_tile.get_iter_ptr();
    const u32 tile_cap = WhiteboardMng::tile_n() * 2u;
    if (!gen_perlin_mod_tiled(mod_pix, w, h, seed, tile_scratch, tile_cap)) {
        return false;
    }
    Rng32 rng_grass;
    Rng32 rng_plains;
    rng_seed(&rng_grass, seed ^ 0x4F1A2B3Cu);
    rng_seed(&rng_plains, seed ^ 0x7C3E9D1Au);
    u32 forest_n = 0;
    u32 grass_n = 0;
    u32 plains_n = 0;
    for (u32 i = 0; i < n; ++i) {
        const u16 gd = grass_dist[i];
        const u16 dd = dist[i];
        grass_pix[i] = scale_dist_viz(gd);
        desert_pix[i] = scale_dist_viz(dd);
        if (gd != k_inf && is_forest_terr_p1(terrain[i]) && res_ov_free(res_ov[i])) {
            const u16 ch = forest_chance_adj(grass_forest_chance(gd), mod_pix[i]);
            if (roll_forest(&rng_grass, ch)) {
                res_ov[i] = OV_FOREST[0];
                ++forest_n;
                ++grass_n;
                continue;
            }
        }
        if (dd != k_inf && is_forest_terr_p2(terrain[i]) && res_ov_free(res_ov[i])) {
            const u16 ch = forest_chance_adj(plains_forest_chance(dd), mod_pix[i]);
            if (ch > 0 && roll_forest(&rng_plains, ch)) {
                res_ov[i] = OV_FOREST[0];
                ++forest_n;
                ++plains_n;
            }
        }
    }
    out->m_w = w;
    out->m_h = h;
    out->m_max_grass_dist = max_grass;
    out->m_max_desert_dist = max_desert;
    out->m_forest_n = forest_n;
    out->m_forest_grass_n = grass_n;
    out->m_forest_plains_n = plains_n;
    out->m_plains_fill_n = plains_fill_n;
    out->m_plains_fill_dist = fill_dist;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_ForestOverlay -
//================================================================================================================================

P1_Gen_ForestOverlay::P1_Gen_ForestOverlay (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_max_grass_dist = 0;
    m_rslt.m_max_desert_dist = 0;
    m_rslt.m_forest_n = 0;
    m_rslt.m_forest_grass_n = 0;
    m_rslt.m_forest_plains_n = 0;
    m_rslt.m_plains_fill_n = 0;
    m_rslt.m_plains_fill_dist = 0;
    m_rslt.m_plains_clim_n = 0;
    m_rslt.m_plains_roll_n = 0;
    m_rslt.m_plains_deep_n = 0;
    for (u32 b = 0; b < 6u; ++b) {
        m_rslt.m_plains_band_tile_n[b] = 0;
        m_rslt.m_plains_roll_yes[b] = 0;
        m_rslt.m_plains_roll_no[b] = 0;
    }
    for (u32 r = 0; r < 100u; ++r) {
        m_rslt.m_plains_rn_hist[r] = 0;
    }
}

bool P1_Gen_ForestOverlay::generate (const u8* terrain, const u8* climate, u8* res_ov, u16 w, u16 h) {
    m_valid_generation = false;
    m_rslt.m_dist_ov.clear();
    m_rslt.m_dist_desert_ov.clear();
    m_rslt.m_perlin_mod_ov.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_max_grass_dist = 0;
    m_rslt.m_max_desert_dist = 0;
    m_rslt.m_forest_n = 0;
    m_rslt.m_forest_grass_n = 0;
    m_rslt.m_forest_plains_n = 0;
    m_rslt.m_plains_fill_n = 0;
    m_rslt.m_plains_fill_dist = 0;
    m_rslt.m_plains_clim_n = 0;
    m_rslt.m_plains_roll_n = 0;
    m_rslt.m_plains_deep_n = 0;
    for (u32 b = 0; b < 6u; ++b) {
        m_rslt.m_plains_band_tile_n[b] = 0;
        m_rslt.m_plains_roll_yes[b] = 0;
        m_rslt.m_plains_roll_no[b] = 0;
    }
    for (u32 r = 0; r < 100u; ++r) {
        m_rslt.m_plains_rn_hist[r] = 0;
    }
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || climate == nullptr || res_ov == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!build_forest_overlay(terrain, climate, res_ov, w, h, m_prm.m_seed, &m_rslt)) {
        m_rslt.m_dist_ov.clear();
        m_rslt.m_dist_desert_ov.clear();
        m_rslt.m_perlin_mod_ov.clear();
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_ForestOverlay::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_ForestOverlayRslt& P1_Gen_ForestOverlay::result () const {
    return m_rslt;
}

void P1_Gen_ForestOverlay::save_dist_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return;
    }
    m_rslt.m_dist_ov.save(path);
}

void P1_Gen_ForestOverlay::save_desert_dist_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return;
    }
    m_rslt.m_dist_desert_ov.save(path);
}

void P1_Gen_ForestOverlay::save_perlin_mod_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u8* mod = m_rslt.m_perlin_mod_ov.data();
    if (w == 0 || h == 0 || mod == nullptr) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_1B wb_viz("P1_Gen_ForestOverlay", "viz", m_prm.m_seed);
    P1_WB_CHK(wb_viz);
    u8* viz = wb_viz.get_iter_ptr();
    for (u32 i = 0; i < n; ++i) {
        viz[i] = static_cast<u8>((static_cast<u32>(mod[i]) * 255u + static_cast<u32>(k_perlin_mod_span) / 2u)
            / static_cast<u32>(k_perlin_mod_span));
    }
    save_perlin_gray_pgm(path, viz, w, h);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
