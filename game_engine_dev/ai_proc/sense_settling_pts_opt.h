//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SENSE_SETTLING_PTS_OPT_H
#define SENSE_SETTLING_PTS_OPT_H

#include "game_state.h"
#include "sense_making_settler_pts.h"

//================================================================================================================================
//=> - SenseSettlingPtsOpt -
//================================================================================================================================
//
//  Per frontier city: outer flood seeded on CircularTileAreas ring at k_r_near, limited to Euclidean disk k_r_outer.
//  First river (city, near disk, or outer front) runs inner 41-ish river flood (queue ~20). Premiums stamp excl via
//  CityBlockingMask::preview. Best NP: plains=1 / grassland=2, first wins ties, grassland stops NP updates.
//  Flood visited uses rolling ids. Spill best-NPs if short of SMS_BEST_N. Needs WhiteboardMng::init.
//
//================================================================================================================================

class SenseSettlingPtsOpt {
public:
    static SmSettlerBestPts select_and_pick_pts (const GameArraySimple& map, CityArray& cities, u16 player);

private:
    SenseSettlingPtsOpt () = delete;
    SenseSettlingPtsOpt (const SenseSettlingPtsOpt& other) = delete;
    SenseSettlingPtsOpt& operator= (const SenseSettlingPtsOpt& other) = delete;
    SenseSettlingPtsOpt (SenseSettlingPtsOpt&& other) = delete;
    SenseSettlingPtsOpt& operator= (SenseSettlingPtsOpt&& other) = delete;
};

#endif // SENSE_SETTLING_PTS_OPT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
