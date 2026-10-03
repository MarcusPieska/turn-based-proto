//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "war_target_policy_sector_flood.h"

#include "city.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "target_ordering_flood.h"
#include "target_sector_defensible.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool walk_ok_tile (const GameState& st, u16 x, u16 y) {
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

static bool seat_cap_xy (const GameState& st, u16 seat, u16* ox, u16* oy) {
    if (ox == nullptr || oy == nullptr) {
        return false;
    }
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        *ox = c->get_x();
        *oy = c->get_y();
        return true;
    }
    return false;
}

static bool pick_near_non_lucky (const GameState& st, u16 seat, u16 from_x, u16 from_y, u16* out_enemy) {
    if (out_enemy == nullptr || st.m_player_states == nullptr) {
        return false;
    }
    u32 best_d = 0xFFFFFFFFu;
    u16 best = U16_KEY_NULL;
    for (u16 e = 0; e < st.m_player_n; ++e) {
        if (e == seat || st.m_player_states[e].m_lucky != 0u) {
            continue;
        }
        if (st.m_player_states[e].m_is_active == 0u) {
            continue;
        }
        u16 ex = 0u;
        u16 ey = 0u;
        if (!seat_cap_xy(st, e, &ex, &ey)) {
            continue;
        }
        const u32 adx = from_x > ex ? static_cast<u32>(from_x - ex) : static_cast<u32>(ex - from_x);
        const u32 ady = from_y > ey ? static_cast<u32>(from_y - ey) : static_cast<u32>(ey - from_y);
        const u32 d = adx + ady;
        if (d < best_d) {
            best_d = d;
            best = e;
        }
    }
    if (best == U16_KEY_NULL) {
        return false;
    }
    *out_enemy = best;
    return true;
}

static bool find_enemy_seed (
    const GameState& st,
    u16 enemy,
    u16 from_x,
    u16 from_y,
    u16* ox,
    u16* oy) {
    if (ox == nullptr || oy == nullptr) {
        return false;
    }
    const u16 cn = st.m_cities.get_city_count();
    u32 best = 0xFFFFFFFFu;
    u16 bx = 0;
    u16 by = 0;
    bool found = false;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != enemy) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        if (!walk_ok_tile(st, x, y)) {
            continue;
        }
        const u32 adx = from_x > x ? static_cast<u32>(from_x - x) : static_cast<u32>(x - from_x);
        const u32 ady = from_y > y ? static_cast<u32>(from_y - y) : static_cast<u32>(y - from_y);
        const u32 d = adx + ady;
        if (!found || d < best) {
            best = d;
            bx = x;
            by = y;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

static bool fill_enemy_cities (GameState& st, u16 enemy, u16 from_x, u16 from_y, WarTargetPlan* io) {
    u16 sx = 0;
    u16 sy = 0;
    if (!find_enemy_seed(st, enemy, from_x, from_y, &sx, &sy)) {
        return false;
    }
    TargetOrderingFlood flood;
    flood.set_enemy(static_cast<u8>(enemy));
    io->m_n = flood.fill(st, sx, sy, io->m_cities, WarTargetPlan::k_city_cap);
    return io->m_n > 0u;
}

//================================================================================================================================
//=> - war_target_pick_sector_flood -
//================================================================================================================================

bool war_target_pick_sector_flood (
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
    u16 sector = U16_KEY_NULL;
    u16 out_e = U16_KEY_NULL;
    u16 tn = 0u;
    if (TargetSector_Defensible::pick(seat, io->m_cities, WarTargetPlan::k_city_cap, &tn, &sector, &out_e)
        && tn > 0u
        && out_e < st.m_player_n
        && out_e != seat
        && st.m_player_states[out_e].m_lucky == 0u
        && st.m_player_states[out_e].m_is_active != 0u
        && (locked == U16_KEY_NULL || locked == out_e)) {
        io->m_enemy = out_e;
        io->m_n = tn;
        return true;
    }
    u16 enemy = locked;
    if (enemy == U16_KEY_NULL) {
        if (!pick_near_non_lucky(st, seat, from_x, from_y, &enemy)) {
            io->m_enemy = U16_KEY_NULL;
            return false;
        }
    }
    if (!fill_enemy_cities(st, enemy, from_x, from_y, io)) {
        io->m_enemy = U16_KEY_NULL;
        io->m_n = 0u;
        return false;
    }
    io->m_enemy = enemy;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
