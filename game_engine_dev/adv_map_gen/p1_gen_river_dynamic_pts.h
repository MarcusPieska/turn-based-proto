//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_DYNAMIC_PTS_H
#define P1_GEN_RIVER_DYNAMIC_PTS_H

#include "game_primitives.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_prob.h"
#include "p1_gen_river_pts.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_RiverDynamicPts -
//================================================================================================================================
//
//  Merges multi-step P1_Gen_RiverPts layers by prob band; output matches P1_Gen_RiverPtsRslt.
//
//================================================================================================================================

class P1_Gen_RiverDynamicPts {
public:
    explicit P1_Gen_RiverDynamicPts (const P1_RunPrm& prm);
    ~P1_Gen_RiverDynamicPts ();

    bool generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const P1_Gen_RiverProbRslt& prob,
        const P1_OceanIndexRef& ocean);
    bool is_valid () const;
    const P1_Gen_RiverPtsRslt& result () const;

private:
    P1_Gen_RiverDynamicPts (const P1_Gen_RiverDynamicPts& other) = delete;
    P1_Gen_RiverDynamicPts (P1_Gen_RiverDynamicPts&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_RiverPtsRslt m_rslt; 
};

#endif // P1_GEN_RIVER_DYNAMIC_PTS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
