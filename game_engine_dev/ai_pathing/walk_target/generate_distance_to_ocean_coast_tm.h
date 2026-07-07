//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_TO_OCEAN_COAST_TM_H
#define GENERATE_DISTANCE_TO_OCEAN_COAST_TM_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Generate_DistanceToOceanCoast_Tm -
//================================================================================================================================

class Generate_DistanceToOceanCoast_Tm {
public:
    static const u32 k_scr_n = 2u;
    static bool generate (
        const u8* terrain,
        const u8* rivers,
        u16 w,
        u16 h,
        u16* turn,
        u16** scr,
        u16* out_max);

private:
    Generate_DistanceToOceanCoast_Tm () = delete;
    Generate_DistanceToOceanCoast_Tm (const Generate_DistanceToOceanCoast_Tm& other) = delete;
    Generate_DistanceToOceanCoast_Tm (Generate_DistanceToOceanCoast_Tm&& other) = delete;
};

#endif // GENERATE_DISTANCE_TO_OCEAN_COAST_TM_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
