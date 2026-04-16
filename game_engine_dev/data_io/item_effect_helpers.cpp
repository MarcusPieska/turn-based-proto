//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "item_effect_helpers.h"

ItemEffectType ItemEffectHelper::type_str_to_enum (const std::string& s) {
    if (s == "NONE") { return ItemEffectType::NONE; }
    if (s == "BOOSTER") { return ItemEffectType::BOOSTER; }
    if (s == "BUILD") { return ItemEffectType::BUILD; }
    if (s == "ENABLE") { return ItemEffectType::ENABLE; }
    if (s == "RESEARCH_TECH") { return ItemEffectType::RESEARCH_TECH; }
    if (s == "TRAIN") { return ItemEffectType::TRAIN; }
    if (s == "TERRAIN_BOOSTER") { return ItemEffectType::TERRAIN_BOOSTER; }
    return ItemEffectType::NONE;
}

std::string ItemEffectHelper::type_enum_to_str (ItemEffectType v) {
    switch (v) {
        case ItemEffectType::NONE : return "NONE";
        case ItemEffectType::BOOSTER : return "BOOSTER";
        case ItemEffectType::BUILD : return "BUILD";
        case ItemEffectType::ENABLE : return "ENABLE";
        case ItemEffectType::RESEARCH_TECH : return "RESEARCH_TECH";
        case ItemEffectType::TRAIN : return "TRAIN";
        case ItemEffectType::TERRAIN_BOOSTER : return "TERRAIN_BOOSTER";
        default: return "NONE";
    }
}

ItemEffectBoosterType ItemEffectHelper::booster_type_str_to_enum (const std::string& s) {
    if (s == "NONE") { return ItemEffectBoosterType::NONE; }
    if (s == "SCIENCE") { return ItemEffectBoosterType::SCIENCE; }
    if (s == "HAPPINESS") { return ItemEffectBoosterType::HAPPINESS; }
    if (s == "PRODUCTION") { return ItemEffectBoosterType::PRODUCTION; }
    if (s == "AIR_DEFENSE") { return ItemEffectBoosterType::AIR_DEFENSE; }
    if (s == "AIR_RANGE") { return ItemEffectBoosterType::AIR_RANGE; }
    if (s == "COMMERCE") { return ItemEffectBoosterType::COMMERCE; }
    if (s == "CORRUPTION") { return ItemEffectBoosterType::CORRUPTION; }
    if (s == "DEFENSE") { return ItemEffectBoosterType::DEFENSE; }
    if (s == "ESPIONAGE") { return ItemEffectBoosterType::ESPIONAGE; }
    if (s == "MOVEMENT") { return ItemEffectBoosterType::MOVEMENT; }
    if (s == "NUKE_DEFENSE") { return ItemEffectBoosterType::NUKE_DEFENSE; }
    if (s == "POLLUTION") { return ItemEffectBoosterType::POLLUTION; }
    if (s == "POP_GROWTH") { return ItemEffectBoosterType::POP_GROWTH; }
    if (s == "SEA_TRADE") { return ItemEffectBoosterType::SEA_TRADE; }
    if (s == "SHIP_DEFENSE") { return ItemEffectBoosterType::SHIP_DEFENSE; }
    if (s == "SHIP_MOVEMENT") { return ItemEffectBoosterType::SHIP_MOVEMENT; }
    if (s == "SHIP_TRAINING") { return ItemEffectBoosterType::SHIP_TRAINING; }
    if (s == "UNIT_EXP") { return ItemEffectBoosterType::UNIT_EXP; }
    if (s == "UPGRADE_COST") { return ItemEffectBoosterType::UPGRADE_COST; }
    if (s == "WAR_WEAR") { return ItemEffectBoosterType::WAR_WEAR; }
    return ItemEffectBoosterType::NONE;
}

std::string ItemEffectHelper::booster_type_enum_to_str (ItemEffectBoosterType v) {
    switch (v) {
        case ItemEffectBoosterType::NONE : return "NONE";
        case ItemEffectBoosterType::SCIENCE : return "SCIENCE";
        case ItemEffectBoosterType::HAPPINESS : return "HAPPINESS";
        case ItemEffectBoosterType::PRODUCTION : return "PRODUCTION";
        case ItemEffectBoosterType::AIR_DEFENSE : return "AIR_DEFENSE";
        case ItemEffectBoosterType::AIR_RANGE : return "AIR_RANGE";
        case ItemEffectBoosterType::COMMERCE : return "COMMERCE";
        case ItemEffectBoosterType::CORRUPTION : return "CORRUPTION";
        case ItemEffectBoosterType::DEFENSE : return "DEFENSE";
        case ItemEffectBoosterType::ESPIONAGE : return "ESPIONAGE";
        case ItemEffectBoosterType::MOVEMENT : return "MOVEMENT";
        case ItemEffectBoosterType::NUKE_DEFENSE : return "NUKE_DEFENSE";
        case ItemEffectBoosterType::POLLUTION : return "POLLUTION";
        case ItemEffectBoosterType::POP_GROWTH : return "POP_GROWTH";
        case ItemEffectBoosterType::SEA_TRADE : return "SEA_TRADE";
        case ItemEffectBoosterType::SHIP_DEFENSE : return "SHIP_DEFENSE";
        case ItemEffectBoosterType::SHIP_MOVEMENT : return "SHIP_MOVEMENT";
        case ItemEffectBoosterType::SHIP_TRAINING : return "SHIP_TRAINING";
        case ItemEffectBoosterType::UNIT_EXP : return "UNIT_EXP";
        case ItemEffectBoosterType::UPGRADE_COST : return "UPGRADE_COST";
        case ItemEffectBoosterType::WAR_WEAR : return "WAR_WEAR";
        default: return "NONE";
    }
}

ItemTerrainYield ItemEffectHelper::terrain_yield_str_to_enum (const std::string& s) {
    if (s == "NONE") { return ItemTerrainYield::NONE; }
    if (s == "FOOD") { return ItemTerrainYield::FOOD; }
    if (s == "SHIELDS") { return ItemTerrainYield::SHIELDS; }
    if (s == "COMMERCE") { return ItemTerrainYield::COMMERCE; }
    if (s == "HAPPINESS") { return ItemTerrainYield::HAPPINESS; }
    if (s == "DEFENSE") { return ItemTerrainYield::DEFENSE; }
    return ItemTerrainYield::NONE;
}

std::string ItemEffectHelper::terrain_yield_enum_to_str (ItemTerrainYield v) {
    switch (v) {
        case ItemTerrainYield::NONE : return "NONE";
        case ItemTerrainYield::FOOD : return "FOOD";
        case ItemTerrainYield::SHIELDS : return "SHIELDS";
        case ItemTerrainYield::COMMERCE : return "COMMERCE";
        case ItemTerrainYield::HAPPINESS : return "HAPPINESS";
        case ItemTerrainYield::DEFENSE : return "DEFENSE";
        default: return "NONE";
    }
}

ItemEffectsScope ItemEffectHelper::effects_scope_str_to_enum (const std::string& s) {
    if (s == "CIV") { return ItemEffectsScope::GLOBAL; }
    if (s == "NONE") { return ItemEffectsScope::NONE; }
    if (s == "LOCAL") { return ItemEffectsScope::LOCAL; }
    if (s == "CITY") { return ItemEffectsScope::CITY; }
    if (s == "GLOBAL") { return ItemEffectsScope::GLOBAL; }
    return ItemEffectsScope::NONE;
}

std::string ItemEffectHelper::effects_scope_enum_to_str (ItemEffectsScope v) {
    switch (v) {
        case ItemEffectsScope::NONE : return "NONE";
        case ItemEffectsScope::LOCAL : return "LOCAL";
        case ItemEffectsScope::CITY : return "CITY";
        case ItemEffectsScope::GLOBAL : return "GLOBAL";
        default: return "NONE";
    }
}

ItemEffectAmountMode ItemEffectHelper::amount_mode_str_to_enum (const std::string& s) {
    if (s == "NONE") { return ItemEffectAmountMode::NONE; }
    if (s == "COUNT") { return ItemEffectAmountMode::COUNT; }
    if (s == "PERCENTAGE") { return ItemEffectAmountMode::PERCENTAGE; }
    return ItemEffectAmountMode::NONE;
}

std::string ItemEffectHelper::amount_mode_enum_to_str (ItemEffectAmountMode v) {
    switch (v) {
        case ItemEffectAmountMode::NONE : return "NONE";
        case ItemEffectAmountMode::COUNT : return "COUNT";
        case ItemEffectAmountMode::PERCENTAGE : return "PERCENTAGE";
        default: return "NONE";
    }
}

ItemEffectBuildMode ItemEffectHelper::build_mode_str_to_enum (const std::string& s) {
    if (s == "NONE") { return ItemEffectBuildMode::NONE; }
    if (s == "AUTO_BUILD") { return ItemEffectBuildMode::AUTO_BUILD; }
    if (s == "NO_BUILD") { return ItemEffectBuildMode::NO_BUILD; }
    return ItemEffectBuildMode::NONE;
}

std::string ItemEffectHelper::build_mode_enum_to_str (ItemEffectBuildMode v) {
    switch (v) {
        case ItemEffectBuildMode::NONE : return "NONE";
        case ItemEffectBuildMode::AUTO_BUILD : return "AUTO_BUILD";
        case ItemEffectBuildMode::NO_BUILD : return "NO_BUILD";
        default: return "NONE";
    }
}

ItemEffectUpkeepMode ItemEffectHelper::upkeep_mode_str_to_enum (const std::string& s) {
    if (s == "NONE") { return ItemEffectUpkeepMode::NONE; }
    if (s == "NORMAL_UPKEEP") { return ItemEffectUpkeepMode::NORMAL_UPKEEP; }
    if (s == "NO_UPKEEP") { return ItemEffectUpkeepMode::NO_UPKEEP; }
    return ItemEffectUpkeepMode::NONE;
}

std::string ItemEffectHelper::upkeep_mode_enum_to_str (ItemEffectUpkeepMode v) {
    switch (v) {
        case ItemEffectUpkeepMode::NONE : return "NONE";
        case ItemEffectUpkeepMode::NORMAL_UPKEEP : return "NORMAL_UPKEEP";
        case ItemEffectUpkeepMode::NO_UPKEEP : return "NO_UPKEEP";
        default: return "NONE";
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
