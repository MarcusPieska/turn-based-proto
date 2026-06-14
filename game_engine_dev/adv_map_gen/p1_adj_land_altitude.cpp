//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "p1_adj_land_altitude.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static bool is_plains (u8 cls) {
    return cls == TERR_PLAINS[0];
}

static f32 ov_norm (u8 v) {
    return static_cast<f32>(v) / 255.f;
}

static size_t frac_idx (u32 n, f32 pop, f32 cap) {
    if (n == 0u) {
        return 0u;
    }
    if (pop <= 0.f) {
        return 0u;
    }
    if (pop >= cap) {
        return static_cast<size_t>(n - 1u);
    }
    const size_t j = static_cast<size_t>(pop * static_cast<f32>(n));
    return j >= static_cast<size_t>(n) ? static_cast<size_t>(n - 1u) : j;
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

static u8 land_alt_cls (f32 v, f32 thr_hil, f32 thr_mtn) {
    if (v <= thr_hil) {
        return TERR_PLAINS[0];
    }
    if (v <= thr_mtn) {
        return TERR_HILLS[0];
    }
    return TERR_MOUNTAINS[0];
}

static bool build_land_norm (
    const u8* terrain,
    u16 w,
    u16 h,
    const u8* noise,
    const u8* dist_riv,
    const u8* near_mtn,
    const P1_Adj_LandAltitudePrm& sp,
    f32** out_norm,
    u32* out_land_n,
    f32* thr_hil,
    f32* thr_mtn) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (out_norm == nullptr || out_land_n == nullptr || thr_hil == nullptr || thr_mtn == nullptr) {
        return false;
    }
    *out_norm = nullptr;
    *out_land_n = 0;
    if (n == 0 || terrain == nullptr || noise == nullptr || dist_riv == nullptr || near_mtn == nullptr) {
        return false;
    }
    u32 land_n = 0;
    for (u32 i = 0; i < n; ++i) {
        if (is_plains(terrain[i])) {
            land_n++;
        }
    }
    *out_land_n = land_n;
    if (land_n == 0) {
        *thr_hil = 0.f;
        *thr_mtn = 0.f;
        return true;
    }
    f32* raw = new f32[land_n];
    if (raw == nullptr) {
        return false;
    }
    u32 li = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_plains(terrain[i])) {
            continue;
        }
        const f32 sc = sp.m_w_noise * ov_norm(noise[i])
            + sp.m_w_near * ov_norm(near_mtn[i])
            + sp.m_w_riv * ov_norm(dist_riv[i]);
        raw[li++] = sc;
    }
    f32 vmin = raw[0];
    f32 vmax = raw[0];
    for (u32 i = 1; i < land_n; ++i) {
        if (raw[i] < vmin) {
            vmin = raw[i];
        }
        if (raw[i] > vmax) {
            vmax = raw[i];
        }
    }
    f32 denom = vmax - vmin;
    if (denom < 1e-12f) {
        denom = 1.f;
    }
    f32* norm = new f32[land_n];
    if (norm == nullptr) {
        delete[] raw;
        return false;
    }
    for (u32 i = 0; i < land_n; ++i) {
        norm[i] = (raw[i] - vmin) / denom;
    }
    delete[] raw;
    const size_t jh = frac_idx(land_n, sp.m_lim_hills, 1.f);
    const size_t jm = frac_idx(land_n, sp.m_lim_mtn, 1.f);
    f32* work = new f32[land_n];
    if (work == nullptr) {
        delete[] norm;
        return false;
    }
    for (u32 i = 0; i < land_n; ++i) {
        work[i] = norm[i];
    }
    nth_f32(work, land_n, static_cast<u32>(jh));
    *thr_hil = work[jh];
    for (u32 i = 0; i < land_n; ++i) {
        work[i] = norm[i];
    }
    nth_f32(work, land_n, static_cast<u32>(jm));
    *thr_mtn = work[jm];
    delete[] work;
    *out_norm = norm;
    return true;
}

static void fill_joint_map (
    const u8* terrain,
    u16 w,
    u16 h,
    const f32* norm,
    u8* joint) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 li = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_plains(terrain[i]) || norm == nullptr) {
            joint[i] = 0;
            continue;
        }
        const f32 v = norm[li++];
        if (v <= 0.f) {
            joint[i] = 0;
        } else if (v >= 1.f) {
            joint[i] = 255;
        } else {
            joint[i] = static_cast<u8>(v * 255.f);
        }
    }
}

static bool apply_land_alt (
    u8* terrain,
    u16 w,
    u16 h,
    const u8* noise,
    const u8* dist_riv,
    const u8* near_mtn,
    const P1_Adj_LandAltitudePrm& sp) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0 || terrain == nullptr || noise == nullptr || dist_riv == nullptr || near_mtn == nullptr) {
        return false;
    }
    f32* norm = nullptr;
    u32 land_n = 0;
    f32 thr_hil = 0.f;
    f32 thr_mtn = 0.f;
    if (!build_land_norm(terrain, w, h, noise, dist_riv, near_mtn, sp, &norm, &land_n, &thr_hil, &thr_mtn)) {
        return false;
    }
    if (land_n == 0) {
        return true;
    }
    u32 li = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_plains(terrain[i])) {
            continue;
        }
        terrain[i] = land_alt_cls(norm[li++], thr_hil, thr_mtn);
    }
    delete[] norm;
    return true;
}

//================================================================================================================================
//=> - P1_Adj_LandAltitude -
//================================================================================================================================

P1_Adj_LandAltitude::P1_Adj_LandAltitude (const P1_RunPrm& prm, const P1_Adj_LandAltitudePrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_adjust(false) {
}

bool P1_Adj_LandAltitude::adjust (
    u8* terrain,
    u16 w,
    u16 h,
    const u8* noise,
    const u8* dist_riv,
    const u8* near_mtn) 
{
    m_valid_adjust = false;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || noise == nullptr
        || dist_riv == nullptr || near_mtn == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!apply_land_alt(terrain, w, h, noise, dist_riv, near_mtn, m_sp)) {
        return false;
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_LandAltitude::is_valid () const {
    return m_valid_adjust;
}

bool P1_Adj_LandAltitude::joint_ov (
    const u8* terrain,
    u16 w,
    u16 h,
    const u8* noise,
    const u8* dist_riv,
    const u8* near_mtn,
    u8* joint) const 
{
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || noise == nullptr
        || dist_riv == nullptr || near_mtn == nullptr || joint == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    f32* norm = nullptr;
    u32 land_n = 0;
    f32 thr_hil = 0.f;
    f32 thr_mtn = 0.f;
    if (!build_land_norm(terrain, w, h, noise, dist_riv, near_mtn, m_sp, &norm, &land_n, &thr_hil, &thr_mtn)) {
        return false;
    }
    fill_joint_map(terrain, w, h, norm, joint);
    delete[] norm;
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
