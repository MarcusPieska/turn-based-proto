//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_shaped_outline.h"

#include "generator_constants.h"
#include "perlin_noise.h"

#include <cmath>
#include <cstring>

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_dist_inf = 0xFFFFu;

static inline bool ol_is_land (const u8* ov, u32 i) {
    return ov[i] == WL_OVERLAY_LAND_GRAY;
}

static u32 derived_layer_seed (u32 base_seed, i32 layer_idx) {
    return base_seed ^ (static_cast<u32>(layer_idx + 1) * 2654435769u);
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
    u32 seed,
    u16 w,
    u16 h,
    f32 radial,
    u8* terrain,
    const u8* ol_ov,
    const u16* land_depth) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0 || terrain == nullptr || ol_ov == nullptr || land_depth == nullptr) {
        return false;
    }
    const u16 d_max = land_depth_max(land_depth, n);
    const i32 lc = 5;
    const f32 lac = 2.f;
    const f32 freq_base = 0.5f;
    const f32 layer_w = 0.2f;
    const f32 freq_step = 1.62f;
    f32* combo = new f32[n];
    if (combo == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        combo[i] = 0.f;
    }
    PerlinImgParams lp;
    lp.m_w = w;
    lp.m_h = h;
    lp.m_lacunarity = lac;
    f32 freq = freq_base;
    for (i32 k = 0; k < lc; ++k) {
        lp.m_frequency = freq;
        if (!accumulate_perlin_field_f32(combo, layer_w, lp, derived_layer_seed(seed, k))) {
            delete[] combo;
            return false;
        }
        freq *= freq_step;
    }
    f32 vmin = 0.f;
    f32 vmax = 0.f;
    bool first = true;
    for (u32 i = 0; i < n; ++i) {
        combo[i] -= land_dist_sub(land_depth[i], d_max, radial);
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
    f32* norm = new f32[n];
    if (norm == nullptr) {
        delete[] combo;
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        norm[i] = (combo[i] - vmin) / denom;
    }
    delete[] combo;
    const f64 lim_cap = 1.0;
    const size_t j0 = frac_idx(n, 0.7, lim_cap);
    const size_t j1 = frac_idx(n, 0.85, lim_cap);
    const size_t j2 = frac_idx(n, 0.88, lim_cap);
    f32* work = new f32[n];
    if (work == nullptr) {
        delete[] norm;
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        work[i] = norm[i];
    }
    nth_f32(work, n, static_cast<u32>(j0));
    f64 thr[3];
    thr[0] = static_cast<f64>(work[j0]);
    for (u32 i = 0; i < n; ++i) {
        work[i] = norm[i];
    }
    nth_f32(work, n, static_cast<u32>(j1));
    thr[1] = static_cast<f64>(work[j1]);
    for (u32 i = 0; i < n; ++i) {
        work[i] = norm[i];
    }
    nth_f32(work, n, static_cast<u32>(j2));
    thr[2] = static_cast<f64>(work[j2]);
    delete[] work;
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
    delete[] norm;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_ShapedOutline -
//================================================================================================================================

P1_Gen_ShapedOutline::P1_Gen_ShapedOutline (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false) {
}

bool P1_Gen_ShapedOutline::generate_layer (
    u8* terrain,
    u16 w,
    u16 h,
    const u8* ol_ov,
    const u16* land_depth,
    f32 radial) 
{
    m_valid_generation = false;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || ol_ov == nullptr || land_depth == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!shape_outline_layer(m_prm.m_seed, w, h, radial, terrain, ol_ov, land_depth)) {
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
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
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
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* near_ter = new u8[n];
    u8* far_ter = new u8[n];
    if (near_ter == nullptr || far_ter == nullptr) {
        delete[] far_ter;
        delete[] near_ter;
        return false;
    }
    std::memcpy(near_ter, terrain, static_cast<size_t>(n));
    std::memcpy(far_ter, terrain, static_cast<size_t>(n));
    if (!generate_layer(near_ter, w, h, ol_ov, land_depth, sp.m_radial_near)) {
        delete[] far_ter;
        delete[] near_ter;
        return false;
    }
    if (!generate_layer(far_ter, w, h, ol_ov, land_depth, sp.m_radial_far)) {
        delete[] far_ter;
        delete[] near_ter;
        return false;
    }
    const bool ok = merge_layers(terrain, w, h, ol_ov, land_depth, near_ter, far_ter);
    delete[] far_ter;
    delete[] near_ter;
    return ok;
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
