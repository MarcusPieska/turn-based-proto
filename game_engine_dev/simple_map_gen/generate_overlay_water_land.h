//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_OVERLAY_WATER_LAND_H
#define GENERATE_OVERLAY_WATER_LAND_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - OverlayWaterLand -
//================================================================================================================================

class Generate_OverlayWaterLand {
public:
    explicit Generate_OverlayWaterLand ();
    ~Generate_OverlayWaterLand ();

    bool generate (const MapArrayTerrain& terrain);
    bool is_valid () const;
    void save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    const u8* water_land_gray () const;
    bool save_water_land_gray (cstr path) const;
    const MapArrayOverlay* overlay_ptr () const;

    MapArrayOverlay* take_overlay ();

private:
    Generate_OverlayWaterLand (const Generate_OverlayWaterLand& other) = delete;
    Generate_OverlayWaterLand (Generate_OverlayWaterLand&& other) = delete;

    bool m_valid_generation;
    MapArrayOverlay* m_overlay;
};

#endif // GENERATE_OVERLAY_WATER_LAND_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
