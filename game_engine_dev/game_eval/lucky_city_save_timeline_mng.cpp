//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_city_save_timeline_mng.h"

#include <cstdio>

#include "city.h"
#include "city_array.h"
#include "eval_paths.h"
#include "game_io.h"
#include "game_state.h"
#include "tech_age_mng.h"

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

//================================================================================================================================
//=> - LuckyCitySaveTimelineMng -
//================================================================================================================================

LuckyCitySaveTimelineMng::LuckyCitySaveTimelineMng ()
    : m_tl(nullptr),
      m_seat(nullptr),
      m_lucky_n(0),
      m_player_n(0),
      m_save_n(0) {
}

LuckyCitySaveTimelineMng::~LuckyCitySaveTimelineMng () {
    clr();
}

void LuckyCitySaveTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    delete[] m_seat;
    m_seat = nullptr;
    m_lucky_n = 0;
    m_player_n = 0;
    m_save_n = 0;
}

bool LuckyCitySaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_player_n = player_n;
    m_save_n = save_n;
    return true;
}

bool LuckyCitySaveTimelineMng::fill (const EvalPaths& paths) {
    if (!paths.ok() || paths.save_turn_n() != m_save_n || m_player_n == 0) {
        return false;
    }
    char players_p[512];
    if (!paths.players_path(paths.save_turn_at(0), players_p, sizeof(players_p))) {
        return false;
    }
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    if (!GameIo::load_players(players_p, seats, seat_n) || seat_n < m_player_n) {
        free_seats(seats, seat_n);
        return false;
    }
    u16 lucky_cap = 0;
    for (u16 p = 0; p < m_player_n; ++p) {
        if (seats[p].m_lucky != 0u) {
            lucky_cap = static_cast<u16>(lucky_cap + 1u);
        }
    }
    if (lucky_cap == 0) {
        free_seats(seats, seat_n);
        return false;
    }
    m_tl = new LuckyCitySaveTimeline[lucky_cap];
    m_seat = new u16[lucky_cap];
    m_lucky_n = 0;
    for (u16 p = 0; p < m_player_n; ++p) {
        if (seats[p].m_lucky == 0u) {
            continue;
        }
        if (!m_tl[m_lucky_n].setup(m_save_n)) {
            free_seats(seats, seat_n);
            clr();
            return false;
        }
        m_seat[m_lucky_n] = p;
        m_lucky_n = static_cast<u16>(m_lucky_n + 1u);
    }
    free_seats(seats, seat_n);
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char cities_p[512];
        if (!paths.cities_path(turn, cities_p, sizeof(cities_p))) {
            return false;
        }
        CityArray cities;
        if (!GameIo::load_cities(cities_p, cities)) {
            return false;
        }
        for (u16 li = 0; li < m_lucky_n; ++li) {
            const u16 seat = m_seat[li];
            u32 city_tot = 0;
            const u16 cn = cities.get_city_count();
            for (u16 ci = 0; ci < cn; ++ci) {
                const City* c = cities.get_city(ci);
                if (c == nullptr || c->get_owner() != seat) {
                    continue;
                }
                city_tot = city_tot + 1u;
            }
            if (!m_tl[li].set(si, city_tot)) {
                return false;
            }
        }
    }
    for (u16 li = 0; li < m_lucky_n; ++li) {
        m_tl[li].sync_count();
    }
    return true;
}

void LuckyCitySaveTimelineMng::sort () {
    if (m_tl == nullptr || m_lucky_n < 2) {
        return;
    }
    for (u16 i = 0; i < m_lucky_n; ++i) {
        u16 best = i;
        for (u16 j = static_cast<u16>(i + 1u); j < m_lucky_n; ++j) {
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

u16 LuckyCitySaveTimelineMng::lucky_n () const {
    return m_lucky_n;
}

u16 LuckyCitySaveTimelineMng::save_n () const {
    return m_save_n;
}

LuckyCitySaveTimeline& LuckyCitySaveTimelineMng::at (u16 i) {
    return m_tl[i];
}

const LuckyCitySaveTimeline& LuckyCitySaveTimelineMng::at (u16 i) const {
    return m_tl[i];
}

u16 LuckyCitySaveTimelineMng::seat (u16 i) const {
    if (m_seat == nullptr || i >= m_lucky_n) {
        return UINT16_MAX;
    }
    return m_seat[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
