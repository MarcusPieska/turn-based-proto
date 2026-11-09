//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "war_target_policy_sector_flood.h"

#include "city.h"
#include "expansion_state.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "gen_land_sectors.h"
#include "land_sector_adj.h"
#include "log_dbg.h"
#include "sector_support.h"
#include "target_ordering_flood.h"
#include "war_turn_handler.h"

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

bool war_target_flood_sector (GameState& st, u16 seat, u16 sector, WarTargetPlan* io) {
    if (io == nullptr || seat >= st.m_player_n || st.m_player_states == nullptr) {
        return false;
    }
    io->m_n = 0u;
    io->m_enemy = U16_KEY_NULL;
    io->m_sector = sector;
    u16 sx = 0u;
    u16 sy = 0u;
    if (!SectorSupport::find_own_seed(seat, sector, &sx, &sy)) {
        return false;
    }
    const u16 tn = SectorSupport::flood_foreign(
        seat, sector, sx, sy, io->m_cities, WarTargetPlan::k_city_cap);
    if (tn == 0u) {
        return false;
    }
    io->m_n = tn;
    const City* c0 = st.m_cities.get_city(io->m_cities[0]);
    if (c0 == nullptr) {
        io->m_n = 0u;
        return false;
    }
    io->m_enemy = c0->get_owner();
    return true;
}

static u8 flood_skip_reason (GameState& st, u16 seat, u16 sector, WarTargetPlan* io) {
    if (io == nullptr) {
        return 2u;
    }
    io->m_n = 0u;
    io->m_enemy = U16_KEY_NULL;
    io->m_sector = sector;
    u16 sx = 0u;
    u16 sy = 0u;
    if (!SectorSupport::find_own_seed(seat, sector, &sx, &sy)) {
        return 0u;
    }
    const u16 tn = SectorSupport::flood_foreign(
        seat, sector, sx, sy, io->m_cities, WarTargetPlan::k_city_cap);
    if (tn == 0u) {
        return 1u;
    }
    io->m_n = tn;
    const City* c0 = st.m_cities.get_city(io->m_cities[0]);
    if (c0 == nullptr) {
        io->m_n = 0u;
        return 2u;
    }
    io->m_enemy = c0->get_owner();
    return 255u;
}

static void note_select (u16 seat, u16 sector) {
    ExpansionState* exp = WarTurnHandler::sec_exp(seat);
    if (exp == nullptr || !exp->ok() || sector >= exp->sec_n()) {
        return;
    }
    if (!exp->visited(sector)) {
        exp->mark(sector);
        (void)exp->path_push(sector);
    }
    exp->set_cur(sector);
    exp->set_have(false);
}

static bool gather_adj_cands (u16 seat, u16* out, u16 cap, u16* out_n) {
    if (out == nullptr || out_n == nullptr || cap == 0u) {
        return false;
    }
    *out_n = 0u;
    const LandSectorAdj* adj = WarTurnHandler::sec_adj();
    const Whiteboard_2B* wb = WarTurnHandler::sec_wb();
    if (adj == nullptr || wb == nullptr || !wb->ok()) {
        return false;
    }
    const u16 sec_n = adj->sec_n();
    if (sec_n == 0u) {
        return false;
    }
    u8* ctrl = new u8[sec_n];
    u8* seen = new u8[sec_n];
    if (ctrl == nullptr || seen == nullptr) {
        delete[] ctrl;
        delete[] seen;
        return false;
    }
    for (u16 i = 0; i < sec_n; ++i) {
        ctrl[i] = 0u;
        seen[i] = 0u;
    }
    for (u16 i = 0; i < sec_n; ++i) {
        if (SectorSupport::has_own_city(seat, i)) {
            ctrl[i] = 1u;
        }
    }
    for (u16 s = 0; s < sec_n; ++s) {
        if (ctrl[s] == 0u) {
            continue;
        }
        if (seen[s] == 0u && *out_n < cap) {
            seen[s] = 1u;
            out[*out_n] = s;
            *out_n = static_cast<u16>(*out_n + 1u);
        }
        for (u16 e = adj->hd(s); e != U16_KEY_NULL; e = adj->nx(e)) {
            const u16 t = adj->to(e);
            if (t >= sec_n || seen[t] != 0u) {
                continue;
            }
            if (*out_n >= cap) {
                break;
            }
            seen[t] = 1u;
            out[*out_n] = t;
            *out_n = static_cast<u16>(*out_n + 1u);
        }
    }
    delete[] ctrl;
    delete[] seen;
    return *out_n > 0u;
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
    io->m_sector = U16_KEY_NULL;
    u16 cands[512];
    u16 cand_n = 0u;
    u16 skip_n = 0u;
    if (gather_adj_cands(seat, cands, 512u, &cand_n)) {
        for (u16 i = 0; i < cand_n; ++i) {
            const u16 sector = cands[i];
            const u8 reason = flood_skip_reason(st, seat, sector, io);
            if (reason == 255u) {
                note_select(seat, sector);
                LOG_WAR_SECT_SELECT::LOG(SC_U32(seat), SC_U32(sector), SC_U32(st.m_current_turn));
                return true;
            }
            LOG_WAR_SECT_SKIP::LOG(
                SC_U32(seat), SC_U32(sector), SC_U32(st.m_current_turn), SC_U32(reason));
            skip_n = static_cast<u16>(skip_n + 1u);
        }
    }
    LOG_WAR_SECT_EXHAUST::LOG(SC_U32(seat), SC_U32(st.m_current_turn), SC_U32(skip_n));
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
    io->m_sector = U16_KEY_NULL;
    LOG_WAR_SECT_FALLBACK::LOG(
        SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), SC_U32(skip_n));
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
