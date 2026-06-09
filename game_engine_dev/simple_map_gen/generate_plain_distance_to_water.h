//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_PLAIN_DISTANCE_TO_WATER_H
#define GENERATE_PLAIN_DISTANCE_TO_WATER_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Generate_PlainDistanceToWater -
//================================================================================================================================

class Generate_PlainDistanceToWater {
public:
    static u8* generate (const u8* terrain, u16 w, u16 h);

private:
    Generate_PlainDistanceToWater () = delete;
    Generate_PlainDistanceToWater (const Generate_PlainDistanceToWater& other) = delete;
    Generate_PlainDistanceToWater (Generate_PlainDistanceToWater&& other) = delete;
};

#endif // GENERATE_PLAIN_DISTANCE_TO_WATER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
