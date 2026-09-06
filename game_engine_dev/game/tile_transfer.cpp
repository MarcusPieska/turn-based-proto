//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tile_transfer.h"

#include "build_adds_array.h"
#include "city.h"
#include "city_border.h"
#include "circular_tile_areas.h"
#include "game_array_simple.h"
#include "game_state.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static constexpr u16 k_r_tab = 20u;
static constexpr u16 k_near_cap = 512u;
static constexpr u16 k_claim_cult = 25u;

struct NearCity {
    u16 m_idx;
    u16 m_x;
    u16 m_y;
    u16 m_r;
    u8 m_own;
};

static i32 dist2 (i32 x0, i32 y0, i32 x1, i32 y1) {
    const i32 dx = x1 - x0;
    const i32 dy = y1 - y0;
    return dx * dx + dy * dy;
}

static u16 cult_r (u16 culture) {
    u16 r = CityBorder::radius_for(culture);
    const u16 r_floor = CityBorder::radius_for(k_claim_cult);
    if (r < r_floor) {
        r = r_floor;
    }
    if (r > k_r_tab) {
        r = k_r_tab;
    }
    return r;
}

static bool near_has (const NearCity* near, u16 near_n, u16 cidx) {
    for (u16 i = 0; i < near_n; ++i) {
        if (near[i].m_idx == cidx) {
            return true;
        }
    }
    return false;
}

static bool in_rng (i32 tx, i32 ty, i32 ox, i32 oy, u16 R) {
    const i32 dx = tx - ox;
    const i32 dy = ty - oy;
    if (dx == 0 && dy == 0) {
        return true;
    }
    if (R == 0) {
        return false;
    }
    const i32 adx = dx < 0 ? -dx : dx;
    const i32 ady = dy < 0 ? -dy : dy;
    if (adx > static_cast<i32>(R) || ady > static_cast<i32>(R)) {
        return false;
    }
    const CircArea area = CircularTileAreas::get(R);
    for (u16 i = 0; i < area.m_lim; ++i) {
        if (static_cast<i32>(area.m_brd[i][0]) == dx && static_cast<i32>(area.m_brd[i][1]) == dy) {
            return true;
        }
    }
    return false;
}

static bool tile_wins (
    i32 tx,
    i32 ty,
    i32 cx,
    i32 cy,
    const NearCity* near,
    u16 near_n,
    u16 self_idx,
    u8 from_owner) {
    const i32 d_us = dist2(tx, ty, cx, cy);
    for (u16 i = 0; i < near_n; ++i) {
        if (near[i].m_idx == self_idx) {
            continue;
        }
        const i32 ox = static_cast<i32>(near[i].m_x);
        const i32 oy = static_cast<i32>(near[i].m_y);
        if (!in_rng(tx, ty, ox, oy, near[i].m_r)) {
            continue;
        }
        const i32 d_ot = dist2(tx, ty, ox, oy);
        if (d_ot >= d_us) {
            continue;
        }
        if (near[i].m_own != from_owner) {
            continue;
        }
        return false;
    }
    return true;
}

static void try_add (
    GameState& state,
    u16 ux,
    u16 uy,
    u16 self_idx,
    NearCity* near,
    u16* near_n) {
    if (*near_n >= k_near_cap) {
        return;
    }
    if (state.m_map.get_add_typ(ux, uy) != BUILD_ADD_CITY) {
        return;
    }
    const u16 cidx = state.m_map.get_add_idx(ux, uy);
    const u16 cn = state.m_cities.get_city_count();
    if (cidx == self_idx || cidx >= cn || near_has(near, *near_n, cidx)) {
        return;
    }
    City* c = state.m_cities.get_city(cidx);
    if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
        return;
    }
    near[*near_n].m_idx = cidx;
    near[*near_n].m_x = c->get_x();
    near[*near_n].m_y = c->get_y();
    near[*near_n].m_r = cult_r(c->get_current_culture());
    near[*near_n].m_own = static_cast<u8>(c->get_owner());
    *near_n = static_cast<u16>(*near_n + 1u);
}

static void collect_in_disc (
    GameState& state,
    u16 cx,
    u16 cy,
    u16 r,
    u16 self_idx,
    NearCity* near,
    u16* near_n) {
    if (r == 0 || *near_n >= k_near_cap) {
        return;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    if (r <= k_r_tab) {
        const CircArea disc = CircularTileAreas::get(r);
        for (u16 i = 0; i < disc.m_lim; ++i) {
            const i32 x = static_cast<i32>(cx) + static_cast<i32>(disc.m_brd[i][0]);
            const i32 y = static_cast<i32>(cy) + static_cast<i32>(disc.m_brd[i][1]);
            if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                continue;
            }
            try_add(state, static_cast<u16>(x), static_cast<u16>(y), self_idx, near, near_n);
        }
        return;
    }
    const i32 ir = static_cast<i32>(r);
    const i32 r2 = ir * ir;
    for (i32 dy = -ir; dy <= ir; ++dy) {
        for (i32 dx = -ir; dx <= ir; ++dx) {
            if (dx * dx + dy * dy > r2) {
                continue;
            }
            const i32 x = static_cast<i32>(cx) + dx;
            const i32 y = static_cast<i32>(cy) + dy;
            if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                continue;
            }
            try_add(state, static_cast<u16>(x), static_cast<u16>(y), self_idx, near, near_n);
        }
    }
}

//================================================================================================================================
//=> - TileTransfer -
//================================================================================================================================

u32 TileTransfer::apply (GameState& state, u16 city_idx, u8 from_owner, u8 to_owner, u16* out_near) {
    if (out_near != nullptr) {
        *out_near = 0;
    }
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return 0;
    }
    const u16 cx = city->get_x();
    const u16 cy = city->get_y();
    if (cx == U16_KEY_NULL || cy == U16_KEY_NULL) {
        return 0;
    }
    const u16 R = cult_r(city->get_current_culture());
    state.m_map.set_civ_owner(cx, cy, to_owner);

    NearCity near[k_near_cap];
    u16 near_n = 0;
    const u16 r_search = static_cast<u16>((static_cast<u32>(R) * 3u) / 2u);
    for (u16 r = 1; r <= r_search; ++r) {
        collect_in_disc(state, cx, cy, r, city_idx, near, &near_n);
    }
    if (out_near != nullptr) {
        *out_near = near_n;
    }

    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    u32 tot = 0;
    for (u16 r = 1; r <= R; ++r) {
        const CircArea cur = CircularTileAreas::get(r);
        u16 i0 = 0;
        if (r > 0) {
            i0 = CircularTileAreas::get(static_cast<u16>(r - 1u)).m_lim;
        }
        u32 ring_n = 0;
        for (u16 i = i0; i < cur.m_lim; ++i) {
            const i32 x = static_cast<i32>(cx) + static_cast<i32>(cur.m_brd[i][0]);
            const i32 y = static_cast<i32>(cy) + static_cast<i32>(cur.m_brd[i][1]);
            if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(x);
            const u16 uy = static_cast<u16>(y);
            if (state.m_map.get_add_typ(ux, uy) == BUILD_ADD_CITY) {
                continue;
            }
            if (state.m_map.get_civ_owner(ux, uy) != from_owner) {
                continue;
            }
            if (!tile_wins(x, y, static_cast<i32>(cx), static_cast<i32>(cy), near, near_n, city_idx, from_owner)) {
                continue;
            }
            state.m_map.set_civ_owner(ux, uy, to_owner);
            ++ring_n;
        }
        tot = static_cast<u32>(tot + ring_n);
    }
    return tot;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
