//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_TESTER_EARLY_CHAIN_H
#define P1_TESTER_EARLY_CHAIN_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_dynamic_pts.h"
#include "p1_gen_river_prob.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "map_config.h"
#include "p1_map_size.h"
#include "p1_pipeline_steps.h"

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
    u16 m_pts_ocn_sec_n;
    WB_QueXY* m_pts_que;
    MapArrayDistance m_ocn;
    u16 m_ocean_n;
    u16 m_largest_idx;
    u32 m_wat_n;
    MapArrayDistance m_sec;
    u16 m_sector_n;
};

inline bool p1_early_has_sectors (const P1_EarlyChainRslt& ec) {
    return ec.m_sec.data() != nullptr && ec.m_w == ec.m_sec.width() && ec.m_h == ec.m_sec.height() && ec.m_sector_n > 0u;
}

inline P1_Gen_RiverSectorsRslt p1_early_sectors_rslt (const P1_EarlyChainRslt& ec) {
    P1_Gen_RiverSectorsRslt r;
    r.m_w = ec.m_w;
    r.m_h = ec.m_h;
    r.m_sector_n = ec.m_sector_n;
    r.m_ocn_sec_n = ec.m_pts_ocn_sec_n;
    r.m_ov = const_cast<u16*>(ec.m_sec.data());
    r.m_dist_ov = nullptr;
    return r;
}

inline bool p1_early_has_ocean (const P1_EarlyChainRslt& ec) {
    return ec.m_ocn.data() != nullptr && ec.m_w == ec.m_ocn.width() && ec.m_h == ec.m_ocn.height();
}

inline P1_OceanIndexRef p1_early_ocean_ref (const P1_EarlyChainRslt& ec) {
    P1_OceanIndexRef r;
    r.m_w = ec.m_w;
    r.m_h = ec.m_h;
    r.m_ocean_n = ec.m_ocean_n;
    r.m_largest_idx = ec.m_largest_idx;
    r.m_wat_n = ec.m_wat_n;
    r.m_ov = ec.m_ocn.data();
    return r;
}

void p1_free_early_chain (P1_EarlyChainRslt* r);
bool p1_build_early_chain (const P1_RunPrm& prm, const MapConfig& cfg, u16 last_step, P1_EarlyChainRslt* out, double* sec);

#endif // P1_TESTER_EARLY_CHAIN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
