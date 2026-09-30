//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_muster_save_timeline_mng.h"

#include <cstdio>

#include "city.h"
#include "eval_paths.h"
#include "game_io.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 seat_muster_sz (GameState& st, u16 seat) {
    u32 tot = 0;
    static const u16 k_cap = 64u;
    UnitAddKey keys[k_cap];
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        u16 n = 0;
        if (!UnitMovementMng::muster_collect_depart(st, c->get_x(), c->get_y(), seat, keys, k_cap, &n)) {
            continue;
        }
        for (u16 k = 0; k < n; ++k) {
            const UnitAddStruct* u = st.m_units.get_unit_add(keys[k]);
            if (u == nullptr) {
                continue;
            }
            tot = tot + 1u + static_cast<u32>(u->m_unit_size);
        }
    }
    return tot;
}

//================================================================================================================================
//=> - PlayerMusterSaveTimelineMng -
//================================================================================================================================

PlayerMusterSaveTimelineMng::PlayerMusterSaveTimelineMng ()
    : m_tl(nullptr),
      m_seat(nullptr),
      m_player_n(0),
      m_save_n(0) {
}

PlayerMusterSaveTimelineMng::~PlayerMusterSaveTimelineMng () {
    clr();
}

void PlayerMusterSaveTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    delete[] m_seat;
    m_seat = nullptr;
    m_player_n = 0;
    m_save_n = 0;
}

bool PlayerMusterSaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_tl = new PlayerUnitSaveTimeline[player_n];
    m_seat = new u16[player_n];
    m_player_n = player_n;
    m_save_n = save_n;
    for (u16 i = 0; i < player_n; ++i) {
        if (!m_tl[i].setup(save_n)) {
            clr();
            return false;
        }
        m_seat[i] = i;
    }
    return true;
}

bool PlayerMusterSaveTimelineMng::fill (const EvalPaths& paths, const RuntimeStatics& st) {
    if (m_tl == nullptr || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char map_p[512];
        char units_p[512];
        char cities_p[512];
        if (!paths.map_path(turn, map_p, sizeof(map_p))
            || !paths.units_path(turn, units_p, sizeof(units_p))
            || !paths.cities_path(turn, cities_p, sizeof(cities_p))) {
            return false;
        }
        GameState gs;
        gs.m_statics = &st;
        gs.m_player_n = m_player_n;
        if (!GameIo::load_map_tiles(map_p, gs.m_map)
            || !GameIo::load_units(units_p, gs.m_units)
            || !GameIo::load_cities(cities_p, gs.m_cities)) {
            return false;
        }
        for (u16 p = 0; p < m_player_n; ++p) {
            if (!m_tl[p].set(si, seat_muster_sz(gs, p))) {
                return false;
            }
        }
    }
    for (u16 p = 0; p < m_player_n; ++p) {
        m_tl[p].sync_count();
    }
    return true;
}

void PlayerMusterSaveTimelineMng::sort () {
    if (m_tl == nullptr || m_player_n < 2) {
        return;
    }
    for (u16 i = 0; i < m_player_n; ++i) {
        u16 best = i;
        for (u16 j = static_cast<u16>(i + 1u); j < m_player_n; ++j) {
            if (m_tl[j].count() > m_tl[best].count()) {
                best = j;
            }
        }
        if (best != i) {
            m_tl[i].swap(m_tl[best]);
            const u16 s = m_seat[i];
            m_seat[i] = m_seat[best];
            m_seat[best] = s;
        }
    }
}

u16 PlayerMusterSaveTimelineMng::player_n () const {
    return m_player_n;
}

u16 PlayerMusterSaveTimelineMng::save_n () const {
    return m_save_n;
}

PlayerUnitSaveTimeline& PlayerMusterSaveTimelineMng::at (u16 i) {
    return m_tl[i];
}

const PlayerUnitSaveTimeline& PlayerMusterSaveTimelineMng::at (u16 i) const {
    return m_tl[i];
}

u16 PlayerMusterSaveTimelineMng::seat (u16 i) const {
    if (m_seat == nullptr || i >= m_player_n) {
        return UINT16_MAX;
    }
    return m_seat[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
