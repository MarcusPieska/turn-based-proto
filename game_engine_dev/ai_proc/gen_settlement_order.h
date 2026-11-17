//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_SETTLEMENT_ORDER_H
#define GEN_SETTLEMENT_ORDER_H

#include "game_primitives.h"
#include "starting_point_generator.h"

class GameArraySimple;

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

#define GSO_MAX_PN 200

//================================================================================================================================
//=> - GenSettlementOrder -
//================================================================================================================================
//
//  Suggested settle order from AI_TILE_OV_INTENT_CITY stamps. Land floods from starts; a site is appended when first covered.
//  gen_excl: floods block, each site in at most one list. gen_all: one start, every reachable planned site.
//  Per-player results are tight heap SpgCoordPair arrays. Flood scratch whiteboards are local to gen and released after.
//  Live [hd, tl) are indices into that player's array (st is always 0, en == n). Cap GSO_MAX_PN players.
//
//================================================================================================================================

class GenSettlementOrder {
public:
    GenSettlementOrder ();
    ~GenSettlementOrder ();
    bool gen_excl (const GameArraySimple& map, const SpgCoordPair* starts, u32 start_n);
    bool gen_all (const GameArraySimple& map, u16 sx, u16 sy);
    void clr ();
    bool ok () const;
    u16 pn () const;
    u32 n (u16 p) const;
    u32 st (u16 p) const;
    u32 en (u16 p) const;
    u32 hd (u16 p) const;
    u32 tl (u16 p) const;
    u32 bytes () const;
    SpgCoordPair at (u16 p, u32 i) const;

private:
    GenSettlementOrder (const GenSettlementOrder& other) = delete;
    GenSettlementOrder& operator= (const GenSettlementOrder& other) = delete;
    GenSettlementOrder (GenSettlementOrder&& other) = delete;
    GenSettlementOrder& operator= (GenSettlementOrder&& other) = delete;

    SpgCoordPair* m_pts[GSO_MAX_PN]; // Per-player tight site list
    u32 m_n[GSO_MAX_PN]; // Per-player site count
    u32 m_hd[GSO_MAX_PN]; // Per-player live head
    u32 m_tl[GSO_MAX_PN]; // Per-player live tail (one-past)
    u16 m_pn; // Player list count
    bool m_ok; // True when ready for gen / query
};

#endif // GEN_SETTLEMENT_ORDER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
