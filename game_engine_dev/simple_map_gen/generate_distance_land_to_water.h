//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_LAND_TO_WATER_H
#define GENERATE_DISTANCE_LAND_TO_WATER_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - DistanceLandToWater -
//================================================================================================================================

class Generate_DistanceLandToWater {
public:
    explicit Generate_DistanceLandToWater (u32 seed);

    bool generate ();
    bool generate (const u8* wl_gray, u16 w, u16 h);
    bool is_valid () const;
    bool save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    const MapArrayDistance& distance () const;
    const MapArrayOverlay& water_land () const;

private:
    Generate_DistanceLandToWater (const Generate_DistanceLandToWater& other) = delete;
    Generate_DistanceLandToWater (Generate_DistanceLandToWater&& other) = delete;

    u32 m_seed;
    bool m_valid_generation;
    MapArrayOverlay m_wl;
    MapArrayDistance m_dist;
};

#endif // GENERATE_DISTANCE_LAND_TO_WATER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
