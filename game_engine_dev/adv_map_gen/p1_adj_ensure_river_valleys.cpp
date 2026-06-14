//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_ensure_river_valleys.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool is_elev_land (u8 cls) {
    return cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool on_or_adj8_riv (const u8* riv, u16 w, u16 h, u16 x, u16 y) {
    static const i32 k_dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const i32 k_dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const u32 ci = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    if (riv[ci] != 0) {
        return true;
    }
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx[k];
        const i32 ny = static_cast<i32>(y) + k_dy[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ti = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (riv[ti] != 0) {
            return true;
        }
    }
    return false;
}

static bool apply_ensure_river_valleys (u8* terrain, u16 w, u16 h, const u8* riv) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0 || terrain == nullptr || riv == nullptr) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (!is_land(terrain[i]) || !is_elev_land(terrain[i])) {
                continue;
            }
            if (on_or_adj8_riv(riv, w, h, x, y)) {
                terrain[i] = TERR_PLAINS[0];
            }
        }
    }
    return true;
}

//================================================================================================================================
//=> - P1_Adj_EnsureRiverValleys -
//================================================================================================================================

P1_Adj_EnsureRiverValleys::P1_Adj_EnsureRiverValleys (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_adjust(false) {
}

bool P1_Adj_EnsureRiverValleys::adjust (u8* terrain, u16 w, u16 h, const u8* riv) {
    m_valid_adjust = false;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || riv == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!apply_ensure_river_valleys(terrain, w, h, riv)) {
        return false;
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_EnsureRiverValleys::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
