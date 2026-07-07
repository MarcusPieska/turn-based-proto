//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_TO_OCEAN_COAST_H
#define GENERATE_DISTANCE_TO_OCEAN_COAST_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Generate_DistanceToOceanCoast -
//================================================================================================================================

class Generate_DistanceToOceanCoast {
public:
    static u16* generate (const u8* terrain, u16 w, u16 h, u16* out_max);

private:
    Generate_DistanceToOceanCoast () = delete;
    Generate_DistanceToOceanCoast (const Generate_DistanceToOceanCoast& other) = delete;
    Generate_DistanceToOceanCoast (Generate_DistanceToOceanCoast&& other) = delete;
};

#endif // GENERATE_DISTANCE_TO_OCEAN_COAST_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
