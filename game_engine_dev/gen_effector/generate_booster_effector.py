#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True
sys.path.append("../data_io")

from item_effects import ie_type_enum 
from item_effects import ie_booster_type_enum
from item_effects import ie_terrain_yield_enum
from item_effects import ie_scope_enum
from item_effects import ie_amount_mode_enum
from item_effects import ie_build_mode_enum
from item_effects import ie_upkeep_mode_enum

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def unroll_enum_members(items):
    indices = list(range(len(items)))
    return ",\n    ".join(["%s = %d" % (item, idx) for item, idx in zip(items, indices)])

def unroll_to_enum(items, context):
    frm_str = "if (s == \"%s\") { return %s::%s; }"
    return "\n    ".join([frm_str % (item, context, item) for item in items])

def unroll_to_str(items, context):
    frm_str = "case %s::%s : return \"%s\";"
    return "\n        ".join([frm_str % (context, item, item) for item in items])

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    sub_pairs = []

    sub_pairs.append(("[IE_TYPE_ENUM_MEMBERS]", unroll_enum_members(ie_type_enum)))
    sub_pairs.append(("[IE_BOOSTER_TYPE_ENUM_MEMBERS]", unroll_enum_members(ie_booster_type_enum)))
    sub_pairs.append(("[IE_TERRAIN_YIELD_ENUM_MEMBERS]", unroll_enum_members(ie_terrain_yield_enum)))
    sub_pairs.append(("[IE_SCOPE_ENUM_MEMBERS]", unroll_enum_members(ie_scope_enum)))
    sub_pairs.append(("[IE_AMOUNT_MODE_ENUM_MEMBERS]", unroll_enum_members(ie_amount_mode_enum)))
    sub_pairs.append(("[IE_BUILD_MODE_ENUM_MEMBERS]", unroll_enum_members(ie_build_mode_enum)))
    sub_pairs.append(("[IE_UPKEEP_MODE_ENUM_MEMBERS]", unroll_enum_members(ie_upkeep_mode_enum)))
    
    sub_pairs.append(("[IE_TYPE_FROM_STRING_BODY]", unroll_to_enum(ie_type_enum, "ItemEffectType")))
    sub_pairs.append(("[IE_BOOSTER_TYPE_FROM_STRING_BODY]", unroll_to_enum(ie_booster_type_enum, "ItemEffectBoosterType")))
    sub_pairs.append(("[IE_TERRAIN_YIELD_FROM_STRING_BODY]", unroll_to_enum(ie_terrain_yield_enum, "ItemTerrainYield")))
    sub_pairs.append(("[IE_SCOPE_FROM_STRING_BODY]", unroll_to_enum(ie_scope_enum, "ItemEffectsScope")))
    sub_pairs.append(("[IE_AMOUNT_MODE_FROM_STRING_BODY]", unroll_to_enum(ie_amount_mode_enum, "ItemEffectAmountMode")))
    sub_pairs.append(("[IE_BUILD_MODE_FROM_STRING_BODY]", unroll_to_enum(ie_build_mode_enum, "ItemEffectBuildMode")))
    sub_pairs.append(("[IE_UPKEEP_MODE_FROM_STRING_BODY]", unroll_to_enum(ie_upkeep_mode_enum, "ItemEffectUpkeepMode")))

    sub_pairs.append(("[IE_TYPE_TO_STRING_SWITCH_BODY]", unroll_to_str(ie_type_enum, "ItemEffectType")))
    sub_pairs.append(("[IE_BOOSTER_TYPE_TO_STRING_SWITCH_BODY]", unroll_to_str(ie_booster_type_enum, "ItemEffectBoosterType")))
    sub_pairs.append(("[IE_TERRAIN_YIELD_TO_STRING_SWITCH_BODY]", unroll_to_str(ie_terrain_yield_enum, "ItemTerrainYield")))
    sub_pairs.append(("[IE_SCOPE_TO_STRING_SWITCH_BODY]", unroll_to_str(ie_scope_enum, "ItemEffectsScope")))
    sub_pairs.append(("[IE_AMOUNT_MODE_TO_STRING_SWITCH_BODY]", unroll_to_str(ie_amount_mode_enum, "ItemEffectAmountMode")))
    sub_pairs.append(("[IE_BUILD_MODE_TO_STRING_SWITCH_BODY]", unroll_to_str(ie_build_mode_enum, "ItemEffectBuildMode")))
    sub_pairs.append(("[IE_UPKEEP_MODE_TO_STRING_SWITCH_BODY]", unroll_to_str(ie_upkeep_mode_enum, "ItemEffectUpkeepMode")))


    file_prefix = "TEMPLATE_"
    files = []
    files.append("item_effects.h")
    files.append("item_effect_helpers.h")
    files.append("item_effect_helpers.cpp")

    for output_path in files:
        with open(file_prefix + output_path, "r") as ptr:
            content = ptr.read()
        for old_string, new_string in sub_pairs:
            content = content.replace(old_string, new_string)
        with open(output_path, "w") as ptr:
            ptr.write(content)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
