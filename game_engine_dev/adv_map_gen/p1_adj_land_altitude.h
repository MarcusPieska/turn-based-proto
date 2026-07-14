//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_LAND_ALTITUDE_H
#define P1_ADJ_LAND_ALTITUDE_H

#include "game_primitives.h"
#include "p1_map_config.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 land altitude defaults -
//================================================================================================================================

#define P1_LAND_ALT_W_NOISE_DEF 1.0f
#define P1_LAND_ALT_W_NEAR_DEF 1.25f
#define P1_LAND_ALT_W_RIV_DEF 0.35f

//================================================================================================================================
//=> - P1_Adj_LandAltitudePrm -
//================================================================================================================================

struct P1_Adj_LandAltitudePrm {
    f32 m_w_noise;
    f32 m_w_near;
    f32 m_w_riv;
    f32 m_lim_hills;
    f32 m_lim_mtn;
};

static inline P1_Adj_LandAltitudePrm p1_adj_land_altitude_prm_from_cfg (const P1_MapConfig& cfg) {
    P1_Adj_LandAltitudePrm p;
    p.m_w_noise = P1_LAND_ALT_W_NOISE_DEF;
    p.m_w_near = P1_LAND_ALT_W_NEAR_DEF;
    p.m_w_riv = P1_LAND_ALT_W_RIV_DEF;
    p.m_lim_hills = cfg.m_land_alt_lim_hills;
    p.m_lim_mtn = cfg.m_land_alt_lim_mtn;
    return p;
}

static inline P1_Adj_LandAltitudePrm p1_adj_land_altitude_prm_def () {
    return p1_adj_land_altitude_prm_from_cfg(p1_map_config_def());
}

//================================================================================================================================
//=> - P1_Adj_LandAltitude -
//================================================================================================================================

class P1_Adj_LandAltitude {
public:
    explicit P1_Adj_LandAltitude (
        const P1_RunPrm& prm,
        const P1_Adj_LandAltitudePrm& sp = p1_adj_land_altitude_prm_def ());

    bool adjust (
        u8* terrain,
        u16 w,
        u16 h,
        const u8* noise,
        const u8* dist_riv,
        const u8* near_mtn);
    bool joint_ov (
        const u8* terrain,
        u16 w,
        u16 h,
        const u8* noise,
        const u8* dist_riv,
        const u8* near_mtn,
        u8* joint) const;
    bool is_valid () const;

private:
    P1_Adj_LandAltitude (const P1_Adj_LandAltitude& other) = delete;
    P1_Adj_LandAltitude (P1_Adj_LandAltitude&& other) = delete;

    P1_RunPrm m_prm;
    P1_Adj_LandAltitudePrm m_sp;
    bool m_valid_adjust;
};

#endif // P1_ADJ_LAND_ALTITUDE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
