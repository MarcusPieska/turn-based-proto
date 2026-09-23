//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_farm_save_timeline.h"

//================================================================================================================================
//=> - PlayerFarmSaveTimeline -
//================================================================================================================================

PlayerFarmSaveTimeline::PlayerFarmSaveTimeline ()
    : m_n(nullptr),
      m_save_n(0),
      m_count(0) {
}

PlayerFarmSaveTimeline::~PlayerFarmSaveTimeline () {
    clr();
}

void PlayerFarmSaveTimeline::clr () {
    delete[] m_n;
    m_n = nullptr;
    m_save_n = 0;
    m_count = 0;
}

bool PlayerFarmSaveTimeline::setup (u16 save_n) {
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

bool PlayerFarmSaveTimeline::set (u16 save_i, u32 n) {
    if (m_n == nullptr || save_i >= m_save_n) {
        return false;
    }
    m_n[save_i] = n;
    return true;
}

void PlayerFarmSaveTimeline::sync_count () {
    if (m_n == nullptr || m_save_n == 0) {
        m_count = 0;
        return;
    }
    m_count = m_n[m_save_n - 1u];
}

u16 PlayerFarmSaveTimeline::save_n () const {
    return m_save_n;
}

u32 PlayerFarmSaveTimeline::count () const {
    return m_count;
}

u32 PlayerFarmSaveTimeline::at (u16 save_i) const {
    if (m_n == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_n[save_i];
}

void PlayerFarmSaveTimeline::swap (PlayerFarmSaveTimeline& o) {
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
