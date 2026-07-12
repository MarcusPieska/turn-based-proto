//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_FOREST_OVERLAY_H
#define P1_GEN_FOREST_OVERLAY_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_ForestOverlayRslt -
//================================================================================================================================

struct P1_Gen_ForestOverlayRslt {
    u16 m_w;
    u16 m_h;
    u16 m_max_grass_dist;
    u16 m_max_desert_dist;
    u32 m_forest_n;
    u32 m_forest_grass_n;
    u32 m_forest_plains_n;
    u32 m_plains_fill_n;
    u16 m_plains_fill_dist;
    u32 m_plains_clim_n;
    u32 m_plains_roll_n;
    u32 m_plains_deep_n;
    u32 m_plains_band_tile_n[6];
    u32 m_plains_roll_yes[6];
    u32 m_plains_roll_no[6];
    u32 m_plains_rn_hist[100];
    MapArrayOverlay m_dist_ov;
    MapArrayOverlay m_dist_desert_ov;
    MapArrayOverlay m_perlin_mod_ov;
};

//================================================================================================================================
//=> - P1_Gen_ForestOverlay -
//================================================================================================================================
//
//  Phase 1: grass/black-soil BFS from plains; phase 2: plains climate BFS from desert.
//
//================================================================================================================================

class P1_Gen_ForestOverlay {
public:
    explicit P1_Gen_ForestOverlay (const P1_RunPrm& prm);

    bool generate (const u8* terrain, const u8* climate, u8* res_ov, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_ForestOverlayRslt& result () const;
    void save_dist_output (cstr path) const;
    void save_desert_dist_output (cstr path) const;
    void save_perlin_mod_output (cstr path) const;

private:
    P1_Gen_ForestOverlay (const P1_Gen_ForestOverlay& other) = delete;
    P1_Gen_ForestOverlay (P1_Gen_ForestOverlay&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_ForestOverlayRslt m_rslt;
};

#endif // P1_GEN_FOREST_OVERLAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
