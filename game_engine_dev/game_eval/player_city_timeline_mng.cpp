//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_city_timeline_mng.h"

#include "eval_log.h"

//================================================================================================================================
//=> - PlayerCityTimelineMng -
//================================================================================================================================

PlayerCityTimelineMng::PlayerCityTimelineMng ()
    : m_tl(nullptr),
      m_seat(nullptr),
      m_player_n(0),
      m_turn_n(0),
      m_max_turn(0) {
}

PlayerCityTimelineMng::~PlayerCityTimelineMng () {
    clr();
}

void PlayerCityTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    delete[] m_seat;
    m_seat = nullptr;
    m_player_n = 0;
    m_turn_n = 0;
    m_max_turn = 0;
}

bool PlayerCityTimelineMng::setup (u16 player_n, u16 turn_n) {
    clr();
    if (player_n == 0 || turn_n == 0) {
        return false;
    }
    m_tl = new PlayerCityTimeline[player_n];
    m_seat = new u16[player_n];
    m_player_n = player_n;
    m_turn_n = turn_n;
    m_max_turn = (turn_n == 0) ? 0 : static_cast<u16>(turn_n - 1u);
    for (u16 i = 0; i < player_n; ++i) {
        if (!m_tl[i].setup(turn_n)) {
            clr();
            return false;
        }
        m_seat[i] = i;
    }
    return true;
}

bool PlayerCityTimelineMng::fill (const EvalLog& log) {
    if (m_tl == nullptr || !log.ok()) {
        return false;
    }
    const u32 seen_cap = static_cast<u32>(m_player_n) * 64u + 64u;
    u16* sx = new u16[seen_cap];
    u16* sy = new u16[seen_cap];
    u16* sp = new u16[seen_cap];
    u32 seen_n = 0;
    u16 cur = 0;
    m_max_turn = 0;
    for (u32 li = 0; li < log.line_n(); ++li) {
        u16 turn = 0;
        if (log.parse_new_turn(li, &turn)) {
            cur = turn;
            if (turn > m_max_turn) {
                m_max_turn = turn;
            }
            continue;
        }
        u16 x = 0;
        u16 y = 0;
        u16 player = 0;
        if (!log.parse_city_foundation(li, &x, &y, &player)) {
            continue;
        }
        if (player >= m_player_n || cur >= m_turn_n) {
            continue;
        }
        bool dup = false;
        for (u32 s = 0; s < seen_n; ++s) {
            if (sp[s] == player && sx[s] == x && sy[s] == y) {
                dup = true;
                break;
            }
        }
        if (dup) {
            continue;
        }
        if (seen_n < seen_cap) {
            sx[seen_n] = x;
            sy[seen_n] = y;
            sp[seen_n] = player;
            seen_n = seen_n + 1u;
        }
        if (!m_tl[player].note(cur)) {
            delete[] sx;
            delete[] sy;
            delete[] sp;
            return false;
        }
    }
    delete[] sx;
    delete[] sy;
    delete[] sp;
    return true;
}

void PlayerCityTimelineMng::sort () {
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

u16 PlayerCityTimelineMng::player_n () const {
    return m_player_n;
}

u16 PlayerCityTimelineMng::max_turn () const {
    return m_max_turn;
}

u16 PlayerCityTimelineMng::turn_n () const {
    return m_turn_n;
}

PlayerCityTimeline& PlayerCityTimelineMng::at (u16 i) {
    return m_tl[i];
}

const PlayerCityTimeline& PlayerCityTimelineMng::at (u16 i) const {
    return m_tl[i];
}

u16 PlayerCityTimelineMng::seat (u16 i) const {
    if (m_seat == nullptr || i >= m_player_n) {
        return UINT16_MAX;
    }
    return m_seat[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
