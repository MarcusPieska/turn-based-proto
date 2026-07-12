//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_NEARNESS_TO_WATERSHED_MTN_H
#define P1_GEN_NEARNESS_TO_WATERSHED_MTN_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "p1_gen_watershed_mountain_line_sets.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 nearness defaults -
//================================================================================================================================

#define P1_WSHED_MTN_NEAR_START_DEF 255u
#define P1_WSHED_MTN_NEAR_FALL_STEP_DEF 3u

//================================================================================================================================
//=> - P1_Gen_NearnessToWatershedMtnPrm -
//================================================================================================================================

struct P1_Gen_NearnessToWatershedMtnPrm {
    u8 m_start_val;
    u8 m_fall_step;
};

static inline P1_Gen_NearnessToWatershedMtnPrm p1_gen_nearness_to_watershed_mtn_prm_def () {
    P1_Gen_NearnessToWatershedMtnPrm p;
    p.m_start_val = static_cast<u8>(P1_WSHED_MTN_NEAR_START_DEF);
    p.m_fall_step = static_cast<u8>(P1_WSHED_MTN_NEAR_FALL_STEP_DEF);
    return p;
}

//================================================================================================================================
//=> - P1_Gen_NearnessToWatershedMtnRslt -
//================================================================================================================================

struct P1_Gen_NearnessToWatershedMtnRslt {
    u16 m_w;
    u16 m_h;
    u16 m_max_dist;
    MapArrayOverlay m_ov;
};

//================================================================================================================================
//=> - P1_Gen_NearnessToWatershedMtn -
//================================================================================================================================

class P1_Gen_NearnessToWatershedMtn {
public:
    explicit P1_Gen_NearnessToWatershedMtn (
        const P1_RunPrm& prm,
        const P1_Gen_NearnessToWatershedMtnPrm& sp = p1_gen_nearness_to_watershed_mtn_prm_def ());

    bool generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const P1_Gen_WatershedMountainLineSetsRslt& line_sets,
        const P1_Gen_CoastalMtnLimitsRslt& coast_lim);
    bool is_valid () const;
    const P1_Gen_NearnessToWatershedMtnRslt& result () const;
    void save_output (cstr path) const;

private:
    P1_Gen_NearnessToWatershedMtn (const P1_Gen_NearnessToWatershedMtn& other) = delete;
    P1_Gen_NearnessToWatershedMtn (P1_Gen_NearnessToWatershedMtn&& other) = delete;

    P1_RunPrm m_prm;
    P1_Gen_NearnessToWatershedMtnPrm m_sp;
    bool m_valid_generation;
    P1_Gen_NearnessToWatershedMtnRslt m_rslt;
};

#endif // P1_GEN_NEARNESS_TO_WATERSHED_MTN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
