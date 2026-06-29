//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_P2P_MK4_H
#define GENERATE_DISTANCE_P2P_MK4_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Generate_DistanceP2P_Mk4 -
//================================================================================================================================

class Generate_DistanceP2P_Mk4 {
public:
    static const u32 k_scr_n = 3u;
    static const u16 k_dist_sent = 0xFFFFu;
    static bool generate (
        const u8* terrain,
        const u8* rivers,
        u16 w,
        u16 h,
        u16 src_x,
        u16 src_y,
        u16 dst_x,
        u16 dst_y,
        u16* dist_o,
        u32* pred,
        u16** scr,
        u16* out_max);

private:
    Generate_DistanceP2P_Mk4 () = delete;
    Generate_DistanceP2P_Mk4 (const Generate_DistanceP2P_Mk4& other) = delete;
    Generate_DistanceP2P_Mk4 (Generate_DistanceP2P_Mk4&& other) = delete;
};

#endif // GENERATE_DISTANCE_P2P_MK4_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
