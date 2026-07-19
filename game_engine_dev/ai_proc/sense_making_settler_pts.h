//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SENSE_MAKING_SETTLER_PTS_H
#define SENSE_MAKING_SETTLER_PTS_H

#include "game_state.h"
#include "generator_constants.h"
#include "map_terrain_data.h"
#include "starting_point_generator.h"

class GameArraySimple;

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

#define SMS_MAX_SETTLER_PTS 10000
#define SMS_BEST_N SETTLER_MISSION_SLOTS

//================================================================================================================================
//=> - SmSettlerPt -
//================================================================================================================================

struct SmSettlerPt {
    u16 x;
    u16 y;
};

//================================================================================================================================
//=> - SmSettlerPtResult -
//================================================================================================================================

struct SmSettlerPtResult {
    SmSettlerPt pts[SMS_MAX_SETTLER_PTS];
    u32 n;
};

//================================================================================================================================
//=> - SmSettlerBestPts -
//================================================================================================================================

struct SmSettlerBestPts {
    SmSettlerPt pts[SMS_BEST_N]; // Ranked settle targets; best first
    u16 n; // Valid prefix length; 0 if none
};

//================================================================================================================================
//=> - SenseMakingSettlerPt -
//================================================================================================================================

class SenseMakingSettlerPt {
public:
    static SmSettlerPtResult select (
        const MapTerrainData& map,
        const MapTerrainData& own,
        const MapTerrainData& blk,
        u16 player,
        u16 min_dist,
        u16 max_dist,
        const SpgCoordPair* src, 
        u32 src_n);

    static SmSettlerBestPts pick (
        MapTerrainData& map,
        const SmSettlerPtResult& cand,
        const SpgCoordPair& cap,
        const MapArrayDistance& l2w,
        const GameArraySimple* gmap);

private:
    SenseMakingSettlerPt () = delete;
    SenseMakingSettlerPt (const SenseMakingSettlerPt& other) = delete;
    SenseMakingSettlerPt& operator= (const SenseMakingSettlerPt& other) = delete;
    SenseMakingSettlerPt (SenseMakingSettlerPt&& other) = delete;
    SenseMakingSettlerPt& operator= (SenseMakingSettlerPt&& other) = delete;
};

#endif // SENSE_MAKING_SETTLER_PTS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
