//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TARGET_SECTOR_DEFENSIBLE_H
#define TARGET_SECTOR_DEFENSIBLE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - TargetSector_Defensible -
//================================================================================================================================
//
//  Picks a land sector and enemy seat for war declaration. Prefers any shared sector; otherwise the
//  adjacent sector with the longest collective border (sum of link m_len) against our fully owned
//  sectors. Fills the caller's city-index queue. Requires SectorSupport::bind.
//
//================================================================================================================================

class TargetSector_Defensible {
public:
    static bool pick (
        u16 player,
        u16* tgts,
        u16 cap,
        u16* out_n,
        u16* out_sector,
        u16* out_enemy);

private:
    TargetSector_Defensible () = delete;
};

#endif // TARGET_SECTOR_DEFENSIBLE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
