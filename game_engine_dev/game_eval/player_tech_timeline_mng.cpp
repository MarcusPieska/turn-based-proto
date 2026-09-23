//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_tech_timeline_mng.h"

#include "eval_log.h"

//================================================================================================================================
//=> - PlayerTechTimelineMng -
//================================================================================================================================

PlayerTechTimelineMng::PlayerTechTimelineMng ()
    : m_tl(nullptr),
      m_seat(nullptr),
      m_player_n(0),
      m_tech_n(0),
      m_max_turn(0) {
}

PlayerTechTimelineMng::~PlayerTechTimelineMng () {
    clr();
}

void PlayerTechTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    delete[] m_seat;
    m_seat = nullptr;
    m_player_n = 0;
    m_tech_n = 0;
    m_max_turn = 0;
}

bool PlayerTechTimelineMng::setup (u16 player_n, u16 tech_n) {
    clr();
    if (player_n == 0 || tech_n == 0) {
        return false;
    }
    m_tl = new PlayerTechTimeline[player_n];
    m_seat = new u16[player_n];
    m_player_n = player_n;
    m_tech_n = tech_n;
    m_max_turn = 0;
    for (u16 i = 0; i < player_n; ++i) {
        if (!m_tl[i].setup(tech_n)) {
            clr();
            return false;
        }
        m_seat[i] = i;
    }
    return true;
}

bool PlayerTechTimelineMng::fill (const EvalLog& log) {
    if (m_tl == nullptr || !log.ok()) {
        return false;
    }
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
        u16 player = 0;
        u16 tech = 0;
        if (!log.parse_tech_discover(li, &player, &tech)) {
            continue;
        }
        if (player >= m_player_n || tech >= m_tech_n) {
            continue;
        }
        if (!m_tl[player].note(tech, cur)) {
            return false;
        }
    }
    return true;
}

void PlayerTechTimelineMng::sort () {
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

u16 PlayerTechTimelineMng::player_n () const {
    return m_player_n;
}

u16 PlayerTechTimelineMng::tech_n () const {
    return m_tech_n;
}

u16 PlayerTechTimelineMng::max_turn () const {
    return m_max_turn;
}

u16 PlayerTechTimelineMng::turn_n () const {
    if (m_max_turn == UINT16_MAX) {
        return UINT16_MAX;
    }
    return static_cast<u16>(m_max_turn + 1u);
}

PlayerTechTimeline& PlayerTechTimelineMng::at (u16 i) {
    return m_tl[i];
}

const PlayerTechTimeline& PlayerTechTimelineMng::at (u16 i) const {
    return m_tl[i];
}

u16 PlayerTechTimelineMng::seat (u16 i) const {
    if (m_seat == nullptr || i >= m_player_n) {
        return UINT16_MAX;
    }
    return m_seat[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
