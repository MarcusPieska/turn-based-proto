//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_overlay.h"

//================================================================================================================================
//=> - Overlay -
//================================================================================================================================

Generate_Overlay::Generate_Overlay () :
    m_valid_generation(false),
    m_overlay(nullptr) {
}

Generate_Overlay::~Generate_Overlay () {
    delete m_overlay;
    m_overlay = nullptr;
}

bool Generate_Overlay::begin_overlay (const MapArrayTerrain& ter) {
    m_valid_generation = false;
    const u16 w = ter.width();
    const u16 h = ter.height();
    const u8* td = ter.data();
    if (w == 0 || h == 0 || td == nullptr) {
        return false;
    }
    if (m_overlay == nullptr) {
        m_overlay = new MapArrayOverlay();
    }
    if (!m_overlay->resize(w, h)) {
        return false;
    }
    return true;
}

bool Generate_Overlay::generate_spec_terrain_overlay (const MapArrayTerrain& ter, u8 cls, bool invert) {
    if (!begin_overlay(ter)) {
        return false;
    }
    u8 on_col = OVERLAY_SELECTED1;
    u8 off_col = OVERLAY_OMITTED;
    if (invert) {
        on_col = OVERLAY_OMITTED;
        off_col = OVERLAY_SELECTED1;
    }
    const u16 w = ter.width();
    const u16 h = ter.height();
    const u8* td = ter.data();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* g = m_overlay->data_w();
    for (u32 i = 0; i < n; ++i) {
        g[i] = td[i] == cls ? on_col : off_col;
    }
    m_valid_generation = true;
    return true;
}

bool Generate_Overlay::generate_terrain_limit_overlay (const MapArrayTerrain& ter, u8 cls, bool invert) {
    if (!begin_overlay(ter)) {
        return false;
    }
    u8 on_col = OVERLAY_SELECTED1;
    u8 off_col = OVERLAY_OMITTED;
    if (invert) {
        on_col = OVERLAY_OMITTED;
        off_col = OVERLAY_SELECTED1;
    }
    const u16 w = ter.width();
    const u16 h = ter.height();
    const u8* td = ter.data();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* g = m_overlay->data_w();
    for (u32 i = 0; i < n; ++i) {
        g[i] = td[i] >= cls ? on_col : off_col;
    }
    m_valid_generation = true;
    return true;
}

bool Generate_Overlay::generate_spec_terrains_overlay (const MapArrayTerrain& ter, u8 cls0, u8 cls1) {
    if (!begin_overlay(ter)) {
        return false;
    }
    const u16 w = ter.width();
    const u16 h = ter.height();
    const u8* td = ter.data();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* g = m_overlay->data_w();
    for (u32 i = 0; i < n; ++i) {
        const u8 c = td[i];
        if (c == cls0) {
            g[i] = OVERLAY_SELECTED1;
        } else if (c == cls1) {
            g[i] = OVERLAY_SELECTED2;
        } else {
            g[i] = OVERLAY_OMITTED;
        }
    }
    m_valid_generation = true;
    return true;
}

bool Generate_Overlay::is_valid () const {
    return m_valid_generation;
}

bool Generate_Overlay::save_output (cstr path) const {
    if (path == nullptr) {
        return false;
    }
    if (!m_valid_generation || m_overlay == nullptr) {
        return false;
    }
    return m_overlay->save(path);
}

u16 Generate_Overlay::width () const {
    return m_overlay != nullptr ? m_overlay->width() : 0;
}

u16 Generate_Overlay::height () const {
    return m_overlay != nullptr ? m_overlay->height() : 0;
}

const u8* Generate_Overlay::overlay_gray () const {
    return m_overlay != nullptr ? m_overlay->data() : nullptr;
}

const MapArrayOverlay* Generate_Overlay::overlay_ptr () const {
    return m_overlay;
}

MapArrayOverlay* Generate_Overlay::take_overlay () {
    MapArrayOverlay* p = m_overlay;
    m_overlay = nullptr;
    m_valid_generation = false;
    return p;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
