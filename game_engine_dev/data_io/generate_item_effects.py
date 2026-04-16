#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Enum item lists (reuse for other generators, e.g. string-to-enum) -
#================================================================================================================================#

ie_type_enum = []
ie_type_enum.append("NONE")
ie_type_enum.append("BOOSTER")
ie_type_enum.append("BUILD")
ie_type_enum.append("ENABLE")
ie_type_enum.append("RESEARCH_TECH")
ie_type_enum.append("TRAIN")
ie_type_enum.append("TERRAIN_BOOSTER")

ie_booster_type_enum = []
ie_booster_type_enum.append("NONE")
ie_booster_type_enum.append("SCIENCE")
ie_booster_type_enum.append("HAPPINESS")
ie_booster_type_enum.append("PRODUCTION")
ie_booster_type_enum.append("AIR_DEFENSE")
ie_booster_type_enum.append("AIR_RANGE")
ie_booster_type_enum.append("COMMERCE")
ie_booster_type_enum.append("CORRUPTION")
ie_booster_type_enum.append("DEFENSE")
ie_booster_type_enum.append("ESPIONAGE")
ie_booster_type_enum.append("MOVEMENT")
ie_booster_type_enum.append("NUKE_DEFENSE")
ie_booster_type_enum.append("POLLUTION")
ie_booster_type_enum.append("POP_GROWTH")
ie_booster_type_enum.append("SEA_TRADE")
ie_booster_type_enum.append("SHIP_DEFENSE")
ie_booster_type_enum.append("SHIP_MOVEMENT")
ie_booster_type_enum.append("SHIP_TRAINING")
ie_booster_type_enum.append("UNIT_EXP")
ie_booster_type_enum.append("UPGRADE_COST")
ie_booster_type_enum.append("WAR_WEAR")

ie_terrain_yield_enum = []
ie_terrain_yield_enum.append("NONE")
ie_terrain_yield_enum.append("FOOD")
ie_terrain_yield_enum.append("SHIELDS")
ie_terrain_yield_enum.append("COMMERCE")
ie_terrain_yield_enum.append("HAPPINESS")
ie_terrain_yield_enum.append("DEFENSE")

ie_scope_enum = []
ie_scope_enum.append("NONE")
ie_scope_enum.append("LOCAL")
ie_scope_enum.append("CITY")
ie_scope_enum.append("GLOBAL")

ie_amount_mode_enum = []
ie_amount_mode_enum.append("NONE")
ie_amount_mode_enum.append("COUNT")
ie_amount_mode_enum.append("PERCENTAGE")

ie_build_mode_enum = []
ie_build_mode_enum.append("NONE")
ie_build_mode_enum.append("AUTO_BUILD")
ie_build_mode_enum.append("NO_BUILD")

ie_upkeep_mode_enum = []
ie_upkeep_mode_enum.append("NONE")
ie_upkeep_mode_enum.append("NORMAL_UPKEEP")
ie_upkeep_mode_enum.append("NO_UPKEEP")

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
