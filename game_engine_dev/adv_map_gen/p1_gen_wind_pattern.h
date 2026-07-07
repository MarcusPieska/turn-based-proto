//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_WIND_PATTERN_H
#define P1_GEN_WIND_PATTERN_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_WindPattern -
//================================================================================================================================
//
//  Builds u8 wind direction and strength overlays. Latitude bands supply base flow; optional terrain steers around water and
//  mountains. Per-tile terrain tweaks and smoothing refine the field.
// 
//================================================================================================================================

struct P1_Gen_WindPatternPrm {
    u8 m_smooth_n; // full-map vector smooth passes
};

static inline P1_Gen_WindPatternPrm p1_gen_wind_pattern_prm_def () {
    P1_Gen_WindPatternPrm p;
    p.m_smooth_n = 2;
    return p;
} 

struct P1_Gen_WindPatternRslt {
    u16 m_w;
    u16 m_h;
    MapArrayOverlay m_dir;
    MapArrayOverlay m_str;
};

class P1_Gen_WindPattern {
public:
    explicit P1_Gen_WindPattern (const P1_RunPrm& prm, const P1_Gen_WindPatternPrm& sp = p1_gen_wind_pattern_prm_def ());

    bool generate ();
    bool generate (const u8* terrain, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_WindPatternRslt& result () const;

private:
    P1_Gen_WindPattern (const P1_Gen_WindPattern& other) = delete;
    P1_Gen_WindPattern (P1_Gen_WindPattern&& other) = delete;

    bool gen_core (const u8* terrain, u16 w, u16 h, bool use_ter);

    P1_RunPrm m_prm;
    P1_Gen_WindPatternPrm m_sp;
    bool m_valid_generation;
    P1_Gen_WindPatternRslt m_rslt;
};

#endif // P1_GEN_WIND_PATTERN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
