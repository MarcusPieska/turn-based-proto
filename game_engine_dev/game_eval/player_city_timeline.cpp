//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_city_timeline.h"

//================================================================================================================================
//=> - PlayerCityTimeline -
//================================================================================================================================

PlayerCityTimeline::PlayerCityTimeline ()
    : m_inc(nullptr),
      m_turn_n(0),
      m_count(0) {
}

PlayerCityTimeline::~PlayerCityTimeline () {
    clr();
}

void PlayerCityTimeline::clr () {
    delete[] m_inc;
    m_inc = nullptr;
    m_turn_n = 0;
    m_count = 0;
}

bool PlayerCityTimeline::setup (u16 turn_n) {
    clr();
    if (turn_n == 0) {
        return false;
    }
    m_inc = new u16[turn_n];
    m_turn_n = turn_n;
    for (u16 t = 0; t < turn_n; ++t) {
        m_inc[t] = 0;
    }
    m_count = 0;
    return true;
}

bool PlayerCityTimeline::note (u16 turn) {
    if (m_inc == nullptr || turn >= m_turn_n) {
        return false;
    }
    m_inc[turn] = static_cast<u16>(m_inc[turn] + 1u);
    m_count = static_cast<u16>(m_count + 1u);
    return true;
}

void PlayerCityTimeline::recount () {
    m_count = 0;
    if (m_inc == nullptr) {
        return;
    }
    for (u16 t = 0; t < m_turn_n; ++t) {
        m_count = static_cast<u16>(m_count + m_inc[t]);
    }
}

u16 PlayerCityTimeline::turn_n () const {
    return m_turn_n;
}

u16 PlayerCityTimeline::count () const {
    return m_count;
}

u16 PlayerCityTimeline::rate_at (u16 turn) const {
    if (m_inc == nullptr || turn >= m_turn_n) {
        return 0;
    }
    return m_inc[turn];
}

bool PlayerCityTimeline::cum (u16* y, u16 turn_n) const {
    if (!rate(y, turn_n)) {
        return false;
    }
    for (u16 t = 1; t < turn_n; ++t) {
        y[t] = static_cast<u16>(y[t] + y[t - 1u]);
    }
    return true;
}

bool PlayerCityTimeline::rate (u16* y, u16 turn_n) const {
    if (y == nullptr || turn_n == 0 || m_inc == nullptr) {
        return false;
    }
    const u16 n = (turn_n < m_turn_n) ? turn_n : m_turn_n;
    for (u16 t = 0; t < n; ++t) {
        y[t] = m_inc[t];
    }
    for (u16 t = n; t < turn_n; ++t) {
        y[t] = 0;
    }
    return true;
}

void PlayerCityTimeline::swap (PlayerCityTimeline& o) {
    u16* inc = m_inc;
    u16 turn_n = m_turn_n;
    u16 count = m_count;
    m_inc = o.m_inc;
    m_turn_n = o.m_turn_n;
    m_count = o.m_count;
    o.m_inc = inc;
    o.m_turn_n = turn_n;
    o.m_count = count;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
