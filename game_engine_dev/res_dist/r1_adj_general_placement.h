//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef R1_ADJ_GENERAL_PLACEMENT_H
#define R1_ADJ_GENERAL_PLACEMENT_H

#include "game_primitives.h"
#include "res_dist_state.h"
#include "resource_placement.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//= - R1_Adj_GeneralPlacement -
//================================================================================================================================
//=
//= Places remaining unmet resources into an existing overlay via one fair multi-resource pass.
//= Aims for base_n*res_wt per resource; may place up to 2x that. Never overwrites occupied tiles.
//=
//================================================================================================================================

class R1_Adj_GeneralPlacement {
public:
    R1_Adj_GeneralPlacement ();

    R1_Adj_GeneralPlacement (const R1_Adj_GeneralPlacement& o) = delete;
    R1_Adj_GeneralPlacement (R1_Adj_GeneralPlacement&& o) = delete;

    bool adjust (
        u16* res_ov,
        u16 w,
        u16 h,
        const ResPlcMapCtx& ctx,
        const RuntimeStatics& s,
        ResDistState& st,
        u32 base_n,
        u32 seed);
    bool is_valid () const;
    u32 placed_n () const;

private:
    bool m_ok; // True after successful adjust
    u32 m_plc_n; // Tiles placed this call
};

#endif // R1_ADJ_GENERAL_PLACEMENT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
