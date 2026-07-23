//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef R1_ADJ_RES_PLACE_H
#define R1_ADJ_RES_PLACE_H

#include "game_primitives.h"
#include "resource_placement.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//=> - R1_Adj_ResPlace -
//================================================================================================================================

class R1_Adj_ResPlace {
public:
    R1_Adj_ResPlace ();

    bool adjust (
        u16* res_ov,
        u16 w,
        u16 h,
        const ResPlcMapCtx& ctx,
        const RuntimeStatics& s,
        u16 res_i,
        u32 base_n,
        u32 seed,
        u32* placed_n);
    bool is_valid () const;

private:
    R1_Adj_ResPlace (const R1_Adj_ResPlace& o) = delete;
    R1_Adj_ResPlace (R1_Adj_ResPlace&& o) = delete;

    bool m_valid;
};

bool r1_adj_res_place_u8 (
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    u16 res_i,
    u32 base_n,
    u32 seed,
    u8* out,
    u32 out_n,
    u32* placed_n,
    double* sec_out);

bool r1_adj_res_place_fair (
    u16* res_ov,
    u16 w,
    u16 h,
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    const u16* res_is,
    const u32* soft_want,
    const u32* hard_want,
    u16 res_n,
    u32 seed,
    u32* placed_out,
    u32* placed_tot);

#endif // R1_ADJ_RES_PLACE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
