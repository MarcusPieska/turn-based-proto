//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EFFECT_VECTOR_H
#define EFFECT_VECTOR_H

#include <string>
#include <iosfwd>

#include "../game_primitives.h"

//================================================================================================================================
//=> - Enums -
//================================================================================================================================

enum class Scope {
    CITY = 0,
    LOCAL = 1,
    CIV = 2
};

enum class Target {
    HAPPINESS = 0,
    COMMERCE = 1,
    SCIENCE = 2,
    SHIP_MOVEMENT = 3,
    SHIP_TRAINING = 4,
    DEFENSE = 5,
    SHIP_DEFENSE = 6,
    UPGRADE_COST = 7,
    WAR_WEAR = 8,
    POP_GROWTH = 9,
    CORRUPTION = 10,
    SEA_TRADE = 11,
    PRODUCTION = 12,
    POLLUTION = 13,
    AIR_RANGE = 14,
    MOVEMENT = 15,
    AIR_DEFENSE = 16,
    NUKE_DEFENSE = 17,
    ESPIONAGE = 18,
    UNIT_EXP = 19
};

enum class EffectType {
    BUILD = 0,
    RESEARCH_TECH = 1,
    BOOSTER = 2,
    TRAIN = 3
};

enum class AutoBuild {
    NO_BUILD = 0,
    AUTO_BUILD = 1
};

enum class NoUpkeep {
    NORMAL_UPKEEP = 0,
    NO_UPKEEP = 1
};

enum class ValueType {
    COUNT = 0,
    PERCENTAGE = 1
};

//================================================================================================================================
//=> - Effect structs -
//================================================================================================================================

typedef struct BuildEffect {
    u16 building_index;
    u16 scope;
    u16 auto_build;
    u16 no_upkeep;
} BuildEffect;

typedef struct ResearchTechEffect {
    u16 tech_count;
} ResearchTechEffect;

typedef struct BoosterEffect {
    u16 target;
    i16 value;
    u16 scope;
    u16 value_type;
} BoosterEffect;

typedef struct TrainEffect {
    u16 unit_index;
    u16 count;
} TrainEffect;

//================================================================================================================================
//=> - Tagged union for effects -
//================================================================================================================================

struct Effect {
    EffectType type;
    union {
        BuildEffect build;
        ResearchTechEffect research_tech;
        BoosterEffect booster;
        TrainEffect train;
    } data;
};

//================================================================================================================================
//=> - EffectIO class -
//================================================================================================================================

class EffectIO {
public:

    EffectIO (const std::string& filename) : filename(filename) {}

    int validate_and_count () const;
    void print_content () const;
    void parse_and_allocate () const;

private:
    EffectIO (const EffectIO& other) = delete;
    EffectIO (EffectIO&& other) = delete;

    std::string filename;
};

//================================================================================================================================
//=> - EffectVector class -
//================================================================================================================================

class EffectVector {
public:

    static const Effect* get_effect_data_array ();
    static const Effect& get_effect (u32 index);
    static u32 get_count ();
    static void deallocate_names ();
    static void find_effects_by_name (EffectIndices* indices, const char* name);

private:
    EffectVector () = delete;
    ~EffectVector () = delete;
    EffectVector (const EffectVector& other) = delete;
    EffectVector (EffectVector&& other) = delete;
};

#endif // EFFECT_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
