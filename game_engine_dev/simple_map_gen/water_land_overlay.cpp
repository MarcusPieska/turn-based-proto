//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "water_land_overlay.h"
#include "generate_overlay_water_land.h"

//================================================================================================================================
//=> - WaterLandOverlay -
//================================================================================================================================

WaterLandOverlay::WaterLandOverlay (const MapArrayTerrain& terrain) :
    m_ok(false),
    m_ov(nullptr) {
    Generate_OverlayWaterLand gen;
    if (!gen.generate(terrain) || !gen.is_valid()) {
        return;
    }
    m_ov = gen.take_overlay();
    m_ok = m_ov != nullptr;
}

WaterLandOverlay::~WaterLandOverlay () {
    delete m_ov;
    m_ov = nullptr;
}

bool WaterLandOverlay::is_valid () const {
    return m_ok;
}

u16 WaterLandOverlay::width () const {
    return m_ov != nullptr ? m_ov->width() : 0;
}

u16 WaterLandOverlay::height () const {
    return m_ov != nullptr ? m_ov->height() : 0;
}

const u8* WaterLandOverlay::water_land_gray () const {
    return m_ov != nullptr ? m_ov->data() : nullptr;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
