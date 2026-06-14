//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_WATERSHED_MTN_LINES_H
#define GENERATE_WATERSHED_MTN_LINES_H

#include "game_primitives.h"
#include "generate_river_network.h"

//================================================================================================================================
//=> - WatershedBorderSeg -
//================================================================================================================================

struct WatershedBorderSeg {
    u16 basin_a;
    u16 basin_b;
    u16 mouth_ax;
    u16 mouth_ay;
    u16 mouth_bx;
    u16 mouth_by;
    u32 mouth_dist;
    u16 ov_idx;
    
    u32 tile_n;
    u16 a_plains;
    u16 a_hills;
    u16 b_plains;
    u16 b_hills;
};

//================================================================================================================================
//=> - WatershedMtnLinesResult -
//================================================================================================================================

struct WatershedMtnLinesResult {
    u16 w;
    u16 h;
    u16 seg_n;
    u32 border_tile_n;
    WatershedBorderSeg* segs;
    u16* overlay;
};

//================================================================================================================================
//=> - Generate_WatershedMtnLines -
//================================================================================================================================

class Generate_WatershedMtnLines {
public:
    static WatershedMtnLinesResult* generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const RiverNetworkResult* network);
    static void free_result (WatershedMtnLinesResult* res);

private:
    Generate_WatershedMtnLines () = delete;
    Generate_WatershedMtnLines (const Generate_WatershedMtnLines& other) = delete;
    Generate_WatershedMtnLines (Generate_WatershedMtnLines&& other) = delete;
};

#endif // GENERATE_WATERSHED_MTN_LINES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
