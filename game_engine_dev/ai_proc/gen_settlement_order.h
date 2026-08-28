//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_SETTLEMENT_ORDER_H
#define GEN_SETTLEMENT_ORDER_H

#include "game_primitives.h"
#include "starting_point_generator.h"
#include "whiteboard_mng.h"

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
//  Packed x/y sit on two Whiteboard_2B slabs held until destruction. Sloppy pack: st = p * spot_n; if that
//  overruns tile_n, the board is split as st = p * tile_n / pn. Writes stop at the next list's st.
//  Per-player [st, en) and live [hd, tl) are stack arrays, cap GSO_MAX_PN. Needs WhiteboardMng::init first.
//
//================================================================================================================================

class GenSettlementOrder {
public:
    GenSettlementOrder ();
    ~GenSettlementOrder () = default;
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
    SpgCoordPair at (u16 p, u32 i) const;

private:
    GenSettlementOrder (const GenSettlementOrder& other) = delete;
    GenSettlementOrder& operator= (const GenSettlementOrder& other) = delete;
    GenSettlementOrder (GenSettlementOrder&& other) = delete;
    GenSettlementOrder& operator= (GenSettlementOrder&& other) = delete;

    Whiteboard_2B m_xs; // Packed suggestion x
    Whiteboard_2B m_ys; // Packed suggestion y
    u16 m_pn; // Player list count
    u32 m_ptn; // Total packed suggestion pts
    u32 m_st[GSO_MAX_PN]; // Per-player packed start
    u32 m_en[GSO_MAX_PN]; // Per-player packed stop (one-past)
    u32 m_hd[GSO_MAX_PN]; // Per-player live head
    u32 m_tl[GSO_MAX_PN]; // Per-player live tail (one-past)
};

#endif // GEN_SETTLEMENT_ORDER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
