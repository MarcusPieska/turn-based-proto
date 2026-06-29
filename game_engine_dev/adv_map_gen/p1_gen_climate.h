//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_CLIMATE_H
#define P1_GEN_CLIMATE_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_ClimateOverlayWts -
//================================================================================================================================

struct P1_Gen_ClimateOverlayWts {
    u8 m_w_dist_river;
    u8 m_w_open_dist_water;
    u8 m_w_latitude;
    u8 m_w_plain_dist_water;
};

//================================================================================================================================
//=> - P1_Gen_ClimateTypePct -
//================================================================================================================================

struct P1_Gen_ClimateTypePct {
    u8 m_pct_grassland;
    u8 m_pct_plains;
    u8 m_pct_desert;
};

//================================================================================================================================
//=> - P1_Gen_ClimatePrm -
//================================================================================================================================

struct P1_Gen_ClimatePrm {
    P1_Gen_ClimateOverlayWts m_wts;
    P1_Gen_ClimateTypePct m_pct;
};

static inline P1_Gen_ClimatePrm p1_gen_climate_prm_def () {
    P1_Gen_ClimatePrm p = {};
    p.m_wts.m_w_dist_river = 50;
    p.m_wts.m_w_open_dist_water = 10;
    p.m_wts.m_w_latitude = 90;
    p.m_wts.m_w_plain_dist_water = 10;
    p.m_pct.m_pct_grassland = 40;
    p.m_pct.m_pct_plains = 35;
    p.m_pct.m_pct_desert = 25;
    return p;
}

//================================================================================================================================
//=> - P1_Gen_ClimateRslt -
//================================================================================================================================

struct P1_Gen_ClimateRslt {
    u16 m_w;
    u16 m_h;
    MapArrayOverlay m_ov;
};

//================================================================================================================================
//=> - P1_Gen_Climate -
//================================================================================================================================

class P1_Gen_Climate {
public:
    explicit P1_Gen_Climate (
        const P1_RunPrm& prm,
        const P1_Gen_ClimatePrm& sp = p1_gen_climate_prm_def ());

    bool generate (const u8* terrain, u16 w, u16 h, const u8* river_ov);
    bool is_valid () const;
    const P1_Gen_ClimateRslt& result () const;

private:
    P1_Gen_Climate (const P1_Gen_Climate& other) = delete;
    P1_Gen_Climate (P1_Gen_Climate&& other) = delete;

    P1_RunPrm m_prm;
    P1_Gen_ClimatePrm m_sp;
    bool m_valid_generation;
    P1_Gen_ClimateRslt m_rslt;
};

#endif // P1_GEN_CLIMATE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
