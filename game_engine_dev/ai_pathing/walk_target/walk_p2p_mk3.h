//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WALK_P2P_MK3_H
#define WALK_P2P_MK3_H

#include "game_primitives.h"

class MapBitArrayOverlay;

//================================================================================================================================
//=> - WalkP2P_Mk3 -
//================================================================================================================================

class WalkP2P_Mk3 {
public:
    struct StepRes {
        u16 nx;
        u16 ny;
        u16 cost;
        bool have;
    };
    static u16 move_cost (
        const u8* terrain,
        const u8* rivers,
        u16 w,
        u16 ux,
        u16 uy,
        u16 vx,
        u16 vy);
    StepRes peek_step (
        const MapBitArrayOverlay& turn_o,
        const MapBitArrayOverlay& step_o,
        const u32* pred,
        const u8* terrain,
        const u8* rivers,
        u16 w,
        u16 h,
        u16 x,
        u16 y) const;

private:
    static u32 tidx (u16 w, u16 x, u16 y);
    static bool ovl_reach (const u32* pred, u16 w, u16 x, u16 y);
    static bool ovl_dst (const u32* pred, u16 w, u16 x, u16 y);
    static bool step_cl_u4 (u32 s, u32 ns);
    static bool closer_u4 (u32 t, u32 s, u32 nt, u32 ns);
    static bool pick_nbr (
        const MapBitArrayOverlay& turn_o,
        const MapBitArrayOverlay& step_o,
        const u32* pred,
        u16 w,
        u16 h,
        u16 x,
        u16 y,
        u16& ox,
        u16& oy);
};

#endif // WALK_P2P_MK3_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
