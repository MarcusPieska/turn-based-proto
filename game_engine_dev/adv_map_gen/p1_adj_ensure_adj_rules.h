//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_ENSURE_ADJ_RULES_H
#define P1_ADJ_ENSURE_ADJ_RULES_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Adj_EnsureAdjRules -
//================================================================================================================================
//
//  Propagate terrain and climate adjacency rules until fixpoint via queued re-checks.
//
//================================================================================================================================

class P1_Adj_EnsureAdjRules { 
public:
    explicit P1_Adj_EnsureAdjRules (const P1_RunPrm& prm);

    bool adjust (u8* terrain, u8* climate, const u8* river, u16 w, u16 h);
    bool is_valid () const;
    u32 terr_chg_n () const;
    u32 clim_chg_n () const;
    u32 que_peak_n () const;

private:
    P1_Adj_EnsureAdjRules (const P1_Adj_EnsureAdjRules& other) = delete;
    P1_Adj_EnsureAdjRules (P1_Adj_EnsureAdjRules&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_adjust;
    u32 m_terr_chg_n;
    u32 m_clim_chg_n;
    u32 m_que_peak_n;
};

#endif // P1_ADJ_ENSURE_ADJ_RULES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
