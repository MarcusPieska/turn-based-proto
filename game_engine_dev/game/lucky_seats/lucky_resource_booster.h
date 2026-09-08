//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_RESOURCE_BOOSTER_H
#define LUCKY_RESOURCE_BOOSTER_H

#include "game_primitives.h"
#include "starting_point_generator.h"

class GameArraySimple;
class RuntimeStatics;

//================================================================================================================================
//=> - LuckyResourceBooster -
//================================================================================================================================
//
//  boost_local: duplicate snapshot resources in r=30 then r=10 around each lucky start.
//  boost_river: find nearest FOOD_CROP / LIVESTOCK, plant one crop under the seat, then scatter 20 of
//  each along the nearest river (on/adjacent, non-desert). Call before ResourceTurnHandler::setup.
//
//================================================================================================================================

class LuckyResourceBooster {
public:
    static const u16 k_r_outer = 30u; // First duplication radius
    static const u16 k_r_inner = 10u; // Second duplication radius
    static const u16 k_river_n = 20u; // Copies of each found type along the river
    static const u16 k_riv_stride = 5u; // Place about every 5th corridor tile (~20% density)

    static bool boost_local (GameArraySimple& map, const SpgCoordPair* pts, u16 n, u32* out_added);
    static bool boost_river (
        GameArraySimple& map,
        const RuntimeStatics& st,
        const SpgCoordPair* pts,
        u16 n,
        u32* out_added);

private:
    LuckyResourceBooster () = delete;
};

#endif // LUCKY_RESOURCE_BOOSTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
