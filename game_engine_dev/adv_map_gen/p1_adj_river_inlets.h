//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_RIVER_INLETS_H
#define P1_ADJ_RIVER_INLETS_H

#include "game_primitives.h"
#include "p1_gen_river_lines.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 river inlet defaults -
//================================================================================================================================

#define P1_RIVER_INLET_PERC_DEF 10u
#define P1_RIVER_INLET_MIN_DEF 3u

//================================================================================================================================
//=> - P1_Adj_RiverInletsPrm -
//================================================================================================================================

struct P1_Adj_RiverInletsPrm {
    u8 m_perc;
    u8 m_min_sz;
};

static inline P1_Adj_RiverInletsPrm p1_adj_river_inlets_prm_def () {
    P1_Adj_RiverInletsPrm p;
    p.m_perc = static_cast<u8>(P1_RIVER_INLET_PERC_DEF);
    p.m_min_sz = static_cast<u8>(P1_RIVER_INLET_MIN_DEF);
    return p;
}

//================================================================================================================================
//=> - P1_Adj_RiverInlets -
//================================================================================================================================

class P1_Adj_RiverInlets {
public:
    explicit P1_Adj_RiverInlets (const P1_RunPrm& prm, const P1_Adj_RiverInletsPrm& sp = p1_adj_river_inlets_prm_def ());

    bool adjust (
        u8* terrain,
        u16 w,
        u16 h,
        const u8* riv,
        const P1_Gen_RiverLinesRslt& lines);
    bool is_valid () const;

private:
    P1_Adj_RiverInlets (const P1_Adj_RiverInlets& other) = delete;
    P1_Adj_RiverInlets (P1_Adj_RiverInlets&& other) = delete;

    P1_RunPrm m_prm;
    P1_Adj_RiverInletsPrm m_sp;
    bool m_valid_adjust;
};

#endif // P1_ADJ_RIVER_INLETS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
