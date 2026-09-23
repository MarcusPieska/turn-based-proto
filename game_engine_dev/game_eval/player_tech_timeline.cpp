//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_tech_timeline.h"

//================================================================================================================================
//=> - PlayerTechTimeline -
//================================================================================================================================

PlayerTechTimeline::PlayerTechTimeline ()
    : m_disc(nullptr),
      m_tech_n(0),
      m_count(0) {
}

PlayerTechTimeline::~PlayerTechTimeline () {
    clr();
}

void PlayerTechTimeline::clr () {
    delete[] m_disc;
    m_disc = nullptr;
    m_tech_n = 0;
    m_count = 0;
}

bool PlayerTechTimeline::setup (u16 tech_n) {
    clr();
    if (tech_n == 0) {
        return false;
    }
    m_disc = new u16[tech_n];
    m_tech_n = tech_n;
    for (u16 i = 0; i < tech_n; ++i) {
        m_disc[i] = UINT16_MAX;
    }
    m_count = 0;
    return true;
}

bool PlayerTechTimeline::note (u16 tech, u16 turn) {
    if (m_disc == nullptr || tech >= m_tech_n || turn == UINT16_MAX) {
        return false;
    }
    if (m_disc[tech] != UINT16_MAX) {
        return true;
    }
    m_disc[tech] = turn;
    m_count = static_cast<u16>(m_count + 1u);
    return true;
}

void PlayerTechTimeline::recount () {
    m_count = 0;
    if (m_disc == nullptr) {
        return;
    }
    for (u16 i = 0; i < m_tech_n; ++i) {
        if (m_disc[i] != UINT16_MAX) {
            m_count = static_cast<u16>(m_count + 1u);
        }
    }
}

u16 PlayerTechTimeline::tech_n () const {
    return m_tech_n;
}

u16 PlayerTechTimeline::count () const {
    return m_count;
}

u16 PlayerTechTimeline::disc (u16 tech) const {
    if (m_disc == nullptr || tech >= m_tech_n) {
        return UINT16_MAX;
    }
    return m_disc[tech];
}

bool PlayerTechTimeline::cum (u16* y, u16 turn_n) const {
    if (!rate(y, turn_n)) {
        return false;
    }
    for (u16 t = 1; t < turn_n; ++t) {
        y[t] = static_cast<u16>(y[t] + y[t - 1u]);
    }
    return true;
}

bool PlayerTechTimeline::rate (u16* y, u16 turn_n) const {
    if (y == nullptr || turn_n == 0 || m_disc == nullptr) {
        return false;
    }
    for (u16 t = 0; t < turn_n; ++t) {
        y[t] = 0;
    }
    for (u16 i = 0; i < m_tech_n; ++i) {
        const u16 d = m_disc[i];
        if (d == UINT16_MAX || d >= turn_n) {
            continue;
        }
        y[d] = static_cast<u16>(y[d] + 1u);
    }
    return true;
}

void PlayerTechTimeline::swap (PlayerTechTimeline& o) {
    u16* disc = m_disc;
    u16 tech_n = m_tech_n;
    u16 count = m_count;
    m_disc = o.m_disc;
    m_tech_n = o.m_tech_n;
    m_count = o.m_count;
    o.m_disc = disc;
    o.m_tech_n = tech_n;
    o.m_count = count;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
