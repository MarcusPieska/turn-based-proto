//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_SHAPED_OUTLINE_H
#define P1_GEN_SHAPED_OUTLINE_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_ShapedOutlinePrm -
//================================================================================================================================

struct P1_Gen_ShapedOutlinePrm {
    f32 m_radial_near;
    f32 m_radial_far;
};

static inline P1_Gen_ShapedOutlinePrm p1_gen_shaped_outline_prm_def () {
    P1_Gen_ShapedOutlinePrm p;
    p.m_radial_near = 0.0f;
    p.m_radial_far = 0.85f;
    return p;
}

//================================================================================================================================
//=> - P1_Gen_ShapedOutline -
//================================================================================================================================

class P1_Gen_ShapedOutline {
public:
    explicit P1_Gen_ShapedOutline (const P1_RunPrm& prm);

    bool generate_layer (
        u8* terrain,
        u16 w,
        u16 h,
        const u8* ol_ov,
        const u16* land_depth,
        f32 radial);
    bool merge_layers (
        u8* terrain,
        u16 w,
        u16 h,
        const u8* ol_ov,
        const u16* land_depth,
        const u8* near_ter,
        const u8* far_ter);
    bool apply (
        const P1_Gen_ShapedOutlinePrm& sp,
        u8* terrain,
        u16 w,
        u16 h,
        const u8* ol_ov,
        const u16* land_depth);
    bool is_valid () const;

private:
    P1_Gen_ShapedOutline (const P1_Gen_ShapedOutline& other) = delete;
    P1_Gen_ShapedOutline (P1_Gen_ShapedOutline&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_generation;
};

bool p1_apply_shaped_outline (
    const P1_RunPrm& prm,
    const P1_Gen_ShapedOutlinePrm& sp,
    u8* terrain,
    u16 w,
    u16 h,
    const u8* ol_ov,
    const u16* land_depth);

#endif // P1_GEN_SHAPED_OUTLINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
