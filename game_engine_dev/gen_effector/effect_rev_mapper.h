//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EFFECT_REV_MAPPER_H
#define EFFECT_REV_MAPPER_H

#include "game_primitives.h"
#include "effect_map.h"

class StaticParsingManager;

//================================================================================================================================
//=> - EffectRevMapper factory -
//================================================================================================================================

class EffectRevMapper {
public:
    static EffectMapStruct* build_flat_list (const StaticParsingManager& mgr, u16* out_count);
    static void release_flat_list (EffectMapStruct* rows);

private:
    EffectRevMapper () = delete;
    EffectRevMapper (const EffectRevMapper& other) = delete;
    EffectRevMapper (EffectRevMapper&& other) = delete;
    EffectRevMapper& operator= (const EffectRevMapper& other) = delete;
    EffectRevMapper& operator= (EffectRevMapper&& other) = delete;
    static bool is_effect_set (const ItemEffectStruct& fx);
};

#endif // EFFECT_REV_MAPPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
