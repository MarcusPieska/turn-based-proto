//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_SHAPED_OUTLINE_H
#define P1_GEN_SHAPED_OUTLINE_H

#include "game_primitives.h"
#include "map_config.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_ShapedShelfFracs -
//================================================================================================================================

struct P1_ShapedShelfFracs {
    u8 m_ocean;
    u8 m_sea;
    u8 m_coastal;
    u8 m_plains;
};

//================================================================================================================================
//=> - P1_Gen_ShapedOutlinePrm -
//================================================================================================================================

struct P1_Gen_ShapedOutlinePrm {
    f32 m_radial_near;
    f32 m_radial_far;
    P1_ShapedShelfFracs m_shelf_near;
    P1_ShapedShelfFracs m_shelf_far;
};

static inline P1_ShapedShelfFracs p1_shaped_shelf_fracs_def_near () {
    P1_ShapedShelfFracs f;
    f.m_ocean = 18u;
    f.m_sea = 12u;
    f.m_coastal = 8u;
    f.m_plains = 62u;
    return f;
}

static inline P1_ShapedShelfFracs p1_shaped_shelf_fracs_def_far () {
    P1_ShapedShelfFracs f;
    f.m_ocean = 10u;
    f.m_sea = 20u;
    f.m_coastal = 10u;
    f.m_plains = 60u;
    return f;
}

static inline P1_Gen_ShapedOutlinePrm p1_gen_shaped_outline_prm_def () {
    P1_Gen_ShapedOutlinePrm p;
    p.m_radial_near = 0.0f;
    p.m_radial_far = 0.9f;
    p.m_shelf_near = p1_shaped_shelf_fracs_def_near();
    p.m_shelf_far = p1_shaped_shelf_fracs_def_far();
    return p;
}

static inline P1_Gen_ShapedOutlinePrm p1_gen_shaped_outline_prm_from_cfg (const MapConfig& cfg) {
    P1_Gen_ShapedOutlinePrm p = p1_gen_shaped_outline_prm_def();
    p.m_radial_near = cfg.m_shaped_radial_near;
    p.m_radial_far = cfg.m_shaped_radial_far;
    p.m_shelf_near.m_ocean = cfg.m_shaped_near_ocean;
    p.m_shelf_near.m_sea = cfg.m_shaped_near_sea;
    p.m_shelf_near.m_coastal = cfg.m_shaped_near_coastal;
    p.m_shelf_far.m_ocean = cfg.m_shaped_far_ocean;
    p.m_shelf_far.m_sea = cfg.m_shaped_far_sea;
    p.m_shelf_far.m_coastal = cfg.m_shaped_far_coastal;
    return p;
}

//================================================================================================================================
//=> - P1_Gen_ShapedOutline -
//================================================================================================================================
//
//  Near/far shelf terrain shaping from shared perlin field and land-depth radial weighting.
//
//================================================================================================================================

class P1_Gen_ShapedOutline {
public:
    explicit P1_Gen_ShapedOutline (const P1_RunPrm& prm);
    ~P1_Gen_ShapedOutline ();

    void bind_perlin_field (const f32* field, u16 w, u16 h);
    bool generate_layer (
        u8* terrain,
        u16 w,
        u16 h,
        const u8* ol_ov,
        const u16* land_depth,
        f32 radial,
        const P1_ShapedShelfFracs& shelf);
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

    void free_perlin ();

    P1_RunPrm m_prm;
    bool m_valid_generation;
    const f32* m_perlin_bind;
    u32 m_perlin_n;
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
