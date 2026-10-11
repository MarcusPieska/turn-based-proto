//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_COASTAL_LAND_OVERLAY_H
#define P1_GEN_COASTAL_LAND_OVERLAY_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"
#include "p1_proc_distributed_coastal_points.h"

//================================================================================================================================
//=> - P1_Gen_CoastalLandOverlay -
//================================================================================================================================
//
//  For each trip.m_cen: flood coastal fringe, keep min(25%,w) seeds, flood inland; stall ends
//  m_a/m_b are extremes of that local inland fill. Temp marks use whiteboard sheets.
//
//================================================================================================================================

struct P1_Gen_CoastalLandOverlayPrm {
    u8 m_frac; // Percent of full coastal fringe to keep as inland seeds
};

static inline P1_Gen_CoastalLandOverlayPrm p1_gen_coastal_land_overlay_prm_def () {
    P1_Gen_CoastalLandOverlayPrm p;
    p.m_frac = 25u;
    return p;
}

struct P1_Gen_CoastalLandOverlayRslt {
    u16 m_w; // Map width
    u16 m_h; // Map height
    u8 m_stop; // Inland budget (w/100, min 1)
    u16 m_mass; // Mass of first successful point
    u32 m_pt_n; // Points successfully applied
    u32 m_coast_n; // Union coastal fringe tile count
    u32 m_seed_n; // Union coastal seed tile count
    u32 m_fill_n; // Union inland flood tile count
    MapArrayOverlay m_coast; // Union coastal fringe mark
    MapArrayOverlay m_seeds; // Union coastal seed mark
    MapArrayOverlay m_ov; // Union inland flood (max remaining)
};

class P1_Gen_CoastalLandOverlay {
public:
    explicit P1_Gen_CoastalLandOverlay (
        const P1_RunPrm& prm,
        const P1_Gen_CoastalLandOverlayPrm& sp = p1_gen_coastal_land_overlay_prm_def ());

    bool generate (const u8* terrain, u16 w, u16 h, P1_CoastPtTrip* trips, u32 trips_n);
    bool is_valid () const;
    const P1_Gen_CoastalLandOverlayRslt& result () const;

private:
    P1_Gen_CoastalLandOverlay (const P1_Gen_CoastalLandOverlay& other) = delete;
    P1_Gen_CoastalLandOverlay (P1_Gen_CoastalLandOverlay&& other) = delete;

    P1_RunPrm m_prm; // Run seed/size
    P1_Gen_CoastalLandOverlayPrm m_sp; // Frac and related knobs
    bool m_ok; // True after successful generate
    P1_Gen_CoastalLandOverlayRslt m_rslt; // Owned overlays and counts
};

#endif // P1_GEN_COASTAL_LAND_OVERLAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
