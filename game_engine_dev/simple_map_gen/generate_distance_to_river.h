//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_TO_RIVER_H
#define GENERATE_DISTANCE_TO_RIVER_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Generate_DistanceToRiver -
//================================================================================================================================

class Generate_DistanceToRiver {
public:
    static u8* generate (const u8* terrain, u16 w, u16 h, const u8* river_overlay);

private:
    Generate_DistanceToRiver () = delete;
    Generate_DistanceToRiver (const Generate_DistanceToRiver& other) = delete;
    Generate_DistanceToRiver (Generate_DistanceToRiver&& other) = delete;
};

#endif // GENERATE_DISTANCE_TO_RIVER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
