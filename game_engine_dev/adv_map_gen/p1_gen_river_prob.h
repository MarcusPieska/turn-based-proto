//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_PROB_H
#define P1_GEN_RIVER_PROB_H

#include "game_primitives.h"
#include "p1_map_size.h"

class Whiteboard_1B;
class Whiteboard_2B;

//================================================================================================================================
//=> - P1_Gen_RiverProbRslt -
//================================================================================================================================

struct P1_Gen_RiverProbRslt {
    u16 m_w;
    u16 m_h;
    const u16* m_wat_dist;
    const u16* m_eq_near;
    const u16* m_wgt_sum;
    const u8* m_prob;
};

//================================================================================================================================
//=> - P1_Gen_RiverProb -
//================================================================================================================================
//
//  Land distance-to-water BFS and equator nearness; weighted merge normalized and quantized to prob bands.
//
//================================================================================================================================

class P1_Gen_RiverProb {
public:
    explicit P1_Gen_RiverProb (const P1_RunPrm& prm);
    ~P1_Gen_RiverProb ();

    bool generate (const u8* terrain, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_RiverProbRslt& result () const;

private:
    P1_Gen_RiverProb (const P1_Gen_RiverProb& other) = delete;
    P1_Gen_RiverProb (P1_Gen_RiverProb&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_RiverProbRslt m_rslt;
    Whiteboard_2B* m_wb_wat;
    Whiteboard_2B* m_wb_eq;
    Whiteboard_2B* m_wb_wgt;
    Whiteboard_1B* m_wb_prob;
};

#endif // P1_GEN_RIVER_PROB_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
