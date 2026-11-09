//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_own_lucky_end_mng.h"

#include <cstdio>

#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_state.h"
#include "map_own_lucky_end_snap.h"
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
        delete seats[i].m_civics_adopted;
        seats[i].m_civics_adopted = nullptr;
        delete seats[i].m_tech_age;
        seats[i].m_tech_age = nullptr;
        seats[i].m_res_ledger.clear();
    }
    delete[] seats;
    seats = nullptr;
    n = 0;
}

//================================================================================================================================
//=> - MapOwnLuckyEndMng -
//================================================================================================================================

MapOwnLuckyEndMng::MapOwnLuckyEndMng ()
    : m_lucky(nullptr),
      m_lucky_n(0),
      m_player_n(0),
      m_save_n(0),
      m_wrote_n(0) {
}

MapOwnLuckyEndMng::~MapOwnLuckyEndMng () {
    clr();
}

void MapOwnLuckyEndMng::clr () {
    delete[] m_lucky;
    m_lucky = nullptr;
    m_lucky_n = 0;
    m_player_n = 0;
    m_save_n = 0;
    m_wrote_n = 0;
}

bool MapOwnLuckyEndMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_player_n = player_n;
    m_save_n = save_n;
    return true;
}

bool MapOwnLuckyEndMng::fill (const EvalPaths& paths, cstr out_dir) {
    if (out_dir == nullptr || out_dir[0] == 0 || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    m_wrote_n = 0;
    const u32 turn0 = paths.save_turn_at(0);
    char players_p[512];
    if (!paths.players_path(turn0, players_p, sizeof(players_p))) {
        return false;
    }
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    if (!GameIo::load_players(players_p, seats, seat_n) || seat_n < m_player_n) {
        std::printf("map_own_lucky_end: load_players failed %s seat_n=%u need=%u\n",
            players_p, static_cast<u32>(seat_n), static_cast<u32>(m_player_n));
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
        std::printf("map_own_lucky_end: no lucky seats in %s\n", players_p);
        free_seats(seats, seat_n);
        return false;
    }
    m_lucky = new u16[lucky_cap];
    if (m_lucky == nullptr) {
        free_seats(seats, seat_n);
        return false;
    }
    m_lucky_n = 0;
    for (u16 p = 0; p < m_player_n; ++p) {
        if (seats[p].m_lucky != 0u) {
            m_lucky[m_lucky_n] = p;
            m_lucky_n = static_cast<u16>(m_lucky_n + 1u);
        }
    }
    free_seats(seats, seat_n);

    const u32 turn = paths.save_turn_at(static_cast<u16>(m_save_n - 1u));
    char map_p[512];
    if (!paths.map_path(turn, map_p, sizeof(map_p))) {
        return false;
    }
    GameArraySimple map;
    if (!GameIo::load_map_tiles(map_p, map)) {
        return false;
    }
    for (u16 i = 0; i < m_lucky_n; ++i) {
        const u16 seat = m_lucky[i];
        char out_p[512];
        if (std::snprintf(out_p, sizeof(out_p), "%s/lucky_own_seat_%03u.ppm",
                out_dir, static_cast<u32>(seat)) <= 0) {
            return false;
        }
        if (!MapOwnLuckyEndSnap::write(out_p, map, seat)) {
            return false;
        }
        m_wrote_n = static_cast<u16>(m_wrote_n + 1u);
        std::printf("wrote %s\n", out_p);
    }
    return true;
}

u16 MapOwnLuckyEndMng::wrote_n () const {
    return m_wrote_n;
}

u16 MapOwnLuckyEndMng::lucky_n () const {
    return m_lucky_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
