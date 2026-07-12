//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_DELTA_SWAMPS_H
#define P1_ADJ_DELTA_SWAMPS_H

#include "game_primitives.h"
#include "p1_map_config.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 delta swamp adjust defaults -
//================================================================================================================================

#define P1_DELTA_SWAMP_TGT_PCT_DEF 1u

//================================================================================================================================
//=> - P1_Adj_DeltaSwampsPrm -
//================================================================================================================================

struct P1_Adj_DeltaSwampsPrm {
    u16 m_tgt_pct;
};

static inline P1_Adj_DeltaSwampsPrm p1_adj_delta_swamps_prm_def () {
    P1_Adj_DeltaSwampsPrm p;
    p.m_tgt_pct = static_cast<u16>(P1_DELTA_SWAMP_TGT_PCT_DEF);
    return p;
}

static inline P1_Adj_DeltaSwampsPrm p1_adj_delta_swamps_prm_from_cfg (const P1_MapConfig& cfg) {
    P1_Adj_DeltaSwampsPrm p = p1_adj_delta_swamps_prm_def();
    p.m_tgt_pct = cfg.m_delta_flood_perc;
    return p;
}

//================================================================================================================================
//=> - P1_Adj_DeltaSwampsIn -
//================================================================================================================================

struct P1_Adj_DeltaSwampsIn {
    const u16* m_wshed;
    const u8* m_terrain;
    u8* m_climate;
    u8* m_res_ov;
    const u8* m_riv;
};

//================================================================================================================================
//=> - P1_Adj_DeltaSwamps -
//================================================================================================================================
//
//  Score watershed terrain/climate water weight; per-basin delta size from water tally; global cap from tgt_pct land share.
//
//================================================================================================================================

class P1_Adj_DeltaSwamps {
public:
    explicit P1_Adj_DeltaSwamps (
        const P1_RunPrm& prm,
        const P1_Adj_DeltaSwampsPrm& sp = p1_adj_delta_swamps_prm_def ());

    bool adjust (const P1_Adj_DeltaSwampsIn& in, u16 w, u16 h);
    bool is_valid () const;
    u32 swamp_n () const;
    u32 delta_n () const;

private:
    P1_Adj_DeltaSwamps (const P1_Adj_DeltaSwamps& other) = delete;
    P1_Adj_DeltaSwamps (P1_Adj_DeltaSwamps&& other) = delete;

    P1_RunPrm m_prm;
    P1_Adj_DeltaSwampsPrm m_sp;
    bool m_valid_adjust;
    u32 m_swamp_n;
    u32 m_delta_n;
};

#endif // P1_ADJ_DELTA_SWAMPS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
