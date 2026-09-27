//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_res_save_timeline.h"

//================================================================================================================================
//=> - PlayerResSaveTimeline -
//================================================================================================================================

PlayerResSaveTimeline::PlayerResSaveTimeline ()
    : m_n(nullptr),
      m_save_n(0),
      m_count(0) {
}

PlayerResSaveTimeline::~PlayerResSaveTimeline () {
    clr();
}

void PlayerResSaveTimeline::clr () {
    delete[] m_n;
    m_n = nullptr;
    m_save_n = 0;
    m_count = 0;
}

bool PlayerResSaveTimeline::setup (u16 save_n) {
    clr();
    if (save_n == 0) {
        return false;
    }
    m_n = new u32[save_n];
    m_save_n = save_n;
    for (u16 i = 0; i < save_n; ++i) {
        m_n[i] = 0;
    }
    m_count = 0;
    return true;
}

bool PlayerResSaveTimeline::set (u16 save_i, u32 n) {
    if (m_n == nullptr || save_i >= m_save_n) {
        return false;
    }
    m_n[save_i] = n;
    return true;
}

void PlayerResSaveTimeline::sync_count () {
    if (m_n == nullptr || m_save_n == 0) {
        m_count = 0;
        return;
    }
    u64 sum = 0;
    for (u16 i = 0; i < m_save_n; ++i) {
        sum = sum + static_cast<u64>(m_n[i]);
    }
    m_count = sum;
}

u16 PlayerResSaveTimeline::save_n () const {
    return m_save_n;
}

u64 PlayerResSaveTimeline::count () const {
    return m_count;
}

u32 PlayerResSaveTimeline::at (u16 save_i) const {
    if (m_n == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_n[save_i];
}

void PlayerResSaveTimeline::swap (PlayerResSaveTimeline& o) {
    u32* n = m_n;
    u16 save_n = m_save_n;
    u64 count = m_count;
    m_n = o.m_n;
    m_save_n = o.m_save_n;
    m_count = o.m_count;
    o.m_n = n;
    o.m_save_n = save_n;
    o.m_count = count;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
