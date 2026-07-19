//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "city_blocking_mask.h"
#include "circular_tile_areas.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Static data -
//================================================================================================================================

u8 CityBlockingMask::m_buf[CityBlockingMask::m_n] = {};
const u8* const CityBlockingMask::m_mask = CityBlockingMask::m_buf;

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

u8 CityBlockingMask::score (u8 clim, u8 riv) {
    const bool has_riv = riv != 0;
    if (clim == CLIMATE_BLACK_SOIL) {
        return has_riv ? 4u : 3u;
    }
    if (clim == CLIMATE_GRASSLAND) {
        return has_riv ? 3u : 2u;
    }
    if (clim == CLIMATE_PLAINS) {
        return has_riv ? 2u : 1u;
    }
    if (clim == CLIMATE_DESERT) {
        return has_riv ? 1u : 0u;
    }
    return 0;
}

void CityBlockingMask::band (
    const GameArraySimple& map,
    GameArraySimple* out,
    u8* local,
    u16 cx,
    u16 cy,
    u16 w,
    u16 h,
    u16 half,
    u16 i0,
    u16 i1,
    const i8 (*brd)[2],
    u8 omit_ge)
{
    for (u16 i = i0; i < i1; ++i) {
        const i32 dx = static_cast<i32>(brd[i][0]);
        const i32 dy = static_cast<i32>(brd[i][1]);
        const i32 x = static_cast<i32>(cx) + dx;
        const i32 y = static_cast<i32>(cy) + dy;
        if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (score(map.get_climate(ux, uy), map.get_river(ux, uy)) >= omit_ge) {
            continue;
        }
        if (out != nullptr) {
            out->set_settler_blocked(ux, uy, 1u);
        }
        if (local != nullptr) {
            const i32 lx = dx + static_cast<i32>(half);
            const i32 ly = dy + static_cast<i32>(half);
            if (lx >= 0 && ly >= 0 && lx < static_cast<i32>(m_side) && ly < static_cast<i32>(m_side)) {
                local[static_cast<u32>(ly) * static_cast<u32>(m_side) + static_cast<u32>(lx)] = 1u;
            }
        }
    }
}

void CityBlockingMask::run (const GameArraySimple& map, GameArraySimple* out, u16 cx, u16 cy, u8* local) {
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0 || h == 0 || cx >= w || cy >= h) {
        return;
    }
    const u16 half = static_cast<u16>(m_side / 2u);
    const CircArea core = CircularTileAreas::get(m_r_core);
    if (core.m_lim == 0 || core.m_brd == nullptr) {
        return;
    }
    band(map, out, local, cx, cy, w, h, half, 0, core.m_lim, core.m_brd, 255u);
    u16 r_prev = m_r_core;
    for (u16 k = 0; k < m_r_gate_n; ++k) {
        const u16 r = m_r_gate[k];
        if (r <= r_prev) {
            continue;
        }
        const CircArea prev = CircularTileAreas::get(r_prev);
        const CircArea cur = CircularTileAreas::get(r);
        if (cur.m_lim == 0 || cur.m_brd == nullptr || prev.m_lim > cur.m_lim) {
            continue;
        }
        band(map, out, local, cx, cy, w, h, half, prev.m_lim, cur.m_lim, cur.m_brd, m_omit_ge[k]);
        r_prev = r;
    }
}

//================================================================================================================================
//=> - CityBlockingMask -
//================================================================================================================================

void CityBlockingMask::stamp (GameArraySimple& map, u16 cx, u16 cy) {
    run(map, &map, cx, cy, nullptr);
}

const u8* CityBlockingMask::preview (const GameArraySimple& map, u16 cx, u16 cy) {
    std::memset(m_buf, 0, sizeof(m_buf));
    run(map, nullptr, cx, cy, m_buf);
    return m_mask;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
