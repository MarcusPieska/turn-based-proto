//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef R1_ADJ_CASH_CROP_PLACEMENTS_H
#define R1_ADJ_CASH_CROP_PLACEMENTS_H

#include "game_primitives.h"
#include "res_dist_state.h"
#include "resource_placement.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//= - R1_Adj_CashCropPlacements -
//================================================================================================================================
//=
//= Endow sectors with CASH_CROP resources and place base_n*res_wt tiles into an existing resource
//= overlay (never overwriting). Updates ResDistState. No adjacent cascade. Keeps 25% of sectors (skips 75%).
//=
//================================================================================================================================

#define R1_CSH_PLC_CAP 400
#define R1_CSH_SEC_KEEP_PCT 25

struct R1_CshSecRec {
    u16 m_x; // Sector start tile x
    u16 m_y; // Sector start tile y
    u16 m_sec; // Sector index
};

struct R1_CshAsgRec {
    u16 m_res; // Endowed resource index
    u16 m_sec; // Sector index
    u16 m_x; // Flood start x
    u16 m_y; // Flood start y
};

class R1_Adj_CashCropPlacements {
public:
    R1_Adj_CashCropPlacements ();
    ~R1_Adj_CashCropPlacements ();

    R1_Adj_CashCropPlacements (const R1_Adj_CashCropPlacements& o) = delete;
    R1_Adj_CashCropPlacements (R1_Adj_CashCropPlacements&& o) = delete;

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
    u16 csh_n () const;
    u16 csh_at (u16 i) const;
    u16 sec_n () const;
    u16 asg_n () const;
    const R1_CshAsgRec& asg_at (u16 i) const;
    u32 plc_n () const;
    bool save_res_ppm (cstr path, const u8* terr, const u16* res_ov, u16 res_i) const;

private:
    void clr ();

    bool m_ok; // True after successful adjust
    u16 m_w; // Map width
    u16 m_h; // Map height
    u16 m_csh_n; // CASH_CROP resource count (capped)
    u16 m_sec_n; // Sector start records (capped)
    u16 m_asg_n; // Sector/cash_crop endowment count
    u32 m_plc_n; // Placed tile count this call
    u16 m_csh[R1_CSH_PLC_CAP]; // CASH_CROP resource indices
    R1_CshSecRec m_sec[R1_CSH_PLC_CAP]; // One start point per sector
    R1_CshAsgRec m_asg[R1_CSH_PLC_CAP]; // Endowment assignments
};

#endif // R1_ADJ_CASH_CROP_PLACEMENTS_H

//================================================================================================================================
//= - End of file -
//================================================================================================================================
