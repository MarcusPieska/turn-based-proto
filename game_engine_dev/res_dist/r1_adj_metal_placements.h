//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef R1_ADJ_METAL_PLACEMENTS_H
#define R1_ADJ_METAL_PLACEMENTS_H

#include "game_primitives.h"
#include "res_dist_state.h"
#include "resource_placement.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//= - R1_Adj_MetalPlacements -
//================================================================================================================================
//=
//= Endow sectors with METAL resources and place base_n*res_wt tiles into an existing resource
//= overlay (never overwriting). Updates ResDistState. Cascades spend the same budget (60%).
//=
//================================================================================================================================

#define R1_MET_PLC_CAP 400
#define R1_MET_CLU_PCT 60

struct R1_MetSecRec {
    u16 m_x; // Sector start tile x
    u16 m_y; // Sector start tile y
    u16 m_sec; // Sector index
};

struct R1_MetAsgRec {
    u16 m_res; // Endowed resource index
    u16 m_sec; // Sector index
    u16 m_x; // Flood start x
    u16 m_y; // Flood start y
};

class R1_Adj_MetalPlacements {
public:
    R1_Adj_MetalPlacements ();
    ~R1_Adj_MetalPlacements ();

    R1_Adj_MetalPlacements (const R1_Adj_MetalPlacements& o) = delete;
    R1_Adj_MetalPlacements (R1_Adj_MetalPlacements&& o) = delete;

    bool adjust (
        u16* res_ov,
        const RuntimeStatics& s,
        ResDistState& st,
        const ResPlcMapCtx& ctx,
        const u16* sec_ov,
        u16 sec_n,
        u32 base_n,
        u32 seed);
    
    bool is_valid () const;
    u16 met_n () const;
    u16 met_at (u16 i) const;
    u16 sec_n () const;
    u16 asg_n () const;
    const R1_MetAsgRec& asg_at (u16 i) const;
    u32 plc_n () const;
    bool save_res_ppm (cstr path, const u8* terr, const u16* res_ov, u16 res_i) const;

private:
    void clr ();

    bool m_ok; // True after successful adjust
    u16 m_w; // Map width
    u16 m_h; // Map height
    u16 m_met_n; // METAL resource count (capped)
    u16 m_sec_n; // Sector start records (capped)
    u16 m_asg_n; // Sector/metal endowment count
    u32 m_plc_n; // Placed tile count this call
    u16 m_met[R1_MET_PLC_CAP]; // METAL resource indices
    R1_MetSecRec m_sec[R1_MET_PLC_CAP]; // One start point per sector
    R1_MetAsgRec m_asg[R1_MET_PLC_CAP]; // Endowment assignments
};

#endif // R1_ADJ_METAL_PLACEMENTS_H

//================================================================================================================================
//= - End of file -
//================================================================================================================================
