//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ITEM_EFFECTS_H
#define ITEM_EFFECTS_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Effect limits -
//================================================================================================================================

#define MAX_EFFECT_COUNT 5 

//================================================================================================================================
//=> - Common effect enums -
//================================================================================================================================

enum class ItemEffectType : u16 {
    NONE = 0,
    BOOSTER = 1,
    BUILD = 2,
    ENABLE = 3,
    RESEARCH_TECH = 4,
    TRAIN = 5,
    TERRAIN_BOOSTER = 6
};

enum class ItemEffectBoosterType : u16 {
    NONE = 0,
    SCIENCE = 1,
    HAPPINESS = 2,
    PRODUCTION = 3,
    AIR_DEFENSE = 4,
    AIR_RANGE = 5,
    COMMERCE = 6,
    CORRUPTION = 7,
    DEFENSE = 8,
    ESPIONAGE = 9,
    MOVEMENT = 10,
    NUKE_DEFENSE = 11,
    POLLUTION = 12,
    POP_GROWTH = 13,
    SEA_TRADE = 14,
    SHIP_DEFENSE = 15,
    SHIP_MOVEMENT = 16,
    SHIP_TRAINING = 17,
    UNIT_EXP = 18,
    UPGRADE_COST = 19,
    WAR_WEAR = 20
};

enum class ItemTerrainYield : u8 {
    NONE = 0,
    FOOD = 1,
    SHIELDS = 2,
    COMMERCE = 3,
    HAPPINESS = 4,
    DEFENSE = 5
};

enum class ItemEffectsScope : u8 {
    NONE = 0,
    LOCAL = 1,
    CITY = 2,
    GLOBAL = 3
};

enum class ItemEffectAmountMode : u8 {
    NONE = 0,
    COUNT = 1,
    PERCENTAGE = 2
};

enum class ItemEffectBuildMode : u8 {
    NONE = 0,
    AUTO_BUILD = 1,
    NO_BUILD = 2
};

enum class ItemEffectUpkeepMode : u8 {
    NONE = 0,
    NORMAL_UPKEEP = 1,
    NO_UPKEEP = 2
};

//================================================================================================================================
//=> - Per-effect payload structs -
//================================================================================================================================

typedef struct ItemEffectBooster {
    ItemEffectBoosterType target_id;
    i16 amount;
    ItemEffectsScope scope;
    ItemEffectAmountMode amount_mode;
} ItemEffectBooster;

typedef struct ItemEffectTerrainBooster {
    u16 terrain_id;
    i8 amount;
    ItemTerrainYield yield; 
    ItemEffectsScope scope;
    ItemEffectAmountMode amount_mode;
} ItemEffectTerrainBooster;

typedef struct ItemEffectBuild {
    u16 building_id;
    ItemEffectsScope scope;
    ItemEffectBuildMode build_mode;
    ItemEffectUpkeepMode upkeep_mode;
} ItemEffectBuild;

typedef struct ItemEffectEnable {
    u16 feature_id;
    ItemEffectsScope scope;
} ItemEffectEnable;

typedef struct ItemEffectResearchTech {
    u16 tech_count;
} ItemEffectResearchTech;

typedef struct ItemEffectTrain {
    u16 unit_id;
    u8 turns_interval;
} ItemEffectTrain;

//================================================================================================================================
//=> - Union + tagged entry -
//================================================================================================================================

typedef union ItemEffectsUnion {
    ItemEffectBooster booster;
    ItemEffectBuild build;
    ItemEffectEnable enable;
    ItemEffectResearchTech research_tech;
    ItemEffectTrain train;
} ItemEffectsUnion;

typedef struct ItemEffectStruct {
    u16 type;
    ItemEffectsUnion effect;
} ItemEffectStruct;

typedef struct ItemEffectsStruct {
    ItemEffectStruct items[MAX_EFFECT_COUNT];
} ItemEffectsStruct;

#endif // ITEM_EFFECTS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
