//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_SECTORS_H
#define P1_GEN_RIVER_SECTORS_H

#include "game_primitives.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_pts.h"
#include "p1_map_size.h"

class Whiteboard_1B;
class Whiteboard_2B;

//================================================================================================================================
//=> - P1 river sector limits -
//================================================================================================================================

#define P1_RIVER_SECTOR_NONE 0xFFFFu
#define P1_RIVER_SECTOR_OFF_RAD 20
#define P1_RIVER_SECTOR_OFF_SPAN (P1_RIVER_SECTOR_OFF_RAD * 2 + 1)
#define P1_RIVER_SECTOR_DIST_TILE_N (static_cast<u32>(P1_RIVER_SECTOR_OFF_SPAN) * static_cast<u32>(P1_RIVER_SECTOR_OFF_SPAN))

//================================================================================================================================
//=> - P1_Gen_RiverSectorsRslt -
//================================================================================================================================

struct P1_Gen_RiverSectorsRslt {
    u16 m_w;
    u16 m_h;
    u16 m_ocn_sec_n;
    u16 m_sector_n;
    u16* m_ov;
    const u8* m_dist_ov;
};

//================================================================================================================================
//=> - P1_Gen_RiverSectors -
//================================================================================================================================
//
//  Tile sector ownership from river source points; m_ov[ti] is sector id or P1_RIVER_SECTOR_NONE; m_dist_ov is 41x41 eucl off mask.
//
//================================================================================================================================

class P1_Gen_RiverSectors {
public:
    explicit P1_Gen_RiverSectors (const P1_RunPrm& prm);
    ~P1_Gen_RiverSectors ();

    bool generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const P1_Gen_RiverPtsRslt& pts,
        const P1_OceanIndexRef& ocean);
    bool is_valid () const;
    const P1_Gen_RiverSectorsRslt& result () const;

private:
    P1_Gen_RiverSectors (const P1_Gen_RiverSectors& other) = delete;
    P1_Gen_RiverSectors (P1_Gen_RiverSectors&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_RiverSectorsRslt m_rslt;
    Whiteboard_2B* m_wb_ov;
    Whiteboard_1B* m_wb_dist;
    u8 m_dist_u8[P1_RIVER_SECTOR_DIST_TILE_N];
};

#endif // P1_GEN_RIVER_SECTORS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
