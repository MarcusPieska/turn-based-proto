//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SECT_SELECT_INTEGRAL_H
#define SECT_SELECT_INTEGRAL_H

#include "expansion_state.h"
#include "game_primitives.h"
#include "gen_land_sectors.h"
#include "land_sector_adj.h"

//================================================================================================================================
//=> - SectorSelect_Integral -
//================================================================================================================================
//
//  Stateless walk over shared LandSectorAdj. Progress in ExpansionState. First step random; later
//  prefer best integral score vs visited set. No city logic.
//
//================================================================================================================================

class SectorSelect_Integral {
public:
    static bool begin (
        ExpansionState& st,
        const LandSectorAdj& adj,
        const LandSectorSeedPt* pts,
        u16 pt_n,
        u32 rng);
    static bool select (
        ExpansionState& st,
        const LandSectorAdj& adj,
        const LandSectorSeedPt* pts,
        u16* out_sector);

private:
    SectorSelect_Integral () = delete;

    static bool step (
        ExpansionState& st,
        const LandSectorAdj& adj,
        const LandSectorSeedPt* pts,
        u16* out_sector);
    static i32 score (ExpansionState& st, const LandSectorAdj& adj, u16 n);
};

#endif // SECT_SELECT_INTEGRAL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
