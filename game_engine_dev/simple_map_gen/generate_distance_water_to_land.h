//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_WATER_TO_LAND_H
#define GENERATE_DISTANCE_WATER_TO_LAND_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - DistanceWaterToLand -
//================================================================================================================================

class Generate_DistanceWaterToLand {
public:
    explicit Generate_DistanceWaterToLand (u32 seed);

    bool generate ();
    bool generate (const u8* wl_gray, u16 w, u16 h);
    bool is_valid () const;
    bool save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    const MapArrayDistance& distance () const;
    const MapArrayOverlay& water_land () const;

private:
    Generate_DistanceWaterToLand (const Generate_DistanceWaterToLand& other) = delete;
    Generate_DistanceWaterToLand (Generate_DistanceWaterToLand&& other) = delete;

    u32 m_seed;
    bool m_valid_generation;
    MapArrayOverlay m_wl;
    MapArrayDistance m_dist;
};

#endif // GENERATE_DISTANCE_WATER_TO_LAND_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
