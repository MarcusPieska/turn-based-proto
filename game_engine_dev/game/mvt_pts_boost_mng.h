//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MVT_PTS_BOOST_MNG_H
#define MVT_PTS_BOOST_MNG_H

#include "game_primitives.h"

class GameState;
struct UnitAddStruct;

//================================================================================================================================
//=> - MvtPtsBoostMng -
//================================================================================================================================
//
//  Per-seat LAND_MVT_PTS / SEA_MVT_PTS cache (DynBoosterRegister, CIV scope). refresh once per turn;
//  fill/budget apply base PATH_MP_TURN * unit.mvt_pts + domain boost.
//
//================================================================================================================================

class MvtPtsBoostMng { 
public:
    static bool setup (u16 player_n);
    static void clear ();
    static void refresh (GameState& s);

    static u16 land (u16 seat);
    static u16 sea (u16 seat);
    static i32 budget (const GameState& s, const UnitAddStruct& u);
    static void fill (GameState& s, UnitAddStruct* u);

private:
    MvtPtsBoostMng () = delete;

    static u16* m_land; // LAND_MVT_PTS sum per seat; multiples of 500
    static u16* m_sea; // SEA_MVT_PTS sum per seat; multiples of 500
    static u16 m_n; // Seat count / array length
};

#endif // MVT_PTS_BOOST_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
