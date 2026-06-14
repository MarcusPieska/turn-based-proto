//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "adjust_river_fragmen_subs.h"

#include "generator_constants.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static void land_riv_nbr (
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u16 x,
    u16 y,
    u16* cnt,
    u16* nx,
    u16* ny) 
{
    *cnt = 0;
    *nx = x;
    *ny = y;
    if (riv[tidx(w, x, y)] == 0) {
        return;
    }
    for (i32 k = 0; k < 4; ++k) {
        const i32 ax = static_cast<i32>(x) + k_dx4[k];
        const i32 ay = static_cast<i32>(y) + k_dy4[k];
        if (!in_map(w, h, ax, ay)) {
            continue;
        }
        const u16 ux = static_cast<u16>(ax);
        const u16 uy = static_cast<u16>(ay);
        const u32 ni = tidx(w, ux, uy);
        if (riv[ni] == 0 || is_water(terrain[ni])) {
            continue;
        }
        *cnt = static_cast<u16>(*cnt + 1u);
        *nx = ux;
        *ny = uy;
    }
}

//================================================================================================================================
//=> - RiverFragmenSubs -
//================================================================================================================================

Adjust_RiverFragmenSubs::Adjust_RiverFragmenSubs (u32 seed) :
    m_seed(seed),
    m_valid_adjust(false) {
}

bool Adjust_RiverFragmenSubs::adjust (u8* terrain, const u8* riv, u16 w, u16 h) {
    m_valid_adjust = false;
    if (terrain == nullptr || riv == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* mark = new u8[n];
    if (mark == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        mark[i] = 0;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            if (riv[i] == 0) {
                continue;
            }
            u16 cnt = 0;
            u16 ax = x;
            u16 ay = y;
            land_riv_nbr(terrain, riv, w, h, x, y, &cnt, &ax, &ay);
            if (cnt == 1) {
                u16 cnt2 = 0;
                u16 bx = ax;
                u16 by = ay;
                land_riv_nbr(terrain, riv, w, h, ax, ay, &cnt2, &bx, &by);
                if (cnt2 == 1 && bx == x && by == y) {
                    mark[i] = 1;
                    mark[tidx(w, ax, ay)] = 1;
                }
                continue;
            }
            if (cnt == 0) {
                mark[i] = 1;
            }
        }
    }
    for (u32 i = 0; i < n; ++i) {
        if (mark[i] != 0) {
            terrain[i] = TERR_COASTAL[0];
        }
    }
    delete[] mark;
    m_valid_adjust = true;
    (void)m_seed;
    return true;
}

bool Adjust_RiverFragmenSubs::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
