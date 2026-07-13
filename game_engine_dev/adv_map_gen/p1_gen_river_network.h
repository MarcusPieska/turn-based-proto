//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_NETWORK_H
#define P1_GEN_RIVER_NETWORK_H

#include "game_primitives.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sect_adj.h"
#include "p1_gen_river_sectors.h"
#include "p1_map_size.h"

class Whiteboard_2B;

//================================================================================================================================
//=> - P1 river network limits -
//================================================================================================================================

#define P1_RIVER_DOWN_UNDEF 0xFFFFu
#define P1_RIVER_DOWN_MOUTH 0xFFFEu
#define P1_RIVER_BASIN_NONE 0u
#define P1_RIVER_SYS_NONE 0xFFFFu 

//================================================================================================================================
//=> - P1_Gen_RiverNetworkRslt -
//================================================================================================================================

struct P1_Gen_RiverNetworkRslt {
    u16 m_w;
    u16 m_h;
    u16 m_sector_n;
    u16 m_mouth_n;
    u16 m_claim_n;
    const u16* m_downstream;
    u16* m_ov;
};

//================================================================================================================================
//=> - P1_Gen_RiverNetwork -
//================================================================================================================================
//
//  Per-sector downstream link; m_downstream[si] is next sector toward mouth, 0 mouth, 0xFFFF undefined.
//
//================================================================================================================================

class P1_Gen_RiverNetwork {
public:
    explicit P1_Gen_RiverNetwork (const P1_RunPrm& prm);
    ~P1_Gen_RiverNetwork ();

    bool generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const P1_Gen_RiverPtsRslt& pts,
        const P1_Gen_RiverSectorsRslt& sectors,
        const P1_Gen_RiverSectAdjRslt& sect_adj,
        const P1_Gen_CoastalMtnLimitsRslt& coast_lim,
        const P1_OceanIndexRef& ocean);
    bool is_valid () const;
    const P1_Gen_RiverNetworkRslt& result () const;

private:
    P1_Gen_RiverNetwork (const P1_Gen_RiverNetwork& other) = delete;
    P1_Gen_RiverNetwork (P1_Gen_RiverNetwork&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_RiverNetworkRslt m_rslt;
    Whiteboard_2B* m_wb_down;
    Whiteboard_2B* m_wb_basin;
};

#endif // P1_GEN_RIVER_NETWORK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
