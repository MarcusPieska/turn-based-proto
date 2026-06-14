//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ADJUST_STAMP_MTN_LINE_H
#define ADJUST_STAMP_MTN_LINE_H

#include "game_primitives.h"
#include "generate_small_area_mtn_line_dp.h"

//================================================================================================================================
//=> - StampMtnLineParams -
//================================================================================================================================

struct StampMtnLineParams {
    f64 m_oversize_factor = 1.0;
    u16 m_oversize_px = 0;
};

struct StampMtnLineOversizeLvl {
    i32 m_stamp_perc;
    f64 m_oversize_factor;
    u16 m_oversize_px;
};

//================================================================================================================================
//=> - StampMtnLine -
//================================================================================================================================

class Adjust_StampMtnLine {
public:
    explicit Adjust_StampMtnLine (u32 seed);

    u32 stamp_dp (
        u8* terrain,
        u16 w,
        u16 h,
        const SmallAreaMtnLineDpResult* dp,
        const StampMtnLineParams& params);
    bool is_valid () const;

private:
    Adjust_StampMtnLine (const Adjust_StampMtnLine& other) = delete;
    Adjust_StampMtnLine (Adjust_StampMtnLine&& other) = delete;

    u32 m_seed;
    bool m_valid_adjust;
};

#endif // ADJUST_STAMP_MTN_LINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
