//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_ensure_adj_rules.h"

#include "game_map_defs.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static const i32 k_dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i32 k_dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w) && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool enq ( WB_QueXY& que, u16* pend, u16 w, u16 x, u16 y, u32* que_peak) {
    const u32 i = tidx(w, x, y);
    if (pend[i] != 0) {
        return true;
    }
    pend[i] = 1;
    if (!que.push(x, y)) {
        pend[i] = 0;
        return false;
    }
    const u32 qn = que.count();
    if (qn > *que_peak) {
        *que_peak = qn;
    }
    return true;
}

static bool set_terr (u8* terrain, u16 w, u16 x, u16 y, u8 cls, WB_QueXY& que, u16* pend, u32* terr_chg, u32* que_peak) {
    const u32 i = tidx(w, x, y);
    if (terrain[i] == cls) {
        return true;
    }
    terrain[i] = cls;
    ++(*terr_chg);
    return enq(que, pend, w, x, y, que_peak);
}

static bool set_clim ( u8* climate, u16 w, u16 x, u16 y, u8 cls, WB_QueXY& que, u16* pend, u32* clim_chg, u32* que_peak) {
    const u32 i = tidx(w, x, y);
    if (climate[i] == cls) {
        return true;
    }
    climate[i] = cls;
    ++(*clim_chg);
    return enq(que, pend, w, x, y, que_peak);
}

static bool apply_rules_at (
    u8* terrain,
    u8* climate,
    const u8* river,
    u16 w,
    u16 h,
    u16 cx,
    u16 cy,
    WB_QueXY& que,
    u16* pend,
    u32* terr_chg,
    u32* clim_chg,
    u32* que_peak) 
{
    const u32 ci = tidx(w, cx, cy);
    const u8 ter = terrain[ci];
    const u8 clim = climate[ci];
    const bool riv = (river != nullptr && river[ci] != 0);
    if (clim == CLIMATE_BLACK_SOIL && (ter == TERR_HILLS[0] || ter == TERR_MOUNTAINS[0])) {
        if (!set_terr(terrain, w, cx, cy, TERR_PLAINS[0], que, pend, terr_chg, que_peak)) {
            return false;
        }
    }
    /*
    if (riv && is_land(terrain[ci]) && terrain[ci] != TERR_PLAINS[0]) {
        if (!set_terr(terrain, w, cx, cy, TERR_PLAINS[0], que, pend, terr_chg, que_peak)) {
            return false;
        }
    }
    */
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(cx) + k_dx[k];
        const i32 ny = static_cast<i32>(cy) + k_dy[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u16 ax = static_cast<u16>(nx);
        const u16 ay = static_cast<u16>(ny);
        const u32 ni = tidx(w, ax, ay);
        const u8 nter = terrain[ni];
        const u8 nclim = climate[ni];
        if (ter == TERR_COASTAL[0] && nter == TERR_MOUNTAINS[0]) {
            if (!set_terr(terrain, w, ax, ay, TERR_HILLS[0], que, pend, terr_chg, que_peak)) {
                return false;
            }
        }
        if (ter == TERR_COASTAL[0] && nter == TERR_OCEAN[0]) {
            if (!set_terr(terrain, w, ax, ay, TERR_SEA[0], que, pend, terr_chg, que_peak)) {
                return false;
            }
        }
        if (is_land(ter) && (nter == TERR_OCEAN[0] || nter == TERR_SEA[0])) {
            if (!set_terr(terrain, w, ax, ay, TERR_COASTAL[0], que, pend, terr_chg, que_peak)) {
                return false;
            }
        }
        /*
        if (riv && is_land(nter) && nter != TERR_PLAINS[0]) {
            if (!set_terr(terrain, w, ax, ay, TERR_PLAINS[0], que, pend, terr_chg, que_peak)) {
                return false;
            }
        }
        */
        if (ter == TERR_PLAINS[0] && nter == TERR_MOUNTAINS[0]) {
            if (!set_terr(terrain, w, ax, ay, TERR_HILLS[0], que, pend, terr_chg, que_peak)) {
                return false;
            }
        }
        if (clim == CLIMATE_BLACK_SOIL && nclim != CLIMATE_BLACK_SOIL && is_land(nter)) {
            if (!set_clim(climate, w, ax, ay, CLIMATE_GRASSLAND, que, pend, clim_chg, que_peak)) {
                return false;
            }
        }
        if (clim == CLIMATE_GRASSLAND && nclim == CLIMATE_DESERT && is_land(nter)) {
            if (!set_clim(climate, w, ax, ay, CLIMATE_PLAINS, que, pend, clim_chg, que_peak)) {
                return false;
            }
        }
    }
    return true;
}

static bool apply_ensure_adj_rules (
    u8* terrain,
    u8* climate,
    const u8* river,
    u16 w,
    u16 h,
    u32* terr_chg,
    u32* clim_chg,
    u32* que_peak) 
{
    if (terrain == nullptr || climate == nullptr || !p1_map_size_ok(w, h)
        || terr_chg == nullptr || clim_chg == nullptr || que_peak == nullptr) {
        return false;
    }
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_pend("P1_Adj_EnsureAdjRules", "pend", 0u);
    P1_WB_CHK(wb_pend);
    WB_QueXY que;
    if (!que.ok()) {
        return false;
    }
    u16* pend = wb_pend.get_iter_ptr();
    for (u32 i = 0; i < tile_n; ++i) {
        pend[i] = 0;
    }
    *terr_chg = 0;
    *clim_chg = 0;
    *que_peak = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!apply_rules_at(terrain, climate, river, w, h, x, y, que, pend,
                    terr_chg, clim_chg, que_peak)) {
                return false;
            }
        }
    }
    while (que.count() > 0) {
        const u16 x = que.x_at(0);
        const u16 y = que.y_at(0);
        que.drop(1);
        pend[tidx(w, x, y)] = 0;
        if (!apply_rules_at(terrain, climate, river, w, h, x, y, que, pend,
                terr_chg, clim_chg, que_peak)) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - P1_Adj_EnsureAdjRules -
//================================================================================================================================

P1_Adj_EnsureAdjRules::P1_Adj_EnsureAdjRules (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_adjust(false),
    m_terr_chg_n(0),
    m_clim_chg_n(0),
    m_que_peak_n(0) {
}

bool P1_Adj_EnsureAdjRules::adjust (u8* terrain, u8* climate, const u8* river, u16 w,  u16 h) {
    m_valid_adjust = false;
    m_terr_chg_n = 0;
    m_clim_chg_n = 0;
    m_que_peak_n = 0;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || climate == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!apply_ensure_adj_rules(terrain, climate, river, w, h,
            &m_terr_chg_n, &m_clim_chg_n, &m_que_peak_n)) {
        return false;
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_EnsureAdjRules::is_valid () const {
    return m_valid_adjust;
}

u32 P1_Adj_EnsureAdjRules::terr_chg_n () const {
    return m_terr_chg_n;
}

u32 P1_Adj_EnsureAdjRules::clim_chg_n () const {
    return m_clim_chg_n;
}

u32 P1_Adj_EnsureAdjRules::que_peak_n () const {
    return m_que_peak_n;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
