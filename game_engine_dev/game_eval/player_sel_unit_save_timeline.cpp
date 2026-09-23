//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_sel_unit_save_timeline.h"

//================================================================================================================================
//=> - PlayerSelUnitSaveTimeline -
//================================================================================================================================

PlayerSelUnitSaveTimeline::PlayerSelUnitSaveTimeline ()
    : m_n(nullptr),
      m_save_n(0),
      m_count(0) {
}

PlayerSelUnitSaveTimeline::~PlayerSelUnitSaveTimeline () {
    clr();
}

void PlayerSelUnitSaveTimeline::clr () {
    delete[] m_n;
    m_n = nullptr;
    m_save_n = 0;
    m_count = 0;
}

bool PlayerSelUnitSaveTimeline::setup (u16 save_n) {
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

bool PlayerSelUnitSaveTimeline::set (u16 save_i, u32 n) {
    if (m_n == nullptr || save_i >= m_save_n) {
        return false;
    }
    m_n[save_i] = n;
    return true;
}

void PlayerSelUnitSaveTimeline::sync_count () {
    if (m_n == nullptr || m_save_n == 0) {
        m_count = 0;
        return;
    }
    m_count = m_n[m_save_n - 1u];
}

u16 PlayerSelUnitSaveTimeline::save_n () const {
    return m_save_n;
}

u32 PlayerSelUnitSaveTimeline::count () const {
    return m_count;
}

u32 PlayerSelUnitSaveTimeline::at (u16 save_i) const {
    if (m_n == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_n[save_i];
}

void PlayerSelUnitSaveTimeline::swap (PlayerSelUnitSaveTimeline& o) {
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
