//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SECT_SELECT_WALK_H
#define SECT_SELECT_WALK_H

#include "expansion_state.h"
#include "game_primitives.h"
#include "gen_land_sectors.h"
#include "land_sector_adj.h"

//================================================================================================================================
//=> - SectorSelect_Walk -
//================================================================================================================================
//
//  Stateless walk over shared LandSectorAdj. Progress lives only in ExpansionState. First step
//  from start is random; later prefer unvisited neighbor closest (seed Manhattan) to origin.
//
//================================================================================================================================

class SectorSelect_Walk {
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
    SectorSelect_Walk () = delete;

    static bool step (
        ExpansionState& st,
        const LandSectorAdj& adj,
        const LandSectorSeedPt* pts,
        u16* out_sector);
};

#endif // SECT_SELECT_WALK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
