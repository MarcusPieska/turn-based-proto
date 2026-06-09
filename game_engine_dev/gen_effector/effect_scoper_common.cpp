//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "effect_scoper_common.h"

//================================================================================================================================
//=> - Scope matching -
//================================================================================================================================

bool effect_matches_scope (const ItemEffectStruct& fx, ItemEffectsScope want) {
    if (fx.type == static_cast<u16>(ItemEffectType::NONE)) {
        return false;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::RESEARCH_TECH)) {
        return false;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::TRAIN)) {
        return want == ItemEffectsScope::CITY;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::BOOSTER)) {
        return fx.effect.booster.scope == want;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::TERRAIN_BOOSTER)) {
        const ItemEffectTerrainBooster& tb = *reinterpret_cast<const ItemEffectTerrainBooster*>(&fx.effect);
        return tb.scope == want;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::BUILD)) {
        return fx.effect.build.scope == want;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::ENABLE)) {
        return fx.effect.enable.scope == want;
    }
    return false;
}

//================================================================================================================================
//=> - Scope row copy -
//================================================================================================================================

u16 count_scope_rows (const EffectMapStruct* rows, u16 row_count, ItemEffectsScope want) {
    u16 n = 0;
    if (rows == nullptr) {
        return 0;
    }
    for (u16 i = 0; i < row_count; ++i) {
        if (effect_matches_scope(rows[i].effect, want)) {
            ++n;
        }
    }
    return n;
}

void copy_scope_rows (const EffectMapStruct* rows, u16 row_count, ItemEffectsScope want, EffectMapStruct** out_rows, u16* out_count) {
    *out_rows = nullptr;
    *out_count = 0;
    if (rows == nullptr || row_count == 0) {
        return;
    }
    const u16 n = count_scope_rows(rows, row_count, want);
    if (n == 0) {
        return;
    }
    EffectMapStruct* dst = new EffectMapStruct[n];
    u16 k = 0;
    for (u16 i = 0; i < row_count; ++i) {
        if (!effect_matches_scope(rows[i].effect, want)) {
            continue;
        }
        dst[k] = rows[i];
        ++k;
    }
    *out_rows = dst;
    *out_count = n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
