//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "war_target_secure_capital.h"

#include <cstring>

#include "city.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const i8 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i8 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool walk_ok (const GameState& st, u16 x, u16 y) {
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

static bool enemy_ok (const GameState& st, u16 seat, u16 locked, u16 owner) {
    if (owner == seat || owner >= st.m_player_n || st.m_player_states == nullptr) {
        return false;
    }
    if (st.m_player_states[owner].m_lucky != 0u || st.m_player_states[owner].m_is_active == 0u) {
        return false;
    }
    if (locked != U16_KEY_NULL && locked != owner) {
        return false;
    }
    return true;
}

static bool corridor_ok (const GameState& st, u16 seat, u16 enemy, u16 x, u16 y) {
    if (!walk_ok(st, x, y)) {
        return false;
    }
    const u8 o = st.m_map.get_civ_owner(x, y);
    if (o == U8_KEY_NULL || o == static_cast<u8>(seat) || o == static_cast<u8>(enemy)) {
        return true;
    }
    return false;
}

static bool find_near_foreign (
    GameState& st,
    u16 seat,
    u16 locked,
    u16 sx,
    u16 sy,
    u16* out_enemy,
    u16* out_cx,
    u16* out_cy) {
    if (out_enemy == nullptr || out_cx == nullptr || out_cy == nullptr) {
        return false;
    }
    const u16 w = st.m_map.width();
    const u16 h = st.m_map.height();
    if (sx >= w || sy >= h || !walk_ok(st, sx, sy)) {
        return false;
    }
    if (WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        return false;
    }
    Whiteboard_1B seen("WarTargetSecureCap", "seen", 0u);
    Whiteboard_2B cit("WarTargetSecureCap", "cit", 0u);
    Whiteboard_4B que("WarTargetSecureCap", "que", 0u);
    if (!seen.ok() || !cit.ok() || !que.ok()) {
        return false;
    }
    const u32 tn = st.m_map.tile_n();
    std::memset(seen.raw(), 0, static_cast<size_t>(tn));
    for (u32 i = 0; i < tn; ++i) {
        cit.wr_i(i, U16_KEY_NULL);
    }
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = st.m_cities.get_city(i);
        if (c == nullptr) {
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
    que.wr_i(qt++, static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx));
    seen.wr(sx, sy, 1u);
    while (qh < qt) {
        const u32 i = que.rd_i(qh++);
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u16 cidx = cit.rd(x, y);
        if (cidx != U16_KEY_NULL) {
            City* c = st.m_cities.get_city(cidx);
            if (c != nullptr && enemy_ok(st, seat, locked, c->get_owner())) {
                *out_enemy = c->get_owner();
                *out_cx = x;
                *out_cy = y;
                return true;
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
            if (seen.rd(ux, uy) != 0u || !walk_ok(st, ux, uy)) {
                continue;
            }
            seen.wr(ux, uy, 1u);
            que.wr_i(qt++, static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux));
        }
    }
    return false;
}

static u16 fill_enemy_from (
    GameState& st,
    u16 seat,
    u16 enemy,
    u16 sx,
    u16 sy,
    u16* out,
    u16 cap) {
    if (out == nullptr || cap == 0u) {
        return 0u;
    }
    const u16 w = st.m_map.width();
    const u16 h = st.m_map.height();
    if (sx >= w || sy >= h || !corridor_ok(st, seat, enemy, sx, sy)) {
        return 0u;
    }
    if (WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        return 0u;
    }
    Whiteboard_1B seen("WarTargetSecureCap", "seen2", 0u);
    Whiteboard_2B cit("WarTargetSecureCap", "cit2", 0u);
    Whiteboard_4B que("WarTargetSecureCap", "que2", 0u);
    if (!seen.ok() || !cit.ok() || !que.ok()) {
        return 0u;
    }
    const u32 tn = st.m_map.tile_n();
    std::memset(seen.raw(), 0, static_cast<size_t>(tn));
    for (u32 i = 0; i < tn; ++i) {
        cit.wr_i(i, U16_KEY_NULL);
    }
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != enemy) {
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
    que.wr_i(qt++, static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx));
    seen.wr(sx, sy, 1u);
    u16 n = 0u;
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
            if (seen.rd(ux, uy) != 0u || !corridor_ok(st, seat, enemy, ux, uy)) {
                continue;
            }
            seen.wr(ux, uy, 1u);
            que.wr_i(qt++, static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux));
        }
    }
    return n;
}

//================================================================================================================================
//=> - war_target_secure_capital -
//================================================================================================================================

bool war_target_secure_capital (
    GameState& st,
    u16 seat,
    u16 from_x,
    u16 from_y,
    WarTargetPlan* io) {
    if (io == nullptr || seat >= st.m_player_n || st.m_player_states == nullptr) {
        return false;
    }
    const u16 locked = io->m_enemy;
    io->m_n = 0u;
    u16 enemy = U16_KEY_NULL;
    u16 cx = 0u;
    u16 cy = 0u;
    if (!find_near_foreign(st, seat, locked, from_x, from_y, &enemy, &cx, &cy)) {
        io->m_enemy = U16_KEY_NULL;
        return false;
    }
    io->m_n = fill_enemy_from(st, seat, enemy, cx, cy, io->m_cities, WarTargetPlan::k_city_cap);
    if (io->m_n == 0u) {
        io->m_enemy = U16_KEY_NULL;
        return false;
    }
    io->m_enemy = enemy;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
