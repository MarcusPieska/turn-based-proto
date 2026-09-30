//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_land_size_save_timeline.h"

//================================================================================================================================
//=> - PlayerLandSizeSaveTimeline -
//================================================================================================================================

PlayerLandSizeSaveTimeline::PlayerLandSizeSaveTimeline ()
    : m_land(nullptr),
      m_avg_milli(nullptr),
      m_max(nullptr),
      m_save_n(0),
      m_count(0) {
}

PlayerLandSizeSaveTimeline::~PlayerLandSizeSaveTimeline () {
    clr();
}

void PlayerLandSizeSaveTimeline::clr () {
    delete[] m_land;
    m_land = nullptr;
    delete[] m_avg_milli;
    m_avg_milli = nullptr;
    delete[] m_max;
    m_max = nullptr;
    m_save_n = 0;
    m_count = 0;
}

bool PlayerLandSizeSaveTimeline::setup (u16 save_n) {
    clr();
    if (save_n == 0) {
        return false;
    }
    m_land = new u32[save_n];
    m_avg_milli = new u32[save_n];
    m_max = new u16[save_n];
    m_save_n = save_n;
    for (u16 i = 0; i < save_n; ++i) {
        m_land[i] = 0;
        m_avg_milli[i] = 0;
        m_max[i] = 0;
    }
    m_count = 0;
    return true;
}

bool PlayerLandSizeSaveTimeline::set (u16 save_i, u32 land_n, u32 avg_milli, u16 max_sz) {
    if (m_land == nullptr || save_i >= m_save_n) {
        return false;
    }
    m_land[save_i] = land_n;
    m_avg_milli[save_i] = avg_milli;
    m_max[save_i] = max_sz;
    return true;
}

void PlayerLandSizeSaveTimeline::sync_count () {
    if (m_land == nullptr || m_save_n == 0) {
        m_count = 0;
        return;
    }
    m_count = m_land[m_save_n - 1u];
}

u16 PlayerLandSizeSaveTimeline::save_n () const {
    return m_save_n;
}

u32 PlayerLandSizeSaveTimeline::count () const {
    return m_count;
}

u32 PlayerLandSizeSaveTimeline::land_at (u16 save_i) const {
    if (m_land == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_land[save_i];
}

u32 PlayerLandSizeSaveTimeline::avg_milli_at (u16 save_i) const {
    if (m_avg_milli == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_avg_milli[save_i];
}

u16 PlayerLandSizeSaveTimeline::max_at (u16 save_i) const {
    if (m_max == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_max[save_i];
}

void PlayerLandSizeSaveTimeline::swap (PlayerLandSizeSaveTimeline& o) {
    u32* land = m_land;
    u32* avg = m_avg_milli;
    u16* mx = m_max;
    u16 save_n = m_save_n;
    u32 count = m_count;
    m_land = o.m_land;
    m_avg_milli = o.m_avg_milli;
    m_max = o.m_max;
    m_save_n = o.m_save_n;
    m_count = o.m_count;
    o.m_land = land;
    o.m_avg_milli = avg;
    o.m_max = mx;
    o.m_save_n = save_n;
    o.m_count = count;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
