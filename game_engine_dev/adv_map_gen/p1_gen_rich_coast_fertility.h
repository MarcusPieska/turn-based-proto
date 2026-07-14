//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RICH_COAST_FERTILITY_H
#define P1_GEN_RICH_COAST_FERTILITY_H

#include "game_primitives.h"
#include "p1_map_size.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - P1 rich coast fertility defaults -
//================================================================================================================================

#define P1_RICH_COAST_BRUSH_RAD_DEF 14u
#define P1_RICH_COAST_BRUSH_PEAK_DEF 48u
#define P1_RICH_COAST_STAMP_LIM_DEF 5u
#define P1_RICH_COAST_BRUSH_MAX_RAD 32u
#define P1_RICH_COAST_BLK_SZ 5u
#define P1_RICH_COAST_FAC_N 25u
#define P1_RICH_COAST_BRUSH_CAP ((P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u) * (P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u))

//================================================================================================================================
//=> - P1_Gen_RichCoastFertilityPrm -
//================================================================================================================================

struct P1_Gen_RichCoastFertilityPrm {
    u16 m_brush_rad;
    u16 m_brush_peak;
    u16 m_stamp_lim;
};

static inline P1_Gen_RichCoastFertilityPrm p1_gen_rich_coast_fertility_prm_def () {
    P1_Gen_RichCoastFertilityPrm p;
    p.m_brush_rad = static_cast<u16>(P1_RICH_COAST_BRUSH_RAD_DEF);
    p.m_brush_peak = static_cast<u16>(P1_RICH_COAST_BRUSH_PEAK_DEF);
    p.m_stamp_lim = static_cast<u16>(P1_RICH_COAST_STAMP_LIM_DEF);
    return p; 
}

//================================================================================================================================
//=> - P1_Gen_RichCoastFertilityRslt -
//================================================================================================================================

struct P1_Gen_RichCoastFertilityRslt {
    u16 m_w;
    u16 m_h;
    u16 m_peak;
    const u16* m_ov;
};

//================================================================================================================================
//=> - P1_Gen_RichCoastFertility -
//================================================================================================================================
//
//  Circular brush stamp from shallow-water tiles; additive u16 fertility on nearby land.
//
//================================================================================================================================

class P1_Gen_RichCoastFertility {
public:
    explicit P1_Gen_RichCoastFertility (
        const P1_RunPrm& prm,
        const P1_Gen_RichCoastFertilityPrm& sp = p1_gen_rich_coast_fertility_prm_def ());

    bool generate (const u8* terrain, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_RichCoastFertilityRslt& result () const;
    u16 brush_w () const;
    u16 brush_h () const;
    const u16* brush_data () const;
    void save_brush (cstr path) const;
    void save_output (cstr path) const;

private:
    P1_Gen_RichCoastFertility (const P1_Gen_RichCoastFertility& other) = delete;
    P1_Gen_RichCoastFertility (P1_Gen_RichCoastFertility&& other) = delete;

    bool build_brush ();
    bool build_fac_stamps ();
    u16 build_fert_ov_blk (
        const u8* terrain,
        u16 w,
        u16 h,
        u16 stamp_lim,
        const u16* dist_coast,
        u16* ov);

    P1_RunPrm m_prm;
    P1_Gen_RichCoastFertilityPrm m_sp;
    bool m_valid_generation;
    P1_Gen_RichCoastFertilityRslt m_rslt;
    Whiteboard_2B m_ov_wb;
    u16 m_brush_w;
    u16 m_brush_h;
    u16 m_brush_sp_n;
    i8 m_brush_sp_dx[(P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u) * (P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u)];
    i8 m_brush_sp_dy[(P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u) * (P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u)];
    u16 m_brush_sp_v[(P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u) * (P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u)];
    u16 m_brush[(P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u) * (P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u)];
    u16 m_fac_sp_v[P1_RICH_COAST_FAC_N][P1_RICH_COAST_BRUSH_CAP];
};

#endif // P1_GEN_RICH_COAST_FERTILITY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
