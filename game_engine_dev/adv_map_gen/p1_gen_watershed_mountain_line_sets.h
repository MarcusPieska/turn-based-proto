//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_WATERSHED_MOUNTAIN_LINE_SETS_H
#define P1_GEN_WATERSHED_MOUNTAIN_LINE_SETS_H

#include "game_primitives.h"
#include "p1_gen_watershed_mountains.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 watershed mountain line set defaults -
//================================================================================================================================

#define P1_WSHED_MLS_TILE_CAP_PERC_DEF 30u
#define P1_WSHED_MLS_MIN_DIST_PERC_DEF 2u
#define P1_WSHED_MLS_MAX_PICK_N_DEF 60u

//================================================================================================================================
//=> - P1_Gen_WatershedMountainLineSetsPrm -
//================================================================================================================================

struct P1_Gen_WatershedMountainLineSetsPrm {
    u8 m_tile_cap_perc;
    u8 m_min_dist_perc;
    u16 m_max_pick_n;
};

static inline P1_Gen_WatershedMountainLineSetsPrm p1_gen_watershed_mountain_line_sets_prm_def () {
    P1_Gen_WatershedMountainLineSetsPrm p;
    p.m_tile_cap_perc = static_cast<u8>(P1_WSHED_MLS_TILE_CAP_PERC_DEF);
    p.m_min_dist_perc = static_cast<u8>(P1_WSHED_MLS_MIN_DIST_PERC_DEF);
    p.m_max_pick_n = static_cast<u16>(P1_WSHED_MLS_MAX_PICK_N_DEF);
    return p;
}

//================================================================================================================================
//=> - P1_Gen_WatershedMountainLineSetsRslt -
//================================================================================================================================

struct P1_Gen_WatershedMountainLineSetsRslt {
    u16 m_w;
    u16 m_h;
    u16 m_pick_n;
    u32 m_pick_tile_n;
    u16* m_pick_seg;
    u16* m_ov;
};

//================================================================================================================================
//=> - P1_Gen_WatershedMountainLineSets -
//================================================================================================================================

class P1_Gen_WatershedMountainLineSets {
public:
    explicit P1_Gen_WatershedMountainLineSets (
        const P1_RunPrm& prm,
        const P1_Gen_WatershedMountainLineSetsPrm& sp = p1_gen_watershed_mountain_line_sets_prm_def ());
    ~P1_Gen_WatershedMountainLineSets ();

    bool generate (const P1_Gen_WatershedMountainsRslt& borders);
    bool is_valid () const;
    const P1_Gen_WatershedMountainLineSetsRslt& result () const;
    void save_output (cstr path, const P1_Gen_RiverNetworkRslt& network, const u8* terrain) const;

private:
    P1_Gen_WatershedMountainLineSets (const P1_Gen_WatershedMountainLineSets& other) = delete;
    P1_Gen_WatershedMountainLineSets (P1_Gen_WatershedMountainLineSets&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    P1_Gen_WatershedMountainLineSetsPrm m_sp;
    bool m_valid_generation;
    P1_Gen_WatershedMountainLineSetsRslt m_rslt;
};

#endif // P1_GEN_WATERSHED_MOUNTAIN_LINE_SETS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
