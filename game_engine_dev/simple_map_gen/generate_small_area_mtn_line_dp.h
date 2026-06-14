//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_SMALL_AREA_MTN_LINE_DP_H
#define GENERATE_SMALL_AREA_MTN_LINE_DP_H

#include "game_primitives.h"
#include "generate_watershed_mtn_lines.h"

//================================================================================================================================
//=> - SmallAreaMtnLineDpPt -
//================================================================================================================================

struct SmallAreaMtnLineDpPt {
    u16 x;
    u16 y;
};

//================================================================================================================================
//=> - SmallAreaMtnLineDpChain -
//================================================================================================================================

struct SmallAreaMtnLineDpChain {
    u16 ov_idx;
    u16 basin_a;
    u16 basin_b;
    SmallAreaMtnLineDpPt end_a;
    SmallAreaMtnLineDpPt end_b;
    u32 border_tile_n;
    SmallAreaMtnLineDpPt* border_tiles;
    u16 pt_n;
    SmallAreaMtnLineDpPt* pts;
};

//================================================================================================================================
//=> - SmallAreaMtnLineDpResult -
//================================================================================================================================

struct SmallAreaMtnLineDpResult {
    u16 w;
    u16 h;
    u16 seg_i;
    u16 chain_n;
    SmallAreaMtnLineDpChain* chains;
};

//================================================================================================================================
//=> - SmallAreaMtnLineDpParams -
//================================================================================================================================

struct SmallAreaMtnLineDpParams {
    u32 m_min_split_tile_n = 25;
    f64 m_split_angle_deg = 150.0;
    f64 m_min_seg_len = 8.0;
};

//================================================================================================================================
//=> - Generate_SmallAreaMtnLineDp -
//================================================================================================================================

class Generate_SmallAreaMtnLineDp {
public:
    static SmallAreaMtnLineDpResult* generate (
        const WatershedMtnLinesResult* mtns,
        u16 seg_i,
        const SmallAreaMtnLineDpParams& params);
    static void free_result (SmallAreaMtnLineDpResult* res);

private:
    Generate_SmallAreaMtnLineDp () = delete;
    Generate_SmallAreaMtnLineDp (const Generate_SmallAreaMtnLineDp& other) = delete;
    Generate_SmallAreaMtnLineDp (Generate_SmallAreaMtnLineDp&& other) = delete;
};

#endif // GENERATE_SMALL_AREA_MTN_LINE_DP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
