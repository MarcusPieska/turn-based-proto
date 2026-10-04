//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_city_save_timeline.h"

//================================================================================================================================
//=> - LuckyCitySaveTimeline -
//================================================================================================================================

LuckyCitySaveTimeline::LuckyCitySaveTimeline ()
    : m_n(nullptr),
      m_save_n(0),
      m_count(0) {
}

LuckyCitySaveTimeline::~LuckyCitySaveTimeline () {
    clr();
}

void LuckyCitySaveTimeline::clr () {
    delete[] m_n;
    m_n = nullptr;
    m_save_n = 0;
    m_count = 0;
}

bool LuckyCitySaveTimeline::setup (u16 save_n) {
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

bool LuckyCitySaveTimeline::set (u16 save_i, u32 n) {
    if (m_n == nullptr || save_i >= m_save_n) {
        return false;
    }
    m_n[save_i] = n;
    return true;
}

void LuckyCitySaveTimeline::sync_count () {
    if (m_n == nullptr || m_save_n == 0) {
        m_count = 0;
        return;
    }
    m_count = m_n[m_save_n - 1u];
}

u16 LuckyCitySaveTimeline::save_n () const {
    return m_save_n;
}

u32 LuckyCitySaveTimeline::count () const {
    return m_count;
}

u32 LuckyCitySaveTimeline::at (u16 save_i) const {
    if (m_n == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_n[save_i];
}

void LuckyCitySaveTimeline::swap (LuckyCitySaveTimeline& o) {
    u32* n = m_n;
    u16 save_n = m_save_n;
    u32 count = m_count;
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
