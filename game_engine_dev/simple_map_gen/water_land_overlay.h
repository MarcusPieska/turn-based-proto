//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WATER_LAND_OVERLAY_H
#define WATER_LAND_OVERLAY_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - WaterLandOverlay -
//================================================================================================================================

class WaterLandOverlay {
public:
    explicit WaterLandOverlay (const MapArrayTerrain& terrain);

    bool is_valid () const;
    u16 width () const;
    u16 height () const;
    const u8* water_land_gray () const;
    ~WaterLandOverlay ();

private:
    WaterLandOverlay (const WaterLandOverlay& other) = delete;
    WaterLandOverlay (WaterLandOverlay&& other) = delete;

    bool m_ok;
    MapArrayOverlay* m_ov;
};

#endif // WATER_LAND_OVERLAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
