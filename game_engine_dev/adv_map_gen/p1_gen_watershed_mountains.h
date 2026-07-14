//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_WATERSHED_MOUNTAINS_H
#define P1_GEN_WATERSHED_MOUNTAINS_H

#include "game_primitives.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_map_size.h"

class Whiteboard_2B;
class Whiteboard_8B;

//================================================================================================================================
//=> - P1 watershed mountain defaults -
//================================================================================================================================

#define P1_WSHED_MTN_HILLS_PERC_DEF 90u
#define P1_WSHED_MTN_TILE_CAP_PERC_DEF 30u
#define P1_WSHED_MTN_MIN_DIST_PERC_DEF 5u
#define P1_WSHED_MTN_NOISE_THRESH_DEF 180u

//================================================================================================================================
//=> - P1_Gen_WatershedMountainsPrm -
//================================================================================================================================

struct P1_Gen_WatershedMountainsPrm {
    u8 m_hills_perc;
    u8 m_tile_cap_perc;
    u8 m_min_dist_perc;
    u8 m_noise_thresh;
};

static inline P1_Gen_WatershedMountainsPrm p1_gen_watershed_mountains_prm_def () {
    P1_Gen_WatershedMountainsPrm p;
    p.m_hills_perc = static_cast<u8>(P1_WSHED_MTN_HILLS_PERC_DEF);
    p.m_tile_cap_perc = static_cast<u8>(P1_WSHED_MTN_TILE_CAP_PERC_DEF);
    p.m_min_dist_perc = static_cast<u8>(P1_WSHED_MTN_MIN_DIST_PERC_DEF);
    p.m_noise_thresh = static_cast<u8>(P1_WSHED_MTN_NOISE_THRESH_DEF);
    return p;
}

//================================================================================================================================
//=> - P1_WatershedBorderSeg -
//================================================================================================================================

struct P1_WatershedBorderSeg {
    u16 m_basin_a;
    u16 m_basin_b;
    u16 m_mouth_ax;
    u16 m_mouth_ay;
    u16 m_mouth_bx;
    u16 m_mouth_by;
    u32 m_mouth_dist;
    u16 m_ov_idx;
    u32 m_tile_n;
    u16 m_a_plains;
    u16 m_a_hills;
    u16 m_b_plains;
    u16 m_b_hills;
};

//================================================================================================================================
//=> - P1_Gen_WatershedMountainsRslt -
//================================================================================================================================

struct P1_Gen_WatershedMountainsRslt {
    u16 m_w;
    u16 m_h;
    u16 m_seg_n;
    u16 m_pick_n;
    u32 m_border_tile_n;
    u32 m_pick_tile_n;
    P1_WatershedBorderSeg* m_segs;
    u16* m_ov;
};

//================================================================================================================================
//=> - P1_Gen_WatershedMountains -
//================================================================================================================================

class P1_Gen_WatershedMountains {
public:
    explicit P1_Gen_WatershedMountains (
        const P1_RunPrm& prm,
        const P1_Gen_WatershedMountainsPrm& sp = p1_gen_watershed_mountains_prm_def ());
    ~P1_Gen_WatershedMountains ();

    bool generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const P1_Gen_RiverNetworkRslt& network,
        const P1_Gen_RiverPtsRslt& pts,
        const u8* noise = nullptr);
    bool is_valid () const;
    const P1_Gen_WatershedMountainsRslt& result () const;
    void save_output (cstr path, const P1_Gen_RiverNetworkRslt& network, const u8* terrain) const;

private:
    P1_Gen_WatershedMountains (const P1_Gen_WatershedMountains& other) = delete;
    P1_Gen_WatershedMountains (P1_Gen_WatershedMountains&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    P1_Gen_WatershedMountainsPrm m_sp;
    bool m_valid_generation;
    P1_Gen_WatershedMountainsRslt m_rslt;
    Whiteboard_2B* m_wb_ov;
    Whiteboard_8B* m_wb_segs;
};

#endif // P1_GEN_WATERSHED_MOUNTAINS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
