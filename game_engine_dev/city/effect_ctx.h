//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EFFECT_CTX_H
#define EFFECT_CTX_H

#include "game_primitives.h"
#include "gen_effector/effect_map.h"

class BitArrayCL;
class GeneralBitBank;

//================================================================================================================================
//=> - EmpireBldFn -
//================================================================================================================================

typedef bool (*EmpireBldFn) (u16 owner, u16 bld_idx, void* ud);

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
};

bool effect_src_active (const EffectMapStruct& row, const EffectCtx& ctx);
bool effect_applies_to_city (const EffectMapStruct& row, const EffectCtx& ctx);

#endif // EFFECT_CTX_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
