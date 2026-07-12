//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_DIST_H
#define P1_GEN_RIVER_DIST_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_sectors.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 river dist limits -
//================================================================================================================================

#define P1_RIVER_DIST_HEAD_MAX 200u

//================================================================================================================================
//=> - P1_Gen_RiverDistRslt -
//================================================================================================================================

struct P1_Gen_RiverDistRslt {
    u16 m_w;
    u16 m_h;
    u16 m_max_up;
    u16 m_max_dn;
    MapArrayDistance m_up;
    MapArrayDistance m_dn;
};

//================================================================================================================================
//=> - P1_Gen_RiverDist -
//================================================================================================================================
//
//  Per-basin river overlays: upstream distance from mouth, downstream distance to mouth.
//
//================================================================================================================================

class P1_Gen_RiverDist {
public:
    explicit P1_Gen_RiverDist (const P1_RunPrm& prm);

    bool generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const u8* riv,
        const P1_Gen_RiverSectorsRslt& sectors,
        const P1_Gen_RiverNetworkRslt& network);
    bool is_valid () const;
    const P1_Gen_RiverDistRslt& result () const;

private:
    P1_Gen_RiverDist (const P1_Gen_RiverDist& other) = delete;
    P1_Gen_RiverDist (P1_Gen_RiverDist&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_RiverDistRslt m_rslt;
};

#endif // P1_GEN_RIVER_DIST_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
