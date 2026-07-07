//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_LOESS_BOOST_H
#define P1_GEN_LOESS_BOOST_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_LoessBoost -
//================================================================================================================================
// 
//  Desert climate tiles emit dust; wind dir/str advect it; concentration accumulates on all tiles for now.
//
//================================================================================================================================

struct P1_Gen_LoessBoostPrm {
    u16 m_chunk_sz; // Coarse advection cell size in tiles; upsample smooths
    u8 m_step_n; // Advection steps
    u8 m_hop_max; // Max tiles jumped per step at full wind
    f32 m_emit; // Desert source per step
    f32 m_move; // Base mobile fraction per step
    f32 m_gamma; // Display gamma; lower lifts faint plumes
    f32 m_peak; // Peak scale for normalize; lower boosts contrast
    u8 m_smooth_n; // Fine overlay 3x3 blur passes after upsample
};

static inline P1_Gen_LoessBoostPrm p1_gen_loess_boost_prm_def () {
    P1_Gen_LoessBoostPrm p;
    p.m_chunk_sz = 3;
    p.m_step_n = 6;
    p.m_hop_max = 10;
    p.m_emit = 0.028f;
    p.m_move = 0.62f;
    p.m_gamma = 0.45f;
    p.m_peak = 0.32f;
    p.m_smooth_n = 2;
    return p;
}

//================================================================================================================================
//=> - P1_Gen_LoessBoostRslt -
//================================================================================================================================

struct P1_Gen_LoessBoostRslt {
    u16 m_w;
    u16 m_h;
    MapArrayOverlay m_ov;
};


class P1_Gen_LoessBoost {
public:
    explicit P1_Gen_LoessBoost (const P1_RunPrm& prm, const P1_Gen_LoessBoostPrm& sp = p1_gen_loess_boost_prm_def ());

    bool generate (const u8* climate, const u8* wind_dir, const u8* wind_str, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_LoessBoostRslt& result () const;

private:
    P1_Gen_LoessBoost (const P1_Gen_LoessBoost& other) = delete;
    P1_Gen_LoessBoost (P1_Gen_LoessBoost&& other) = delete;

    P1_RunPrm m_prm;
    P1_Gen_LoessBoostPrm m_sp;
    bool m_valid_generation;
    P1_Gen_LoessBoostRslt m_rslt; 
};

#endif // P1_GEN_LOESS_BOOST_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
