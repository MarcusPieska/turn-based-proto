//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_overlay_wl_clip.h"

#include <cstring>

//================================================================================================================================
//=> - OverlayWlClip -
//================================================================================================================================

Generate_OverlayWlClip::Generate_OverlayWlClip () :
    m_valid_generation(false),
    m_overlay(nullptr) {
}

Generate_OverlayWlClip::~Generate_OverlayWlClip () {
    delete m_overlay;
    m_overlay = nullptr;
}

bool Generate_OverlayWlClip::generate (
    const MapArrayOverlay& first,
    const MapArrayOverlay& second,
    i32 offset_x,
    i32 offset_y) {
    m_valid_generation = false;
    delete m_overlay;
    m_overlay = nullptr;
    const u16 m_w = first.width();
    const u16 m_h = first.height();
    const u8* fg_src = first.data();
    const u16 bw = second.width();
    const u16 bh = second.height();
    const u8* bg = second.data();
    const u32 npx = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    if (npx == 0 || fg_src == nullptr || bg == nullptr || bw == 0 || bh == 0) {
        return false;
    }
    m_overlay = new MapArrayOverlay();
    if (!m_overlay->resize(m_w, m_h)) {
        delete m_overlay;
        m_overlay = nullptr;
        return false;
    }
    u8* fg = m_overlay->data_w();
    std::memcpy(fg, fg_src, static_cast<size_t>(npx));
    for (u16 sy = 0; sy < m_h; ++sy) {
        for (u16 sx = 0; sx < m_w; ++sx) {
            const i32 bx = offset_x + static_cast<i32>(sx);
            const i32 by = offset_y + static_cast<i32>(sy);
            if (bx < 0 || by < 0 || bx >= static_cast<i32>(bw) || by >= static_cast<i32>(bh)) {
                continue;
            }
            const u32 ai = static_cast<u32>(sy) * static_cast<u32>(m_w) + static_cast<u32>(sx);
            const u32 bi = static_cast<u32>(by) * static_cast<u32>(bw) + static_cast<u32>(bx);
            if (fg[ai] == WL_OVERLAY_LAND_GRAY && bg[bi] == WL_OVERLAY_LAND_GRAY) {
                fg[ai] = WL_OVERLAY_WATER_GRAY;
            }
        }
    }
    m_valid_generation = true;
    return true;
}

bool Generate_OverlayWlClip::is_valid () const {
    return m_valid_generation;
}

void Generate_OverlayWlClip::save_output (cstr path) const {
    if (path == nullptr) {
        return;
    }
    save_water_land_gray(path);
}

u16 Generate_OverlayWlClip::width () const {
    return m_overlay != nullptr ? m_overlay->width() : 0;
}

u16 Generate_OverlayWlClip::height () const {
    return m_overlay != nullptr ? m_overlay->height() : 0;
}

const u8* Generate_OverlayWlClip::water_land_gray () const {
    return m_overlay != nullptr ? m_overlay->data() : nullptr;
}

bool Generate_OverlayWlClip::save_water_land_gray (cstr path) const {
    if (!m_valid_generation || path == nullptr || m_overlay == nullptr) {
        return false;
    }
    return m_overlay->save(path);
}

MapArrayOverlay* Generate_OverlayWlClip::take_overlay () {
    MapArrayOverlay* p = m_overlay;
    m_overlay = nullptr;
    m_valid_generation = false;
    return p;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
