//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_TESTER_EARLY_CHAIN_H
#define P1_TESTER_EARLY_CHAIN_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_gen_river_pts.h"
#include "p1_map_config.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_EarlyChainRslt -
//================================================================================================================================

struct P1_EarlyChainRslt {
    u16 m_w;
    u16 m_h;
    MapArrayOverlay m_ov;
    f32* m_perlin_f32;
    u32 m_perlin_n;
    MapArrayOverlay m_perlin_gray;
    u8* m_fill_ter;
    MapArrayOverlay m_ld_wl;
    MapArrayDistance m_ld_dist;
    u8* m_ter;
    u32 m_pts_n;
    WB_QueXY* m_pts_que;
};

void p1_free_early_chain (P1_EarlyChainRslt* r);
bool p1_build_early_chain (const P1_RunPrm& prm, const P1_MapConfig& cfg, u16 last_step, P1_EarlyChainRslt* out, double* sec);

#endif // P1_TESTER_EARLY_CHAIN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
