//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "target_ordering_flood.h"

#include <cstring>

#include "city.h"
#include "city_array.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const i8 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i8 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

//================================================================================================================================
//=> - TargetOrderingFlood -
//================================================================================================================================

TargetOrderingFlood::TargetOrderingFlood ()
    : m_enemy(U8_KEY_NULL)
    , m_own(U8_KEY_NULL)
    , m_neut(false) {
}

void TargetOrderingFlood::set_enemy (u8 seat) {
    m_enemy = seat;
}

void TargetOrderingFlood::set_own (u8 seat) {
    m_own = seat;
}

void TargetOrderingFlood::set_neutral (bool on) {
    m_neut = on;
}

bool TargetOrderingFlood::own_ok (u8 o) const {
    if (m_neut && o == U8_KEY_NULL) {
        return true;
    }
    if (m_enemy != U8_KEY_NULL && o == m_enemy) {
        return true;
    }
    if (m_own != U8_KEY_NULL && o == m_own) {
        return true;
    }
    return false;
}

bool TargetOrderingFlood::walk_ok (const GameState& st, u16 x, u16 y) const {
    const u8 t = st.m_map.get_terrain(x, y);
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
        return false;
    }
    return true;
}

u16 TargetOrderingFlood::fill (GameState& st, u16 sx, u16 sy, u16* out, u16 cap) {
    if (out == nullptr || cap == 0u) {
        return 0;
    }
    const u16 w = st.m_map.width();
    const u16 h = st.m_map.height();
    if (sx >= w || sy >= h) {
        return 0;
    }
    if (!walk_ok(st, sx, sy) || !own_ok(st.m_map.get_civ_owner(sx, sy))) {
        return 0;
    }
    if (WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        return 0;
    }
    Whiteboard_1B seen("TargetOrderingFlood", "seen", 0u);
    Whiteboard_2B cit("TargetOrderingFlood", "cit", 0u);
    Whiteboard_4B que("TargetOrderingFlood", "que", 0u);
    if (!seen.ok() || !cit.ok() || !que.ok()) {
        return 0;
    }
    const u32 tn = st.m_map.tile_n();
    std::memset(seen.raw(), 0, static_cast<size_t>(tn));
    for (u32 i = 0; i < tn; ++i) {
        cit.wr_i(i, U16_KEY_NULL);
    }
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = st.m_cities.get_city(i);
        if (c == nullptr || !own_ok(static_cast<u8>(c->get_owner()))) {
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        if (cx >= w || cy >= h) {
            continue;
        }
        cit.wr(cx, cy, i);
    }
    u32 qh = 0;
    u32 qt = 0;
    const u32 sidx = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
    que.wr_i(qt++, sidx);
    seen.wr(sx, sy, 1u);
    u16 n = 0;
    while (qh < qt && n < cap) {
        const u32 i = que.rd_i(qh++);
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u16 cidx = cit.rd(x, y);
        if (cidx != U16_KEY_NULL) {
            out[n] = cidx;
            n = static_cast<u16>(n + 1u);
            if (n >= cap) {
                return n;
            }
        }
        for (u8 d = 0; d < 8u; ++d) {
            const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_dx[d]);
            const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_dy[d]);
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (seen.rd(ux, uy) != 0u) {
                continue;
            }
            if (!walk_ok(st, ux, uy) || !own_ok(st.m_map.get_civ_owner(ux, uy))) {
                continue;
            }
            seen.wr(ux, uy, 1u);
            que.wr_i(qt++, static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux));
        }
    }
    return n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
