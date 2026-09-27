//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_res_save_timeline_mng.h"

#include <cstdio>

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
//=> - PlayerResSaveTimelineMng -
//================================================================================================================================

PlayerResSaveTimelineMng::PlayerResSaveTimelineMng ()
    : m_tl(nullptr),
      m_seat(nullptr),
      m_player_n(0),
      m_save_n(0),
      m_res_idx(U16_KEY_NULL) {
}

PlayerResSaveTimelineMng::~PlayerResSaveTimelineMng () {
    clr();
}

void PlayerResSaveTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    delete[] m_seat;
    m_seat = nullptr;
    m_player_n = 0;
    m_save_n = 0;
    m_res_idx = U16_KEY_NULL;
}

bool PlayerResSaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_tl = new PlayerResSaveTimeline[player_n];
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

bool PlayerResSaveTimelineMng::fill (const EvalPaths& paths, u16 res_idx) {
    if (m_tl == nullptr || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    if (res_idx == U16_KEY_NULL) {
        return false;
    }
    m_res_idx = res_idx;
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char players_p[512];
        if (!paths.players_path(turn, players_p, sizeof(players_p))) {
            free_seats(seats, seat_n);
            return false;
        }
        if (!GameIo::load_players(players_p, seats, seat_n)) {
            free_seats(seats, seat_n);
            return false;
        }
        if (seat_n < m_player_n) {
            free_seats(seats, seat_n);
            return false;
        }
        for (u16 p = 0; p < m_player_n; ++p) {
            const u32 amt = static_cast<u32>(seats[p].m_res_ledger.get(m_res_idx));
            if (!m_tl[p].set(si, amt)) {
                free_seats(seats, seat_n);
                return false;
            }
        }
    }
    free_seats(seats, seat_n);
    for (u16 p = 0; p < m_player_n; ++p) {
        m_tl[p].sync_count();
    }
    return true;
}

void PlayerResSaveTimelineMng::sort () {
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

u16 PlayerResSaveTimelineMng::player_n () const {
    return m_player_n;
}

u16 PlayerResSaveTimelineMng::save_n () const {
    return m_save_n;
}

u16 PlayerResSaveTimelineMng::res_idx () const {
    return m_res_idx;
}

PlayerResSaveTimeline& PlayerResSaveTimelineMng::at (u16 i) {
    return m_tl[i];
}

const PlayerResSaveTimeline& PlayerResSaveTimelineMng::at (u16 i) const {
    return m_tl[i];
}

u16 PlayerResSaveTimelineMng::seat (u16 i) const {
    if (m_seat == nullptr || i >= m_player_n) {
        return UINT16_MAX;
    }
    return m_seat[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
