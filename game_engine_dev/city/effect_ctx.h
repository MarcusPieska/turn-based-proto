//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EFFECT_CTX_H
#define EFFECT_CTX_H

#include "game_primitives.h"
#include "gen_effector/effect_map.h"

class BitArrayCL;
class GeneralBitBank;
class RuntimeStatics;

//================================================================================================================================
//=> - EmpireBldFn -
//================================================================================================================================

typedef bool (*EmpireBldFn) (u16 owner, u16 bld_idx, void* ud);
typedef u8 (*CityJobSeatFn) (u16 city_idx, u16 job_id, void* ud);

//================================================================================================================================
//=> - EffectCtx -
//================================================================================================================================
//
//  Runtime slice for resolving effect-map rows against live city and player state.
//
//================================================================================================================================

struct EffectCtx {
    u16 m_owner; // Seat index that owns m_city_idx
    u16 m_city_idx; // City batch index in GeneralBitBank pools
    const BitArrayCL* m_tech; // Researched-tech bitset for m_owner
    const GeneralBitBank* m_bld_bank; // Per-city building flags
    const u16* m_wonder_city; // GameState wonder city row; length m_wonder_n
    u16 m_wonder_n; // Wonder catalog count
    const u16* m_small_wonder_city; // Player small-wonder city row; length m_small_wonder_n
    u16 m_small_wonder_n; // Small-wonder catalog count
    EmpireBldFn m_emp_bld_fn; // Optional empire-wide building probe for CIV-scoped building sources
    void* m_emp_bld_ud; // User data for m_emp_bld_fn
    CityJobSeatFn m_job_seat_fn; // Optional JobCountArray seat probe for CITY_JOB enablers
    void* m_job_seat_ud; // User data for m_job_seat_fn
    const u16* m_trait_idx; // Owner civ trait indices; length m_trait_n (U16_KEY_NULL pads)
    u16 m_trait_n; // Trait index count for m_trait_idx
};

bool effect_src_active (const EffectMapStruct& row, const EffectCtx& ctx);
bool effect_applies_to_city (const EffectMapStruct& row, const EffectCtx& ctx);
void effect_ctx_bind_civ_traits (EffectCtx& ctx, const RuntimeStatics& st, u16 civ_idx);

#endif // EFFECT_CTX_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
