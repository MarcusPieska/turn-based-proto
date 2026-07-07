//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_WIND_PATTERN_ADV_H
#define P1_GEN_WIND_PATTERN_ADV_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"
 
//================================================================================================================================
//=> - P1_Gen_WindPatternAdv -
//================================================================================================================================
//
//  Advanced wind overlays via lightweight chunked fluid dynamics. Terrain is summarized per chunk into a blocking alias, the flow
//  field is relaxed on the coarse grid, upsampled, then full-res terrain deflection and upwind lee decay are applied.
//
//================================================================================================================================

struct P1_Gen_WindPatternAdvPrm {
    u16 m_chunk_sz; // terrain summary and fluid grid cell size in tiles
    u8 m_iter_n; // fluid relaxation iterations on coarse grid
    u8 m_jacobi_n; // pressure projection Jacobi passes per iteration
};

static inline P1_Gen_WindPatternAdvPrm p1_gen_wind_pattern_adv_prm_def () {
    P1_Gen_WindPatternAdvPrm p;
    p.m_chunk_sz = 10;
    p.m_iter_n = 32;
    p.m_jacobi_n = 4;
    return p;
}

struct P1_Gen_WindPatternAdvRslt {
    u16 m_w;
    u16 m_h;
    MapArrayOverlay m_dir;
    MapArrayOverlay m_str;
};

class P1_Gen_WindPatternAdv {
public:
    explicit P1_Gen_WindPatternAdv (
        const P1_RunPrm& prm,
        const P1_Gen_WindPatternAdvPrm& sp = p1_gen_wind_pattern_adv_prm_def ());

    bool generate ();
    bool generate (const u8* terrain, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_WindPatternAdvRslt& result () const;

private:
    P1_Gen_WindPatternAdv (const P1_Gen_WindPatternAdv& other) = delete;
    P1_Gen_WindPatternAdv (P1_Gen_WindPatternAdv&& other) = delete;

    bool gen_core (const u8* terrain, u16 w, u16 h, bool use_ter);

    P1_RunPrm m_prm;
    P1_Gen_WindPatternAdvPrm m_sp;
    bool m_valid_generation;
    P1_Gen_WindPatternAdvRslt m_rslt;
};

#endif // P1_GEN_WIND_PATTERN_ADV_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
