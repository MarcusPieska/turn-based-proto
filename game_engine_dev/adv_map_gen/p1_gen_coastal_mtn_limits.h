//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_COASTAL_MTN_LIMITS_H
#define P1_GEN_COASTAL_MTN_LIMITS_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_gen_river_sect_adj.h"
#include "p1_gen_river_sectors.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 coastal mtn limit overlay values -
//================================================================================================================================

#define P1_COASTAL_MTN_OV_BLK 128u
#define P1_COASTAL_MTN_OV_SEL 255u

//================================================================================================================================
//=> - P1_Gen_CoastalMtnLimitsRslt -
//================================================================================================================================

struct P1_Gen_CoastalMtnLimitsRslt {
    u16 m_w;
    u16 m_h;
    u16 m_sector_n;
    u16 m_max_sec_dist;
    u32 m_limit_n;
    u32 m_sel_n;
    MapArrayOverlay m_limit_ov;
};

//================================================================================================================================
//=> - P1_Gen_CoastalMtnLimits -
//================================================================================================================================
//
//  Sector coastal-distance flood; m_limit_ov is 0 / 128 blocked / 255 selected.
//
//================================================================================================================================

class P1_Gen_CoastalMtnLimits {
public:
    explicit P1_Gen_CoastalMtnLimits (const P1_RunPrm& prm);

    bool generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const P1_Gen_RiverSectorsRslt& sectors,
        const P1_Gen_RiverSectAdjRslt& sect_adj);
    
    bool is_valid () const;
    const P1_Gen_CoastalMtnLimitsRslt& result () const;
    void save_output (cstr path, const u8* terrain) const;

private:
    P1_Gen_CoastalMtnLimits (const P1_Gen_CoastalMtnLimits& other) = delete;
    P1_Gen_CoastalMtnLimits (P1_Gen_CoastalMtnLimits&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_CoastalMtnLimitsRslt m_rslt;
};

#endif // P1_GEN_COASTAL_MTN_LIMITS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
