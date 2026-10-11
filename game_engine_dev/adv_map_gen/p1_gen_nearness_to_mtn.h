//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_NEARNESS_TO_MTN_H
#define P1_GEN_NEARNESS_TO_MTN_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h" 

//================================================================================================================================
//=> - P1_Gen_NearnessToMtn -
//================================================================================================================================

struct P1_Gen_NearnessToMtnPrm {
    u8 m_stop;
};

static inline u8 p1_gen_nearness_to_mtn_stop_from_w (u16 w) {
    const u16 s = static_cast<u16>(w / 100u);
    return (s == 0u) ? 1u : static_cast<u8>(s > 255u ? 255u : s);
}

static inline P1_Gen_NearnessToMtnPrm p1_gen_nearness_to_mtn_prm_def () {
    P1_Gen_NearnessToMtnPrm p;
    p.m_stop = 0;
    return p;
}

struct P1_Gen_NearnessToMtnRslt {
    u16 m_w;
    u16 m_h;
    u8 m_stop;
    u32 m_seed_n;
    u32 m_fill_n;
    MapArrayOverlay m_ov;
};

class P1_Gen_NearnessToMtn {
public:
    explicit P1_Gen_NearnessToMtn (
        const P1_RunPrm& prm,
        const P1_Gen_NearnessToMtnPrm& sp = p1_gen_nearness_to_mtn_prm_def ());

    bool generate (const u8* terrain, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_NearnessToMtnRslt& result () const;
    void save_output (cstr path) const;

private:
    P1_Gen_NearnessToMtn (const P1_Gen_NearnessToMtn& other) = delete;
    P1_Gen_NearnessToMtn (P1_Gen_NearnessToMtn&& other) = delete;

    P1_RunPrm m_prm;
    P1_Gen_NearnessToMtnPrm m_sp;
    bool m_valid_generation;
    P1_Gen_NearnessToMtnRslt m_rslt;
};

#endif // P1_GEN_NEARNESS_TO_MTN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
