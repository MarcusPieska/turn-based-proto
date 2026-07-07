//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RAIN_OROGRAPHIC_H
#define P1_GEN_RAIN_OROGRAPHIC_H

#include "game_primitives.h"
#include "generator_constants.h" 
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_RainOrographic -
//================================================================================================================================
//
//  Coast and mountain dual flood height; 8-neighbor slope; per-tile rain from wind and upslope.
//
//================================================================================================================================

struct P1_Gen_RainOrographicPrm {
    u8 m_smooth_n; // box-blur passes on gx, gy, smag
    f64 m_slp_full; // wind fraction vs max slope; rest vs actual; 0 is pre-split rain
    f32 m_gamma; // display gamma; lower lifts faint rain
    f32 m_peak; // peak scale for normalize; lower boosts contrast
    u8 m_pack_dbg; // 1 fills flood/height/slope debug overlays for testers
};

static inline P1_Gen_RainOrographicPrm p1_gen_rain_orographic_prm_def () {
    P1_Gen_RainOrographicPrm p;
    p.m_smooth_n = 2;
    p.m_slp_full = 0.0;
    p.m_gamma = 0.24f;
    p.m_peak = 0.10f;
    p.m_pack_dbg = 0;
    return p;
}

struct P1_Gen_RainOrographicRslt {
    u16 m_w;
    u16 m_h;
    MapArrayOverlay m_rain;
    MapArrayOverlay m_flood_coast;
    MapArrayOverlay m_flood_mtn;
    MapArrayOverlay m_height;
    MapArrayOverlay m_slope;
};

class P1_Gen_RainOrographic {
public:
    explicit P1_Gen_RainOrographic (
        const P1_RunPrm& prm,
        u16 rain_finish,
        u16 slope_finish,
        const P1_Gen_RainOrographicPrm& sp = p1_gen_rain_orographic_prm_def ());

    bool generate (const u8* terrain, const u8* wind_dir, const u8* wind_str, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_RainOrographicRslt& result () const;

private:
    P1_Gen_RainOrographic (const P1_Gen_RainOrographic& other) = delete;
    P1_Gen_RainOrographic (P1_Gen_RainOrographic&& other) = delete;

    P1_RunPrm m_prm;
    P1_Gen_RainOrographicPrm m_sp;
    u16 m_rain_finish;
    u16 m_slope_finish;
    bool m_valid_generation;
    P1_Gen_RainOrographicRslt m_rslt;
};

#endif // P1_GEN_RAIN_OROGRAPHIC_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
