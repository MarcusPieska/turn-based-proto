//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SETTLE_RULES_H
#define SETTLE_RULES_H

#include "game_map_defs.h"
#include "game_primitives.h"

class GameArraySimple;

//================================================================================================================================
//=> - SettleRules -
//================================================================================================================================
//
//  Geography checks for founding a city. Plains/hills only; grassland, plains, and black soil always;
//  desert only with a river. Blocking, ownership, and planned bits stay with the caller.
//
//================================================================================================================================

class SettleRules {
public:
    static bool terr_ok (u8 terr);
    static bool clim_ok (u8 clim, u8 riv);
    static bool tile_ok (const GameArraySimple& map, u16 x, u16 y);

private:
    SettleRules () = delete;
};

inline bool SettleRules::terr_ok (u8 terr) {
    return terr == TERR_PLAINS[0] || terr == TERR_HILLS[0];
}

inline bool SettleRules::clim_ok (u8 clim, u8 riv) {
    if (clim == CLIMATE_BLACK_SOIL || clim == CLIMATE_GRASSLAND || clim == CLIMATE_PLAINS) {
        return true;
    }
    return clim == CLIMATE_DESERT && riv != 0;
}

#endif // SETTLE_RULES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
