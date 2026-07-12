//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_COASTAL_MTN_RIVERS_H
#define P1_ADJ_COASTAL_MTN_RIVERS_H

#include "game_primitives.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "p1_gen_river_sectors.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Adj_CoastalMtnRivers -
//================================================================================================================================
//
//  One river per grey coastal-mtn sector: glob-water coast to interior limit border.
//
//================================================================================================================================

class P1_Adj_CoastalMtnRivers {
public:
    explicit P1_Adj_CoastalMtnRivers (const P1_RunPrm& prm);

    bool adjust (
        const u8* terrain,
        u16 w,
        u16 h,
        u8* riv,
        const P1_Gen_RiverSectorsRslt& sectors,
        const P1_Gen_CoastalMtnLimitsRslt& coast_lim); 
    
    bool is_valid () const;
    u32 path_n () const;

private:
    P1_Adj_CoastalMtnRivers (const P1_Adj_CoastalMtnRivers& other) = delete;
    P1_Adj_CoastalMtnRivers (P1_Adj_CoastalMtnRivers&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_adjust;
    u32 m_path_n;
};

#endif // P1_ADJ_COASTAL_MTN_RIVERS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
