//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_road_save_timeline.h"

//================================================================================================================================
//=> - MapRoadSaveTimeline -
//================================================================================================================================

MapRoadSaveTimeline::MapRoadSaveTimeline ()
    : m_n(nullptr),
      m_save_n(0),
      m_count(0) {
}

MapRoadSaveTimeline::~MapRoadSaveTimeline () {
    clr();
}

void MapRoadSaveTimeline::clr () {
    delete[] m_n;
    m_n = nullptr;
    m_save_n = 0;
    m_count = 0;
}

bool MapRoadSaveTimeline::setup (u16 save_n) {
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

bool MapRoadSaveTimeline::set (u16 save_i, u32 n) {
    if (m_n == nullptr || save_i >= m_save_n) {
        return false;
    }
    m_n[save_i] = n;
    return true;
}

void MapRoadSaveTimeline::sync_count () {
    if (m_n == nullptr || m_save_n == 0) {
        m_count = 0;
        return;
    }
    m_count = m_n[m_save_n - 1u];
}

u16 MapRoadSaveTimeline::save_n () const {
    return m_save_n;
}

u32 MapRoadSaveTimeline::count () const {
    return m_count;
}

u32 MapRoadSaveTimeline::at (u16 save_i) const {
    if (m_n == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_n[save_i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
