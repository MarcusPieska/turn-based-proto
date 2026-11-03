//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SECT_SELECT_DEFENSIBLE_H
#define SECT_SELECT_DEFENSIBLE_H

#include "expansion_state.h"
#include "game_primitives.h"
#include "gen_land_sectors.h"
#include "land_sector_adj.h"

//================================================================================================================================
//=> - SectorSelect_Defensible -
//================================================================================================================================
//
//  Stateless walk over shared LandSectorAdj. Progress in ExpansionState. First step random; later
//  prefer longest shared border (adj.ln) from the current sector. No city logic.
//
//================================================================================================================================

class SectorSelect_Defensible {
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
    SectorSelect_Defensible () = delete;

    static bool step (
        ExpansionState& st,
        const LandSectorAdj& adj,
        const LandSectorSeedPt* pts,
        u16* out_sector);
};

#endif // SECT_SELECT_DEFENSIBLE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
