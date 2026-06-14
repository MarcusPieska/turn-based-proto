//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_ensure_mtn_foothills.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool adj8_plains (const u8* terrain, u16 w, u16 h, u16 x, u16 y) {
    static const i32 k_dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const i32 k_dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx[k];
        const i32 ny = static_cast<i32>(y) + k_dy[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ti = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (terrain[ti] == TERR_PLAINS[0]) {
            return true;
        }
    }
    return false;
}

static bool apply_ensure_mtn_foothills (u8* terrain, u16 w, u16 h) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0 || terrain == nullptr) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (terrain[i] != TERR_MOUNTAINS[0]) {
                continue;
            }
            if (adj8_plains(terrain, w, h, x, y)) {
                terrain[i] = TERR_HILLS[0];
            }
        }
    }
    return true;
}

//================================================================================================================================
//=> - P1_Adj_EnsureMtnFoothills -
//================================================================================================================================

P1_Adj_EnsureMtnFoothills::P1_Adj_EnsureMtnFoothills (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_adjust(false) {
}

bool P1_Adj_EnsureMtnFoothills::adjust (u8* terrain, u16 w, u16 h) {
    m_valid_adjust = false;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!apply_ensure_mtn_foothills(terrain, w, h)) {
        return false;
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_EnsureMtnFoothills::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
