//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_P2P_MK1_H
#define GENERATE_DISTANCE_P2P_MK1_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Generate_DistanceP2P_Mk1 -
//================================================================================================================================

class Generate_DistanceP2P_Mk1 {
public:
    static const u32 k_scr_n = 2u;
    static bool generate (
        const u8* terrain,
        const u8* rivers,
        u16 w,
        u16 h,
        u16 src_x,
        u16 src_y,
        u16 dst_x,
        u16 dst_y,
        u16* turn,
        u16* step,
        u32* pred,
        u16** scr,
        u16* out_max);

private:
    Generate_DistanceP2P_Mk1 () = delete;
    Generate_DistanceP2P_Mk1 (const Generate_DistanceP2P_Mk1& other) = delete;
    Generate_DistanceP2P_Mk1 (Generate_DistanceP2P_Mk1&& other) = delete;
};

#endif // GENERATE_DISTANCE_P2P_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
