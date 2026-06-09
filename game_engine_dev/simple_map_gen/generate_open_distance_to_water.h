//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_OPEN_DISTANCE_TO_WATER_H
#define GENERATE_OPEN_DISTANCE_TO_WATER_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Generate_OpenDistanceToWater -
//================================================================================================================================

class Generate_OpenDistanceToWater {
public:
    static u8* generate (const u8* terrain, u16 w, u16 h);

private:
    Generate_OpenDistanceToWater () = delete;
    Generate_OpenDistanceToWater (const Generate_OpenDistanceToWater& other) = delete;
    Generate_OpenDistanceToWater (Generate_OpenDistanceToWater&& other) = delete;
};

#endif // GENERATE_OPEN_DISTANCE_TO_WATER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
