//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_COASTAL_PROTRUSION_H
#define P1_GEN_COASTAL_PROTRUSION_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"
#include "p1_proc_distributed_coastal_points.h"

//================================================================================================================================
//=> - P1_Gen_CoastalProtrusion -
//================================================================================================================================
//
//  For each trip: coastal-land from m_cen (fills stalls), then ROT20 pivots on a random stall
//  extreme; PULL10 pivots on m_cen after seaward pull. Angle may be reduced from a coast AABB so
//  the rotated bbox stays inside a 1% map-size margin. Post-clip keep is fine∪rough Perlin tops.
//
//================================================================================================================================

enum P1_CoastRotMode : u8 {
    P1_COAST_ROT_FWD = 0u, // Sparse forward map (leaves 1px holes)
    P1_COAST_ROT_INV = 1u // Inverse sample inside rotated source bbox (plugs holes)
};

enum P1_CoastPlugMode : u8 {
    P1_COAST_PLUG_NONE = 0u, // No post-rotation plug
    P1_COAST_PLUG_FLOOD1 = 1u // One 4-neighbor dilate to plug 1px gaps
};

enum P1_CoastSwingMode : u8 {
    P1_COAST_SWING_ROT20 = 0u, // Rotate +/- m_deg about the coastal point
    P1_COAST_SWING_PULL10 = 1u, // Pull seaward by m_pull_mul*(w/100), then rotate +/- m_pull_deg
    P1_COAST_SWING_ALT = 2u // Alternate ROT20 / PULL10 per input point
};

struct P1_Gen_CoastalProtrusionPrm {
    i32 m_deg; // Absolute degrees for ROT20 swing
    i32 m_pull_deg; // Absolute degrees after seaward pull
    u8 m_pull_mul; // Pull distance = mul * (w/100)
    u8 m_keep_fine; // Top percent of fine Perlin kept
    u8 m_keep_rough; // Top percent of rough Perlin kept
    u8 m_rot_mode; // P1_CoastRotMode
    u8 m_plug_mode; // P1_CoastPlugMode
    u8 m_swing_mode; // P1_CoastSwingMode
};

static inline P1_Gen_CoastalProtrusionPrm p1_gen_coastal_protrusion_prm_def () {
    P1_Gen_CoastalProtrusionPrm p;
    p.m_deg = 20;
    p.m_pull_deg = 10;
    p.m_pull_mul = 5u;
    p.m_keep_fine = 30u;
    p.m_keep_rough = 10u;
    p.m_rot_mode = P1_COAST_ROT_INV;
    p.m_plug_mode = P1_COAST_PLUG_NONE;
    p.m_swing_mode = P1_COAST_SWING_ALT;
    return p;
}

struct P1_Gen_CoastalProtrusionRslt {
    u16 m_w; // Map width
    u16 m_h; // Map height
    i32 m_deg; // Last applied rotation (signed, CW)
    u16 m_px; // Last pivot x
    u16 m_py; // Last pivot y
    u32 m_pt_n; // Points with at least one successful swing
    u32 m_src_n; // Sum of per-point coastal tile counts
    u32 m_rot_n; // Union tiles after rotation
    u32 m_clip_n; // Union tiles kept over water
    u32 m_keep_n; // Union tiles kept after Perlin cull
    u8 m_rot_mode; // Applied pixel-rot mode
    u8 m_plug_mode; // Applied plug mode
    u8 m_swing_mode; // Applied swing mode
    f32 m_thr; // Last rough Perlin keep threshold
    MapArrayOverlay m_rot; // Union after rotation
    MapArrayOverlay m_clip; // Union after water clip
    MapArrayOverlay m_ov; // Union final protrusion
};

class P1_Gen_CoastalProtrusion {
public:
    explicit P1_Gen_CoastalProtrusion (
        const P1_RunPrm& prm,
        const P1_Gen_CoastalProtrusionPrm& sp = p1_gen_coastal_protrusion_prm_def ());

    bool generate (
        const u8* terrain,
        const f32* perlin,
        u16 w,
        u16 h,
        P1_CoastPtTrip* trips,
        u32 trips_n);
    bool is_valid () const;
    const P1_Gen_CoastalProtrusionRslt& result () const;

private:
    P1_Gen_CoastalProtrusion (const P1_Gen_CoastalProtrusion& other) = delete;
    P1_Gen_CoastalProtrusion (P1_Gen_CoastalProtrusion&& other) = delete;

    P1_RunPrm m_prm; // Run seed/size
    P1_Gen_CoastalProtrusionPrm m_sp; // Swing / rot knobs
    bool m_ok; // True after successful generate
    P1_Gen_CoastalProtrusionRslt m_rslt; // Owned overlays and metrics
};

#endif // P1_GEN_COASTAL_PROTRUSION_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
