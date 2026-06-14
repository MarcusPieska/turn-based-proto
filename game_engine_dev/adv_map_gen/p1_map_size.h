//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_MAP_SIZE_H
#define P1_MAP_SIZE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - P1 map size -
//================================================================================================================================

static const u16 P1_MAP_W_DEF = 1000;
static const u16 P1_MAP_H_DEF = 1000;

struct P1_MapSize {
    u16 m_w;
    u16 m_h;
};

struct P1_RunPrm {
    u32 m_seed;
    u16 m_w;
    u16 m_h;
};

static inline P1_MapSize p1_map_size_def () {
    P1_MapSize s;
    s.m_w = P1_MAP_W_DEF;
    s.m_h = P1_MAP_H_DEF;
    return s;
}

static inline P1_RunPrm p1_run_prm_def () {
    P1_RunPrm p;
    p.m_seed = 0;
    p.m_w = P1_MAP_W_DEF;
    p.m_h = P1_MAP_H_DEF;
    return p;
}

static inline bool p1_map_size_ok (u16 w, u16 h) {
    return w > 0 && h > 0;
}

static inline bool p1_run_prm_ok (const P1_RunPrm& prm) {
    return p1_map_size_ok(prm.m_w, prm.m_h);
}

#endif // P1_MAP_SIZE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
