//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_LINES_H
#define P1_GEN_RIVER_LINES_H

#include "game_primitives.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_RiverLinesRslt -
//================================================================================================================================

struct P1_Gen_RiverLinesRslt {
    u16 m_w;
    u16 m_h;
    u8* m_ov;
    u8* m_dov;
};

//================================================================================================================================
//=> - P1_Gen_RiverLines -
//================================================================================================================================

class P1_Gen_RiverLines {
public:
    explicit P1_Gen_RiverLines (const P1_RunPrm& prm);
    ~P1_Gen_RiverLines ();

    bool generate (
        const u8* terrain,
        u16 w,
        u16 h,
        P1_Gen_RiverPtsRslt& pts,
        const P1_Gen_RiverSectorsRslt& sectors,
        const P1_Gen_RiverNetworkRslt& network,
        const P1_OceanIndexRef& ocean);
    bool is_valid () const;
    const P1_Gen_RiverLinesRslt& result () const;

private:
    P1_Gen_RiverLines (const P1_Gen_RiverLines& other) = delete;
    P1_Gen_RiverLines (P1_Gen_RiverLines&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_RiverLinesRslt m_rslt;
};

#endif // P1_GEN_RIVER_LINES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
