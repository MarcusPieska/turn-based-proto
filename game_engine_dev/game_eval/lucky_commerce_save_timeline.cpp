//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_commerce_save_timeline.h"

//================================================================================================================================
//=> - LuckyCommerceSaveTimeline -
//================================================================================================================================

LuckyCommerceSaveTimeline::LuckyCommerceSaveTimeline ()
    : m_n(nullptr),
      m_save_n(0),
      m_count(0) {
}

LuckyCommerceSaveTimeline::~LuckyCommerceSaveTimeline () {
    clr();
}

void LuckyCommerceSaveTimeline::clr () {
    delete[] m_n;
    m_n = nullptr;
    m_save_n = 0;
    m_count = 0;
}

bool LuckyCommerceSaveTimeline::setup (u16 save_n) {
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

bool LuckyCommerceSaveTimeline::set (u16 save_i, u32 n) {
    if (m_n == nullptr || save_i >= m_save_n) {
        return false;
    }
    m_n[save_i] = n;
    return true;
}

void LuckyCommerceSaveTimeline::sync_count () {
    if (m_n == nullptr || m_save_n == 0) {
        m_count = 0;
        return;
    }
    m_count = m_n[m_save_n - 1u];
}

u16 LuckyCommerceSaveTimeline::save_n () const {
    return m_save_n;
}

u32 LuckyCommerceSaveTimeline::count () const {
    return m_count;
}

u32 LuckyCommerceSaveTimeline::at (u16 save_i) const {
    if (m_n == nullptr || save_i >= m_save_n) {
        return 0;
    }
    return m_n[save_i];
}

void LuckyCommerceSaveTimeline::swap (LuckyCommerceSaveTimeline& o) {
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
