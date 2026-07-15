//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstring>

#include "p1_gen_shaped_outline.h"
#include "generator_constants.h"
#include "p1_gen_noise_perlin.h"
#include "p1_step_log.h"
#include "p1_wb_util.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_dist_inf = 0xFFFFu;

static inline bool ol_is_land (const u8* ov, u32 i) {
    return ov[i] == WL_OVERLAY_LAND_GRAY;
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

static void swap_f32 (f32* a, f32* b) {
    const f32 t = *a;
    *a = *b;
    *b = t;
}

static void nth_f32 (f32* a, u32 n, u32 k) {
    if (n == 0u) {
        return;
    }
    u32 lo = 0;
    u32 hi = n - 1u;
    while (lo < hi) {
        const f32 pivot = a[lo + (hi - lo) / 2u];
        u32 i = lo;
        u32 j = hi;
        while (i <= j) {
            while (a[i] < pivot) {
                i++;
            }
            while (a[j] > pivot) {
                j--;
            }
            if (i <= j) {
                swap_f32(&a[i], &a[j]);
                i++;
                if (j > 0) {
                    j--;
                } else {
                    break;
                }
            }
        }
        if (k <= j) {
            hi = j;
        } else if (k >= i) {
            lo = i;
        } else {
            break;
        }
    }
}

static u8 terr_cls_shaped (f64 v, const f64 thr[3]) {
    if (v <= thr[0]) {
        return TERR_OCEAN[0];
    }
    if (v <= thr[1]) {
        return TERR_SEA[0];
    }
    if (v <= thr[2]) {
        return TERR_COASTAL[0];
    }
    return TERR_PLAINS[0];
}

static f64 shelf_cum_pop (const P1_ShapedShelfFracs& shelf, u8 band) {
    u32 sum = 0u;
    if (band >= 1u) {
        sum += static_cast<u32>(shelf.m_ocean);
    }
    if (band >= 2u) {
        sum += static_cast<u32>(shelf.m_sea);
    }
    if (band >= 3u) {
        sum += static_cast<u32>(shelf.m_coastal);
    }
    return static_cast<f64>(sum) / 100.0;
}

static bool shelf_thr_from_fracs (
    const f32* norm,
    u32 n,
    const u8* ol_ov,
    u32 shelf_n,
    const P1_ShapedShelfFracs& shelf,
    f32* work,
    f64 thr[3]) 
{
    if (norm == nullptr || ol_ov == nullptr || work == nullptr || shelf_n == 0u) {
        return false;
    }
    auto refill = [&]() {
        u32 si = 0u;
        for (u32 i = 0; i < n; ++i) {
            if (!ol_is_land(ol_ov, i)) {
                continue;
            }
            work[si++] = norm[i];
        }
    };
    refill();
    const size_t j0 = frac_idx(shelf_n, shelf_cum_pop(shelf, 1u), 1.0);
    nth_f32(work, shelf_n, static_cast<u32>(j0));
    thr[0] = static_cast<f64>(work[j0]);
    refill();
    const size_t j1 = frac_idx(shelf_n, shelf_cum_pop(shelf, 2u), 1.0);
    nth_f32(work, shelf_n, static_cast<u32>(j1));
    thr[1] = static_cast<f64>(work[j1]);
    refill();
    const size_t j2 = frac_idx(shelf_n, shelf_cum_pop(shelf, 3u), 1.0);
    nth_f32(work, shelf_n, static_cast<u32>(j2));
    thr[2] = static_cast<f64>(work[j2]);
    return true;
}

static u16 land_depth_max (const u16* land_depth, u32 n) {
    u16 d_max = 0;
    for (u32 i = 0; i < n; ++i) {
        const u16 d = land_depth[i];
        if (d != k_dist_inf && d > d_max) {
            d_max = d;
        }
    }
    return d_max;
}

static bool shape_outline_layer (
    const f32* perlin_base,
    u16 w,
    u16 h,
    f32 radial,
    const P1_ShapedShelfFracs& shelf,
    u8* terrain,
    const u8* ol_ov,
    const u16* land_depth) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0 || perlin_base == nullptr || terrain == nullptr || ol_ov == nullptr || land_depth == nullptr) {
        P1_STEP_ABORT("P1_Gen_ShapedOutline", "shape_outline_layer null args");
    }
    Whiteboard_4B wb_combo("P1_Gen_ShapedOutline", "combo", 0u);
    Whiteboard_4B wb_norm("P1_Gen_ShapedOutline", "norm", 0u);
    P1_WB_CHK(wb_combo);
    P1_WB_CHK(wb_norm);
    f32* combo = reinterpret_cast<f32*>(wb_combo.get_iter_ptr());
    f32* norm = reinterpret_cast<f32*>(wb_norm.get_iter_ptr());
    const u16 d_max = land_depth_max(land_depth, n);
    f32 vmin = 0.f;
    f32 vmax = 0.f;
    bool first = true;
    for (u32 i = 0; i < n; ++i) {
        const f32 cv = perlin_base[i] - land_dist_sub(land_depth[i], d_max, radial);
        combo[i] = cv;
        if (first) {
            vmin = vmax = cv;
            first = false;
        } else {
            if (cv < vmin) {
                vmin = cv;
            }
            if (cv > vmax) {
                vmax = cv;
            }
        }
    }
    f32 denom = vmax - vmin;
    if (denom < 1e-12f) {
        denom = 1.f;
    }
    for (u32 i = 0; i < n; ++i) {
        norm[i] = (combo[i] - vmin) / denom;
    }
    u32 shelf_n = 0u;
    for (u32 i = 0; i < n; ++i) {
        if (ol_is_land(ol_ov, i)) {
            ++shelf_n;
        }
    }
    if (shelf_n == 0u) {
        P1_STEP_ABORT("P1_Gen_ShapedOutline", "shape_outline_layer no shelf pixels");
    }
    f64 thr[3];
    if (!shelf_thr_from_fracs(norm, n, ol_ov, shelf_n, shelf, combo, thr)) {
        P1_STEP_ABORT("P1_Gen_ShapedOutline", "shape_outline_layer shelf thresholds failed");
    }
    for (u32 i = 0; i < n; ++i) {
        if (!ol_is_land(ol_ov, i)) {
            terrain[i] = TERR_OCEAN[0];
            continue;
        }
        f64 vd = static_cast<f64>(norm[i]);
        if (vd < 0.0) {
            vd = 0.0;
        }
        if (vd > 1.0) {
            vd = 1.0;
        }
        terrain[i] = terr_cls_shaped(vd, thr);
    }
    return true;
}

//================================================================================================================================
//=> - P1_Gen_ShapedOutline -
//================================================================================================================================

P1_Gen_ShapedOutline::P1_Gen_ShapedOutline (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_perlin_bind(nullptr),
    m_perlin_n(0u) {
}

P1_Gen_ShapedOutline::~P1_Gen_ShapedOutline () {
    free_perlin();
}

void P1_Gen_ShapedOutline::free_perlin () {
    m_perlin_bind = nullptr;
    m_perlin_n = 0u;
}

void P1_Gen_ShapedOutline::bind_perlin_field (const f32* field, u16 w, u16 h) {
    free_perlin();
    m_perlin_bind = field;
    m_perlin_n = static_cast<u32>(w) * static_cast<u32>(h);
}

bool P1_Gen_ShapedOutline::generate_layer (
    u8* terrain,
    u16 w,
    u16 h,
    const u8* ol_ov,
    const u16* land_depth,
    f32 radial,
    const P1_ShapedShelfFracs& shelf) 
{
    m_valid_generation = false;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || ol_ov == nullptr || land_depth == nullptr) {
        P1_STEP_ABORT("P1_Gen_ShapedOutline", "generate_layer null args");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        P1_STEP_ABORT("P1_Gen_ShapedOutline", "generate_layer size mismatch");
    }
    const f32* perlin = m_perlin_bind;
    if (perlin == nullptr) {
        Whiteboard_4B wb_perlin("P1_Gen_ShapedOutline", "perlin", m_prm.m_seed);
        P1_WB_CHK(wb_perlin);
        f32* combo = reinterpret_cast<f32*>(wb_perlin.get_iter_ptr());
        const P1_Gen_NoisePerlinPrm nprm = p1_gen_noise_perlin_prm_from_cfg(map_config_def(), w, h);
        if (!p1_build_perlin_field_f32(m_prm.m_seed, nprm, combo, nullptr)) {
            P1_STEP_ABORT("P1_Gen_ShapedOutline", "generate_layer perlin build failed");
        }
        if (!shape_outline_layer(combo, w, h, radial, shelf, terrain, ol_ov, land_depth)) {
            return false;
        }
        m_valid_generation = true;
        return true;
    }
    if (!shape_outline_layer(perlin, w, h, radial, shelf, terrain, ol_ov, land_depth)) {
        return false;
    }
    m_valid_generation = true;
    return true;
}

static u8 terr_pick_min (u8 a, u8 b) {
    return a < b ? a : b;
}

bool P1_Gen_ShapedOutline::merge_layers (
    u8* terrain,
    u16 w,
    u16 h,
    const u8* ol_ov,
    const u16* land_depth,
    const u8* near_ter,
    const u8* far_ter) 
{
    m_valid_generation = false;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || ol_ov == nullptr
        || land_depth == nullptr || near_ter == nullptr || far_ter == nullptr) {
        P1_STEP_ABORT("P1_Gen_ShapedOutline", "merge_layers null args");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        P1_STEP_ABORT("P1_Gen_ShapedOutline", "merge_layers size mismatch");
    }
    (void)land_depth;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (!ol_is_land(ol_ov, i)) {
            terrain[i] = TERR_OCEAN[0];
            continue;
        }
        terrain[i] = terr_pick_min(near_ter[i], far_ter[i]);
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_ShapedOutline::apply (
    const P1_Gen_ShapedOutlinePrm& sp,
    u8* terrain,
    u16 w,
    u16 h,
    const u8* ol_ov,
    const u16* land_depth) 
{
    m_valid_generation = false;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || ol_ov == nullptr || land_depth == nullptr) {
        P1_STEP_ABORT("P1_Gen_ShapedOutline", "apply null args");
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_near("P1_Gen_ShapedOutline", "near_ter", 0u);
    Whiteboard_2B wb_far("P1_Gen_ShapedOutline", "far_ter", 0u);
    P1_WB_CHK(wb_near);
    P1_WB_CHK(wb_far);
    u8* near_ter = reinterpret_cast<u8*>(wb_near.get_iter_ptr());
    u8* far_ter = reinterpret_cast<u8*>(wb_far.get_iter_ptr());
    std::memcpy(near_ter, terrain, static_cast<size_t>(n));
    std::memcpy(far_ter, terrain, static_cast<size_t>(n));
    if (!generate_layer(near_ter, w, h, ol_ov, land_depth, sp.m_radial_near, sp.m_shelf_near)) {
        return false;
    }
    if (!generate_layer(far_ter, w, h, ol_ov, land_depth, sp.m_radial_far, sp.m_shelf_far)) {
        return false;
    }
    return merge_layers(terrain, w, h, ol_ov, land_depth, near_ter, far_ter);
}

bool P1_Gen_ShapedOutline::is_valid () const {
    return m_valid_generation;
}

bool p1_apply_shaped_outline (
    const P1_RunPrm& prm,
    const P1_Gen_ShapedOutlinePrm& sp,
    u8* terrain,
    u16 w,
    u16 h,
    const u8* ol_ov,
    const u16* land_depth) 
{
    P1_Gen_ShapedOutline gen(prm);
    return gen.apply(sp, terrain, w, h, ol_ov, land_depth);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
