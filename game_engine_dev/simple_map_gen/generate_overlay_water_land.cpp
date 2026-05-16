//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_overlay_water_land.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static bool is_water_class (u8 c) {
    return c == TERR_OCEAN[0] || c == TERR_SEA[0] || c == TERR_COASTAL[0];
}

//================================================================================================================================
//=> - OverlayWaterLand -
//================================================================================================================================

Generate_OverlayWaterLand::Generate_OverlayWaterLand () :
    m_valid_generation(false),
    m_overlay(nullptr) {
}

Generate_OverlayWaterLand::~Generate_OverlayWaterLand () {
    delete m_overlay;
    m_overlay = nullptr;
}

bool Generate_OverlayWaterLand::generate (const MapArrayTerrain& terrain) {
    m_valid_generation = false;
    delete m_overlay;
    m_overlay = nullptr;
    const u16 w = terrain.width();
    const u16 h = terrain.height();
    const u8* t = terrain.data();
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    if (npx == 0 || t == nullptr) {
        return false;
    }
    m_overlay = new MapArrayOverlay();
    if (!m_overlay->resize(w, h)) {
        delete m_overlay;
        m_overlay = nullptr;
        return false;
    }
    u8* g = m_overlay->data_w();
    for (u32 i = 0; i < npx; ++i) {
        g[i] = is_water_class(t[i]) ? WL_OVERLAY_WATER_GRAY : WL_OVERLAY_LAND_GRAY;
    }
    m_valid_generation = true;
    return true;
}

bool Generate_OverlayWaterLand::is_valid () const {
    return m_valid_generation;
}

void Generate_OverlayWaterLand::save_output (cstr path) const {
    if (path == nullptr) {
        return;
    }
    save_water_land_gray(path);
}

u16 Generate_OverlayWaterLand::width () const {
    return m_overlay != nullptr ? m_overlay->width() : 0;
}

u16 Generate_OverlayWaterLand::height () const {
    return m_overlay != nullptr ? m_overlay->height() : 0;
}

const u8* Generate_OverlayWaterLand::water_land_gray () const {
    return m_overlay != nullptr ? m_overlay->data() : nullptr;
}

bool Generate_OverlayWaterLand::save_water_land_gray (cstr path) const {
    if (!m_valid_generation || path == nullptr || m_overlay == nullptr) {
        return false;
    }
    return m_overlay->save(path);
}

const MapArrayOverlay* Generate_OverlayWaterLand::overlay_ptr () const {
    return m_overlay;
}

MapArrayOverlay* Generate_OverlayWaterLand::take_overlay () {
    MapArrayOverlay* p = m_overlay;
    m_overlay = nullptr;
    m_valid_generation = false;
    return p;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
