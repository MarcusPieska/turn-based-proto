//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_LINES_H
#define P1_GEN_RIVER_LINES_H

#include "game_primitives.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_sectors.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 river line limits -
//================================================================================================================================

#define P1_RIVER_LINE_SYS_NONE 0xFFFFu

//================================================================================================================================
//=> - P1_RiverSysEntry -
//================================================================================================================================

struct P1_RiverSysEntry {
    u16 m_mx;
    u16 m_my;
    u16 m_max_d;
    u32 m_tile_n;
};

//================================================================================================================================
//=> - P1_Gen_RiverLinesRslt -
//================================================================================================================================

struct P1_Gen_RiverLinesRslt {
    u16 m_w;
    u16 m_h;
    u8* m_ov;
    u16 m_sys_n;
    P1_RiverSysEntry* m_sys;
    u16* m_rdep;
    u16* m_rsys;
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
        const P1_Gen_RiverSectorsRslt& sectors,
        const P1_Gen_RiverNetworkRslt& network);
    bool is_valid () const;
    const P1_Gen_RiverLinesRslt& result () const;
    void save_output (cstr path, const u8* terrain) const;

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
