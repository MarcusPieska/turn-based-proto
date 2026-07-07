//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_CONT_OUTLINES_H
#define P1_GEN_CONT_OUTLINES_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h" 

//================================================================================================================================
//=> - P1_ContSzEnt -
//================================================================================================================================

struct P1_ContSzEnt {
    u16 m_pct;
    u16 m_pull_pct;
};

//================================================================================================================================
//=> - P1_ContSzTbl -
//================================================================================================================================

#define P1_CONT_PCT_MAX 20u

struct P1_ContSzTbl {
    P1_ContSzEnt m_ent[P1_CONT_PCT_MAX];
    u8 m_n;
};

//================================================================================================================================
//=> - P1_Gen_ContOutlinesPrm -
//================================================================================================================================

struct P1_Gen_ContOutlinesPrm {
    P1_ContSzTbl m_sz;
    u8 m_main_n;
    u8 m_margin;
    u8 m_ovlp_pct;
};

static inline P1_Gen_ContOutlinesPrm p1_gen_cont_outlines_prm_def () {
    P1_Gen_ContOutlinesPrm p;
    p.m_sz.m_n = 8u;
    p.m_sz.m_ent[0].m_pct = 100u; p.m_sz.m_ent[0].m_pull_pct = 0u;
    p.m_sz.m_ent[1].m_pct = 70u;  p.m_sz.m_ent[1].m_pull_pct = 60u;
    p.m_sz.m_ent[2].m_pct = 70u;  p.m_sz.m_ent[2].m_pull_pct = 40u;
    p.m_sz.m_ent[3].m_pct = 60u;  p.m_sz.m_ent[3].m_pull_pct = 20u;
    p.m_sz.m_ent[4].m_pct = 25u;  p.m_sz.m_ent[4].m_pull_pct = 20u;
    p.m_sz.m_ent[5].m_pct = 20u;  p.m_sz.m_ent[5].m_pull_pct = 20u;
    p.m_sz.m_ent[6].m_pct = 20u;  p.m_sz.m_ent[6].m_pull_pct = 20u;
    p.m_sz.m_ent[7].m_pct = 20u;  p.m_sz.m_ent[7].m_pull_pct = 20u;
    p.m_main_n = 2u;
    p.m_margin = 8u;
    p.m_ovlp_pct = 30u;
    return p;
}

//================================================================================================================================
//=> - P1_Gen_ContOutlinesRslt -
//================================================================================================================================

struct P1_Gen_ContOutlinesRslt {
    u16 m_w;
    u16 m_h;
    u8 m_n;
    P1_ContSzEnt m_ent[P1_CONT_PCT_MAX];
    MapArrayOverlay m_ov;
};

//================================================================================================================================
//=> - P1_Gen_ContOutlines -
//================================================================================================================================
//
//  Per entry: outline overlay; stack-drag-cut-drag-rotate on parents (idx0 random corner).
//
//================================================================================================================================

class P1_Gen_ContOutlines {
public:
    explicit P1_Gen_ContOutlines (
        const P1_RunPrm& prm,
        const P1_Gen_ContOutlinesPrm& sp = p1_gen_cont_outlines_prm_def ());

    bool generate ();
    bool is_valid () const;
    const P1_Gen_ContOutlinesRslt& result () const;

private:
    P1_Gen_ContOutlines (const P1_Gen_ContOutlines& other) = delete;
    P1_Gen_ContOutlines (P1_Gen_ContOutlines&& other) = delete;

    P1_RunPrm m_prm;
    P1_Gen_ContOutlinesPrm m_sp;
    bool m_valid_generation;
    P1_Gen_ContOutlinesRslt m_rslt;
};

void p1_cont_ov_to_gray (const u8* comp, u8* dst, u16 w, u16 h);

bool p1_gen_step01_ov (
    const P1_RunPrm& prm,
    MapArrayOverlay* ov_gray,
    const P1_Gen_ContOutlinesPrm& sp = p1_gen_cont_outlines_prm_def ());

#endif // P1_GEN_CONT_OUTLINES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
