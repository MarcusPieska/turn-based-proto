//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef JOB_ASSIGNER_H
#define JOB_ASSIGNER_H

#include "game_primitives.h"

struct EffectCtx;
class RuntimeStatics;

//================================================================================================================================
//=> - JobAssigner -
//================================================================================================================================
//
//  Stateless helper: fills JobCountArray seats from DynJobSlotRegister capacity and CityJobTraitOrderings.
//  assign_one claims the next preferred seat with remaining room; fill clears then assigns up to pop_limit.
//
//================================================================================================================================

class JobAssigner {
public:
    JobAssigner () = delete;

    static void clear (u16 city_idx);
    static u16 assign_one (u16 city_idx, u16 trait_idx, const EffectCtx& ctx, const RuntimeStatics& st);
    static u16 fill (u16 city_idx, u16 trait_idx, u16 pop_limit, const EffectCtx& ctx, const RuntimeStatics& st);
};

#endif // JOB_ASSIGNER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
