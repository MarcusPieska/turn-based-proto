//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_coast_fertility.h"

#include "game_map_defs.h"
#include "p1_wb_util.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static u32 pct_n (u32 nz, u16 pct) {
    if (nz == 0u || pct == 0u) {
        return 0u;
    }
    if (pct >= 100u) {
        return nz;
    }
    return (nz * static_cast<u32>(pct) + 99u) / 100u;
}

static void qs_swap3 (u16* v, u16* x, u16* y, i32 i, i32 j) {
    const u16 tv = v[i];
    v[i] = v[j];
    v[j] = tv;
    const u16 tx = x[i];
    x[i] = x[j];
    x[j] = tx;
    const u16 ty = y[i];
    y[i] = y[j];
    y[j] = ty;
}

static i32 qs_part_desc (u16* v, u16* x, u16* y, i32 lo, i32 hi) {
    const u16 piv = v[hi];
    i32 i = lo - 1;
    for (i32 j = lo; j < hi; ++j) {
        if (v[j] >= piv) {
            ++i;
            qs_swap3(v, x, y, i, j);
        }
    }
    qs_swap3(v, x, y, i + 1, hi);
    return i + 1;
}

static void qs_sort_desc (u16* v, u16* x, u16* y, i32 lo, i32 hi) {
    if (lo >= hi) {
        return;
    }
    const i32 p = qs_part_desc(v, x, y, lo, hi);
    if (p > lo) {
        qs_sort_desc(v, x, y, lo, p - 1);
    }
    if (p < hi) {
        qs_sort_desc(v, x, y, p + 1, hi);
    }
}

static u32 collect_fert_tiles (
    const u16* fert,
    u16 w,
    u16 h,
    u16* val,
    u16* px,
    u16* py) 
{
    if (fert == nullptr || val == nullptr || px == nullptr || py == nullptr) {
        return 0u;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 k = 0u;
    for (u32 i = 0; i < n; ++i) {
        const u16 fv = fert[i];
        if (fv == 0) {
            continue;
        }
        val[k] = fv;
        px[k] = static_cast<u16>(i % static_cast<u32>(w));
        py[k] = static_cast<u16>(i / static_cast<u32>(w));
        ++k;
    }
    return k;
}

static bool can_grass_up (u8 cl) {
    return cl != CLIMATE_BLACK_SOIL && cl != CLIMATE_GRASSLAND;
}

static bool can_plains_up (u8 cl) {
    return cl != CLIMATE_BLACK_SOIL && cl != CLIMATE_GRASSLAND && cl != CLIMATE_PLAINS;
}

static bool apply_coast_fert (
    u8* climate,
    u16 w,
    u16 h,
    const u16* fert,
    u16 grass_pct,
    u16 plains_pct,
    u32* out_nz,
    u32* out_grass,
    u32* out_plains) 
{
    if (climate == nullptr || fert == nullptr || out_nz == nullptr
        || out_grass == nullptr || out_plains == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    Whiteboard_2B wb_val("P1_Adj_CoastFertility", "val", 0u);
    Whiteboard_2B wb_px("P1_Adj_CoastFertility", "px", 0u);
    Whiteboard_2B wb_py("P1_Adj_CoastFertility", "py", 0u);
    if (!wb_val.ok() || !wb_px.ok() || !wb_py.ok()) {
        return false;
    }
    u16* val = wb_val.get_iter_ptr();
    u16* px = wb_px.get_iter_ptr();
    u16* py = wb_py.get_iter_ptr();
    const u32 nz = collect_fert_tiles(fert, w, h, val, px, py);
    *out_nz = nz;
    *out_grass = 0u;
    *out_plains = 0u;
    if (nz > 1u) {
        qs_sort_desc(val, px, py, 0, static_cast<i32>(nz - 1u));
    }
    const u32 grass_n = pct_n(nz, grass_pct);
    const u32 plains_n = pct_n(nz, plains_pct);
    u32 gi = 0u;
    for (; gi < grass_n; ++gi) {
        const u32 ti = static_cast<u32>(py[gi]) * static_cast<u32>(w) + static_cast<u32>(px[gi]);
        const u8 cl = climate[ti];
        if (!can_grass_up(cl)) {
            continue;
        }
        climate[ti] = CLIMATE_GRASSLAND;
        ++(*out_grass);
    }
    const u32 plains_end = grass_n + plains_n;
    u32 pi = grass_n;
    for (; pi < plains_end && pi < nz; ++pi) {
        const u32 ti = static_cast<u32>(py[pi]) * static_cast<u32>(w) + static_cast<u32>(px[pi]);
        const u8 cl = climate[ti];
        if (!can_plains_up(cl)) {
            continue;
        }
        climate[ti] = CLIMATE_PLAINS;
        ++(*out_plains);
    }
    return true;
}

//================================================================================================================================
//=> - P1_Adj_CoastFertility -
//================================================================================================================================

P1_Adj_CoastFertility::P1_Adj_CoastFertility (
    const P1_RunPrm& prm,
    const P1_Adj_CoastFertilityPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_adjust(false),
    m_nz_n(0),
    m_grass_n(0),
    m_plains_n(0) {
} 

bool P1_Adj_CoastFertility::adjust (u8* climate, u16 w, u16 h, const u16* fert) {
    m_valid_adjust = false;
    m_nz_n = 0;
    m_grass_n = 0;
    m_plains_n = 0;
    if (!p1_run_prm_ok(m_prm) || climate == nullptr || fert == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!apply_coast_fert(climate, w, h, fert, m_sp.m_grass_pct, m_sp.m_plains_pct,
            &m_nz_n, &m_grass_n, &m_plains_n)) {
        return false;
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_CoastFertility::is_valid () const {
    return m_valid_adjust;
}

u32 P1_Adj_CoastFertility::nz_n () const {
    return m_nz_n;
}

u32 P1_Adj_CoastFertility::grass_n () const {
    return m_grass_n;
}

u32 P1_Adj_CoastFertility::plains_n () const {
    return m_plains_n;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
