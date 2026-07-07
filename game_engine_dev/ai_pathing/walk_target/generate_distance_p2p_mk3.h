//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_P2P_MK3_H
#define GENERATE_DISTANCE_P2P_MK3_H

#include "game_primitives.h"

class MapBitArrayOverlay;

//================================================================================================================================
//=> - Generate_DistanceP2P_Mk3 -
//================================================================================================================================

class Generate_DistanceP2P_Mk3 {
public:
    static const u32 k_scr_n = 2u;
    static const u8 k_turn_bpv = 4u;
    static const u8 k_step_bpv = 4u;
    static const u32 k_ovl_mod = 15u;
    static const u32 k_turn_sent = 15u;
    static const u32 k_step_sent = 15u;
    static bool generate (
        const u8* terrain,
        const u8* rivers,
        u16 w,
        u16 h,
        u16 src_x,
        u16 src_y,
        u16 dst_x,
        u16 dst_y,
        MapBitArrayOverlay* turn_o,
        MapBitArrayOverlay* step_o,
        u32* pred,
        u16** scr,
        u16* out_max);

private:
    Generate_DistanceP2P_Mk3 () = delete;
    Generate_DistanceP2P_Mk3 (const Generate_DistanceP2P_Mk3& other) = delete;
    Generate_DistanceP2P_Mk3 (Generate_DistanceP2P_Mk3&& other) = delete;
};

#endif // GENERATE_DISTANCE_P2P_MK3_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
