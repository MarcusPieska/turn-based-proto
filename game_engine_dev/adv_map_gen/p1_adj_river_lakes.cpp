//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_river_lakes.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_coast_or_riv (const u8* terrain, const u8* riv, u32 i) {
    return riv[i] != 0 || terrain[i] == TERR_INLAND_LAKE[0];
}

static bool quad_ok (
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 x,
    u16 y) 
{
    const u32 i0 = tidx(w, x, y);
    const u32 i1 = tidx(w, static_cast<u16>(x + 1u), y);
    const u32 i2 = tidx(w, x, static_cast<u16>(y + 1u));
    const u32 i3 = tidx(w, static_cast<u16>(x + 1u), static_cast<u16>(y + 1u));
    return is_coast_or_riv(terrain, riv, i0)
        && is_coast_or_riv(terrain, riv, i1)
        && is_coast_or_riv(terrain, riv, i2)
        && is_coast_or_riv(terrain, riv, i3);
}

static void stamp_quad (
    u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u16 x,
    u16 y) 
{
    terrain[tidx(w, x, y)] = TERR_INLAND_LAKE[0];
    terrain[tidx(w, static_cast<u16>(x + 1u), y)] = TERR_INLAND_LAKE[0];
    terrain[tidx(w, x, static_cast<u16>(y + 1u))] = TERR_INLAND_LAKE[0];
    terrain[tidx(w, static_cast<u16>(x + 1u), static_cast<u16>(y + 1u))] = TERR_INLAND_LAKE[0];
    static const i32 k_rdx[8] = {-1, -1, 2, 2, 0, 1, 0, 1};
    static const i32 k_rdy[8] = {0, 1, 0, 1, -1, -1, 2, 2};
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(x) + k_rdx[k];
        const i32 ny = static_cast<i32>(y) + k_rdy[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ti = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
        if (riv[ti] != 0) {
            terrain[ti] = TERR_INLAND_LAKE[0];
        }
    }
}

//================================================================================================================================
//=> - P1_Adj_RiverLakes -
//================================================================================================================================

P1_Adj_RiverLakes::P1_Adj_RiverLakes (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_adjust(false) {
}

bool P1_Adj_RiverLakes::adjust (u8* terrain, u16 w, u16 h, const u8* riv) {
    m_valid_adjust = false;
    if (terrain == nullptr || riv == nullptr || !p1_map_size_ok(w, h) || w < 2 || h < 2) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    const u16 xmax = static_cast<u16>(w - 2u);
    const u16 ymax = static_cast<u16>(h - 2u);
    for (u16 y = 0; y <= ymax; ++y) {
        for (u16 x = 0; x <= xmax; ++x) {
            if (!quad_ok(terrain, riv, w, x, y)) {
                continue;
            }
            stamp_quad(terrain, riv, w, h, x, y);
        }
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_RiverLakes::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
