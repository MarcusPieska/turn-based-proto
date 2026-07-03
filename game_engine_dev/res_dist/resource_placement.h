//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCE_PLACEMENT_H
#define RESOURCE_PLACEMENT_H

#include "game_primitives.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//=> - ResPlcMapCtx -
//================================================================================================================================

typedef struct ResPlcMapCtx {
    u16 m_w;
    u16 m_h;
    const u8* m_terrain;
    const u8* m_climate;
    const u8* m_river;
    const u8* m_overlay;
} ResPlcMapCtx;

//================================================================================================================================
//=> - ResPlcVizPrm -
//================================================================================================================================

typedef struct ResPlcVizPrm {
    u8 m_dot_rad;
} ResPlcVizPrm;

static inline ResPlcVizPrm res_plc_viz_prm_def () {
    ResPlcVizPrm p;
    p.m_dot_rad = 2;
    return p;
}

static inline ResPlcVizPrm res_plc_viz_prm_small () {
    ResPlcVizPrm p;
    p.m_dot_rad = 0;
    return p;
}

//================================================================================================================================
//=> - ResPlcMatch -
//================================================================================================================================

class ResPlcMatch {
public:
    static bool tile_ok (
        const ResPlcMapCtx& ctx,
        u32 idx,
        const ResQuad& q);
    static bool entry_ok (
        const ResPlcMapCtx& ctx,
        u32 idx,
        const RuntimeStatics& s,
        u16 res_i,
        u8 quad_idx);
    static u32 mark_all_rules (
        const ResPlcMapCtx& ctx,
        const RuntimeStatics& s,
        u16 res_i,
        u8* out,
        u32 out_n);
};

//================================================================================================================================
//=> - ResPlcSelect -
//================================================================================================================================

class ResPlcSelect {
public:
    static bool run (
        const ResPlcMapCtx& ctx,
        const RuntimeStatics& s,
        u16 res_i,
        u32 base_n,
        u32 seed,
        u8* out,
        u32 out_n,
        u32* placed_n,
        double* sec_out);
};

//================================================================================================================================
//=> - ResPlcOverlay -
//================================================================================================================================

class ResPlcOverlay {
public:
    static u8* build_stub (
        u16 w,
        u16 h,
        const u8* terrain,
        const u8* climate,
        const u8* river);
};

//================================================================================================================================
//=> - ResPlcViz -
//================================================================================================================================

class ResPlcViz {
public:
    static void rule_rgb (u8 rule_idx, u8* r, u8* g, u8* b);
    static bool save_pair_img (
        cstr path,
        const ResPlcMapCtx& ctx,
        const u8* marks,
        u32 mark_n,
        const ResPlcVizPrm& prm);
    static bool save_pair_ov_img (
        cstr path,
        const ResPlcMapCtx& ctx,
        const u8* poss_marks,
        const u8* act_marks,
        u32 mark_n,
        const ResPlcVizPrm& prm);
    static bool make_out_path (
        u32 seed,
        cstr fname,
        char* out,
        u32 cap);
};

#endif // RESOURCE_PLACEMENT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
