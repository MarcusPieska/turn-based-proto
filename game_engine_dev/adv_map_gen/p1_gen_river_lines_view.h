//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_LINES_VIEW_H
#define P1_GEN_RIVER_LINES_VIEW_H

#include "game_primitives.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"

//================================================================================================================================
//=> - P1_Gen_RiverLinesView -
//================================================================================================================================
//
//  Viewable PPM output for P1_Gen_RiverLines: river overlay and link distance field.
//
//================================================================================================================================

class P1_Gen_RiverLinesView {
public:
    static bool save_pri (
        cstr path,
        const u8* terrain,
        u16 w,
        u16 h,
        const P1_Gen_RiverLinesRslt& r,
        const P1_Gen_RiverNetworkRslt& network,
        const P1_Gen_RiverPtsRslt& pts);
    static bool save_dist (
        cstr path,
        const u8* terrain,
        u16 w,
        u16 h,
        const P1_Gen_RiverLinesRslt& r);
};

#endif // P1_GEN_RIVER_LINES_VIEW_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
