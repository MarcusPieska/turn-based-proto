//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_SEAT_SELECTOR_H
#define LUCKY_SEAT_SELECTOR_H

#include "game_primitives.h"
#include "starting_point_generator.h"

class GameArraySimple;

//================================================================================================================================
//=> - LuckySeats -
//================================================================================================================================
//
//  Indices into the start list (player seats chosen as "lucky" for snowball boosts / conquest AI).
//
//================================================================================================================================

struct LuckySeats {
    u16 m_seat[SPG_MAX_PICK_PTS]; // Start indices marked lucky
    u16 m_n; // Valid entries in m_seat
};

//================================================================================================================================
//=> - LuckySeatSelector -
//================================================================================================================================
//
//  Phase 1: for each eligible occupied continent (largest first), stamp the best free land-potential
//  start as lucky until ~10% of starts are marked. Phase 2: DistributeProportional splits leftovers
//  by continent tile count among eligible masses; SelectOnContinent fills them.
//  Each lucky blocks its N nearest other starts from also becoming lucky (phase 1 and 2).
//  Eligible = top k_top masses by tiles, plus any mass with tiles >= k_min_pct% of the largest.
//  Needs TileYields::setup beforehand.
//
//================================================================================================================================

class LuckySeatSelector {
public:
    static const u16 k_pct = 10u; // Target lucky share of starts
    static const u16 k_top = 3u; // Always allow the N largest land masses
    static const u16 k_min_pct = 25u; // Also allow masses >= this % of the largest

    static bool select (const GameArraySimple& map, const SpgPickCoords& starts, LuckySeats* out);

private:
    LuckySeatSelector () = delete;
};

#endif // LUCKY_SEAT_SELECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
