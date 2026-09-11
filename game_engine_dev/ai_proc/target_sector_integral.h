//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TARGET_SECTOR_INTEGRAL_H
#define TARGET_SECTOR_INTEGRAL_H

#include "game_primitives.h"

//================================================================================================================================
//=> - TargetSector_Integral -
//================================================================================================================================
//
//  Picks a land sector and enemy seat for war declaration. Prefers any shared sector; otherwise the
//  best integral neighbor. Fills the caller's city-index queue with that enemy's cities in the
//  sector (same buffer shape as WarTurnHandler::m_tgts). Requires SectorSupport::bind.
//
//================================================================================================================================

class TargetSector_Integral {
public:
    static bool pick (
        u16 player,
        u16* tgts,
        u16 cap,
        u16* out_n,
        u16* out_sector,
        u16* out_enemy);

private:
    TargetSector_Integral () = delete;
};

#endif // TARGET_SECTOR_INTEGRAL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
