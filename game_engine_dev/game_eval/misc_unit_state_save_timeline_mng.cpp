//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "misc_unit_state_save_timeline_mng.h"

#include <cstdio>
#include <cstring>

#include "city.h"
#include "city_array.h"
#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tech_age_mng.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_domain_enum.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void free_seats (PlayerState*& seats, u16& n) {
    if (seats == nullptr) {
        n = 0;
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        delete[] seats[i].m_small_wonder_city;
        seats[i].m_small_wonder_city = nullptr;
        delete seats[i].m_explored_overlay;
        seats[i].m_explored_overlay = nullptr;
        delete seats[i].m_techs_researched;
        seats[i].m_techs_researched = nullptr;
        delete seats[i].m_tech_age;
        seats[i].m_tech_age = nullptr;
        seats[i].m_res_ledger.clear();
    }
    delete[] seats;
    seats = nullptr;
    n = 0;
}

static bool is_land_combat (const RuntimeStatics& st, u16 typ) {
    const u16 n = st.unit().get_item_count();
    if (typ >= n) {
        return false;
    }
    const UnitStaticDataStruct& us = st.unit().get_item(UnitStaticDataKey::from_raw(typ));
    if (us.domain != static_cast<u16>(UnitDomain::LAND)) {
        return false;
    }
    const UnitTypeActionMap& am = st.unit_type_action_map();
    return am.unit_type_can_do(us.type, static_cast<u16>(UnitAction::canAttack))
        || am.unit_type_can_do(us.type, static_cast<u16>(UnitAction::canBarrage));
}

static void mark_cities (const CityArray& cities, u8* city_map, u16 w, u16 h) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(city_map, 0, n);
    const u16 cn = cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        if (x >= w || y >= h) {
            continue;
        }
        city_map[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)] = 1u;
    }
}

static void tally_series (
    const UnitAddVector& units,
    const u8* city_map,
    u16 w,
    u16 h,
    const PlayerState* seats,
    u16 seat_n,
    const RuntimeStatics& st,
    u32* lost,
    u32* camp_nw,
    u16 player_n) {
    for (u16 p = 0; p < player_n; ++p) {
        lost[p] = 0u;
        camp_nw[p] = 0u;
    }
    const u16 head = units.get_head_unit_add_idx();
    for (u16 k = 0; k < head; ++k) {
        const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(k));
        if (u == nullptr) {
            continue;
        }
        const u16 seat = u->m_player_idx;
        if (seat < player_n && seat < seat_n
            && u->m_in_campaign != 0u
            && seats[seat].m_at_war == 0u
            && is_land_combat(st, static_cast<u16>(u->m_unit_typ_idx))) {
            camp_nw[seat] = camp_nw[seat] + 1u;
        }
        if (u->m_x == U16_KEY_NULL || u->m_x >= w || u->m_y >= h) {
            continue;
        }
        if (city_map[static_cast<u32>(u->m_y) * static_cast<u32>(w) + static_cast<u32>(u->m_x)] != 0u) {
            continue;
        }
        UnitAddKey cur = UnitAddKey::from_raw(k);
        while (cur.is_valid()) {
            const UnitAddStruct* v = units.get_unit_add(cur);
            if (v == nullptr) {
                break;
            }
            if (v->m_player_idx < player_n
                && v->m_in_campaign == 0u
                && is_land_combat(st, static_cast<u16>(v->m_unit_typ_idx))) {
                lost[v->m_player_idx] = lost[v->m_player_idx] + 1u;
            }
            if (v->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(v->m_next_unit_in_group);
        }
    }
}

//================================================================================================================================
//=> - MiscUnitStateSaveTimelineMng -
//================================================================================================================================

MiscUnitStateSaveTimelineMng::MiscUnitStateSaveTimelineMng ()
    : m_lucky(nullptr),
      m_player_n(0),
      m_save_n(0) {
    for (u16 i = 0; i < static_cast<u16>(MiscUnitSeries::N); ++i) {
        m_tl[i] = nullptr;
    }
}

MiscUnitStateSaveTimelineMng::~MiscUnitStateSaveTimelineMng () {
    clr();
}

void MiscUnitStateSaveTimelineMng::clr () {
    for (u16 i = 0; i < static_cast<u16>(MiscUnitSeries::N); ++i) {
        delete[] m_tl[i];
        m_tl[i] = nullptr;
    }
    delete[] m_lucky;
    m_lucky = nullptr;
    m_player_n = 0;
    m_save_n = 0;
}

bool MiscUnitStateSaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_player_n = player_n;
    m_save_n = save_n;
    m_lucky = new u8[player_n];
    for (u16 i = 0; i < player_n; ++i) {
        m_lucky[i] = 0u;
    }
    for (u16 s = 0; s < static_cast<u16>(MiscUnitSeries::N); ++s) {
        m_tl[s] = new PlayerUnitSaveTimeline[player_n];
        for (u16 i = 0; i < player_n; ++i) {
            if (!m_tl[s][i].setup(save_n)) {
                clr();
                return false;
            }
        }
    }
    return true;
}

bool MiscUnitStateSaveTimelineMng::fill (const EvalPaths& paths, const RuntimeStatics& st) {
    if (m_tl[0] == nullptr || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    char map_p[512];
    if (!paths.map_path(paths.save_turn_at(0), map_p, sizeof(map_p))) {
        return false;
    }
    GameArraySimple map;
    if (!GameIo::load_map_tiles(map_p, map)) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0u || h == 0u) {
        return false;
    }
    u8* city_map = new u8[static_cast<u32>(w) * static_cast<u32>(h)];
    u32* lost = new u32[m_player_n];
    u32* camp_nw = new u32[m_player_n];
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char units_p[512];
        char cities_p[512];
        char players_p[512];
        if (!paths.units_path(turn, units_p, sizeof(units_p))
            || !paths.cities_path(turn, cities_p, sizeof(cities_p))
            || !paths.players_path(turn, players_p, sizeof(players_p))) {
            delete[] lost;
            delete[] camp_nw;
            delete[] city_map;
            free_seats(seats, seat_n);
            return false;
        }
        seats = nullptr;
        seat_n = 0;
        if (!GameIo::load_players(players_p, seats, seat_n) || seat_n < m_player_n) {
            delete[] lost;
            delete[] camp_nw;
            delete[] city_map;
            free_seats(seats, seat_n);
            return false;
        }
        if (si == 0u && m_lucky != nullptr) {
            for (u16 p = 0; p < m_player_n; ++p) {
                m_lucky[p] = seats[p].m_lucky;
            }
        }
        UnitAddVector units;
        CityArray cities;
        if (!GameIo::load_units(units_p, units) || !GameIo::load_cities(cities_p, cities)) {
            delete[] lost;
            delete[] camp_nw;
            delete[] city_map;
            free_seats(seats, seat_n);
            return false;
        }
        mark_cities(cities, city_map, w, h);
        tally_series(units, city_map, w, h, seats, seat_n, st, lost, camp_nw, m_player_n);
        for (u16 p = 0; p < m_player_n; ++p) {
            if (!m_tl[static_cast<u16>(MiscUnitSeries::LostField)][p].set(si, lost[p])
                || !m_tl[static_cast<u16>(MiscUnitSeries::CampNoWar)][p].set(si, camp_nw[p])) {
                delete[] lost;
                delete[] camp_nw;
                delete[] city_map;
                free_seats(seats, seat_n);
                return false;
            }
        }
    }
    delete[] lost;
    delete[] camp_nw;
    delete[] city_map;
    free_seats(seats, seat_n);
    for (u16 s = 0; s < static_cast<u16>(MiscUnitSeries::N); ++s) {
        for (u16 p = 0; p < m_player_n; ++p) {
            m_tl[s][p].sync_count();
        }
    }
    return true;
}

u16 MiscUnitStateSaveTimelineMng::player_n () const {
    return m_player_n;
}

u16 MiscUnitStateSaveTimelineMng::save_n () const {
    return m_save_n;
}

u8 MiscUnitStateSaveTimelineMng::lucky (u16 seat) const {
    if (m_lucky == nullptr || seat >= m_player_n) {
        return 0u;
    }
    return m_lucky[seat];
}

PlayerUnitSaveTimeline& MiscUnitStateSaveTimelineMng::at (MiscUnitSeries ser, u16 seat) {
    return m_tl[static_cast<u16>(ser)][seat];
}

const PlayerUnitSaveTimeline& MiscUnitStateSaveTimelineMng::at (MiscUnitSeries ser, u16 seat) const {
    return m_tl[static_cast<u16>(ser)][seat];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
