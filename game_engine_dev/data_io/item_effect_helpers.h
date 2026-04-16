//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ITEM_EFFECT_HELPER_H
#define ITEM_EFFECT_HELPER_H

#include <string>

#include "item_effects.h"

//================================================================================================================================
//=> - ItemEffectHelper -
//================================================================================================================================

class ItemEffectHelper {
public:
    static ItemEffectType type_str_to_enum (const std::string& s);
    static std::string type_enum_to_str (ItemEffectType v);

    static ItemEffectBoosterType booster_type_str_to_enum (const std::string& s);
    static std::string booster_type_enum_to_str (ItemEffectBoosterType v);

    static ItemTerrainYield terrain_yield_str_to_enum (const std::string& s);
    static std::string terrain_yield_enum_to_str (ItemTerrainYield v);

    static ItemEffectsScope effects_scope_str_to_enum (const std::string& s);
    static std::string effects_scope_enum_to_str (ItemEffectsScope v);

    static ItemEffectAmountMode amount_mode_str_to_enum (const std::string& s);
    static std::string amount_mode_enum_to_str (ItemEffectAmountMode v);

    static ItemEffectBuildMode build_mode_str_to_enum (const std::string& s);
    static std::string build_mode_enum_to_str (ItemEffectBuildMode v);

    static ItemEffectUpkeepMode upkeep_mode_str_to_enum (const std::string& s);
    static std::string upkeep_mode_enum_to_str (ItemEffectUpkeepMode v);
};

#endif // ITEM_EFFECT_HELPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
