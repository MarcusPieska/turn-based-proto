//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EFFECT_SCOPER_COMMON_H
#define EFFECT_SCOPER_COMMON_H

#include "effect_map.h"

bool effect_matches_scope (const ItemEffectStruct& fx, ItemEffectsScope want);
u16 count_scope_rows (const EffectMapStruct* rows, u16 row_count, ItemEffectsScope want);
void copy_scope_rows (const EffectMapStruct* rows, u16 row_count, ItemEffectsScope want, EffectMapStruct** out_rows, u16* out_count);

#endif

//================================================================================================================================
//=> - End -
//================================================================================================================================