//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_OUTLINE_SHAPING_H
#define P1_ADJ_OUTLINE_SHAPING_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Adj_OutlineShapingPrm -
//================================================================================================================================

struct P1_Adj_OutlineShapingPrm {
    f32 m_inner_grad_limit;
};

static inline P1_Adj_OutlineShapingPrm p1_adj_outline_shaping_prm_def () {
    P1_Adj_OutlineShapingPrm p;
    p.m_inner_grad_limit = 0.8f;
    return p;
}

//================================================================================================================================
//=> - P1_Adj_OutlineShaping -
//================================================================================================================================

class P1_Adj_OutlineShaping {
public:
    explicit P1_Adj_OutlineShaping (const P1_RunPrm& prm, const P1_Adj_OutlineShapingPrm& sp);

    bool adjust (u8* terrain, u16 w, u16 h, const u8* ol_ov, const u16* land_depth);
    bool is_valid () const;

private:
    P1_Adj_OutlineShaping (const P1_Adj_OutlineShaping& other) = delete;
    P1_Adj_OutlineShaping (P1_Adj_OutlineShaping&& other) = delete;

    P1_RunPrm m_prm;
    P1_Adj_OutlineShapingPrm m_sp;
    bool m_valid_adjust;
};

#endif // P1_ADJ_OUTLINE_SHAPING_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
