//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_TO_COASTAL_POINT_H
#define GENERATE_DISTANCE_TO_COASTAL_POINT_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Generate_DistanceToCoastalPoint -
//================================================================================================================================

class Generate_DistanceToCoastalPoint {
public:
    static const u16 k_coast_band = 5u;
    static u16* generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const u16* coast_dist,
        u16 px,
        u16 py,
        u16* out_max);

private:
    Generate_DistanceToCoastalPoint () = delete;
    Generate_DistanceToCoastalPoint (const Generate_DistanceToCoastalPoint& other) = delete;
    Generate_DistanceToCoastalPoint (Generate_DistanceToCoastalPoint&& other) = delete;
};

#endif // GENERATE_DISTANCE_TO_COASTAL_POINT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
