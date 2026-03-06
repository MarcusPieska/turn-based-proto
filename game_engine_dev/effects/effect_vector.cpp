//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include <sstream>

#include "str_mng.h"
#include "effect_vector.h"

//================================================================================================================================
//=> - Private structures and globals -
//================================================================================================================================

static Effect* effect_data_array = nullptr;
static char** effect_names_array = nullptr;
static u32 effect_data_count = 0;

//================================================================================================================================
//=> - Helper functions for parsing -
//================================================================================================================================

static Scope parse_scope(const std::string& scope_str) {
    if (scope_str == "CITY") return Scope::CITY;
    if (scope_str == "LOCAL") return Scope::LOCAL;
    if (scope_str == "CIV") return Scope::CIV;
    
    printf("Unknown scope: %s\n", scope_str.c_str());
    exit(1);
    
    return Scope::CITY; // to quiet the compiler
}

static Target parse_target(const std::string& target_str) {
    if (target_str == "HAPPINESS") return Target::HAPPINESS;
    if (target_str == "COMMERCE") return Target::COMMERCE;
    if (target_str == "SCIENCE") return Target::SCIENCE;
    if (target_str == "SHIP_MOVEMENT") return Target::SHIP_MOVEMENT;
    if (target_str == "SHIP_TRAINING") return Target::SHIP_TRAINING;
    if (target_str == "DEFENSE") return Target::DEFENSE;
    if (target_str == "SHIP_DEFENSE") return Target::SHIP_DEFENSE;
    if (target_str == "UPGRADE_COST") return Target::UPGRADE_COST;
    if (target_str == "WAR_WEAR") return Target::WAR_WEAR;
    if (target_str == "POP_GROWTH") return Target::POP_GROWTH;
    if (target_str == "CORRUPTION") return Target::CORRUPTION;
    if (target_str == "SEA_TRADE") return Target::SEA_TRADE;
    if (target_str == "PRODUCTION") return Target::PRODUCTION;
    if (target_str == "POLLUTION") return Target::POLLUTION;
    if (target_str == "AIR_RANGE") return Target::AIR_RANGE;
    if (target_str == "MOVEMENT") return Target::MOVEMENT;
    if (target_str == "AIR_DEFENSE") return Target::AIR_DEFENSE;
    if (target_str == "NUKE_DEFENSE") return Target::NUKE_DEFENSE;
    if (target_str == "ESPIONAGE") return Target::ESPIONAGE;
    if (target_str == "UNIT_EXP") return Target::UNIT_EXP;
    
    printf("Unknown target: %s\n", target_str.c_str());
    exit(1);

    return Target::HAPPINESS; // to quiet the compiler
}

static AutoBuild parse_auto_build(const std::string& auto_build_str) {
    StringTrimmer trimmer(" \t\r\n");
    std::string trimmed = trimmer.trim(auto_build_str);

    if (trimmed == "AUTO_BUILD") return AutoBuild::AUTO_BUILD;
    if (trimmed == "NO_BUILD") return AutoBuild::NO_BUILD;

    printf("Unknown auto build: %s\n", auto_build_str.c_str());
    exit(1);
    
    return AutoBuild::NO_BUILD; // to quiet the compiler
}

static NoUpkeep parse_no_upkeep(const std::string& no_upkeep_str) {
    StringTrimmer trimmer(" \t\r\n");
    std::string trimmed = trimmer.trim(no_upkeep_str);

    if (trimmed == "NO_UPKEEP") return NoUpkeep::NO_UPKEEP;
    if (trimmed == "NORMAL_UPKEEP") return NoUpkeep::NORMAL_UPKEEP;

    printf("Unknown no upkeep: %s\n", no_upkeep_str.c_str());
    exit(1);

    return NoUpkeep::NORMAL_UPKEEP; // to quiet the compiler
}

static ValueType parse_value_type(const std::string& value_type_str) {
    StringTrimmer trimmer(" \t\r\n");
    std::string trimmed = trimmer.trim(value_type_str);
    
    if (trimmed == "PERCENTAGE") return ValueType::PERCENTAGE;
    if (trimmed == "COUNT") return ValueType::COUNT;
    
    printf("Unknown value type: %s\n", value_type_str.c_str());
    exit(1);
    
    return ValueType::COUNT; // to quiet the compiler
}

static std::vector<std::string> parse_function_args(const std::string& func_call) {
    std::vector<std::string> args;
    size_t open_paren = func_call.find('(');
    size_t close_paren = func_call.rfind(')');
    
    if (open_paren == std::string::npos || close_paren == std::string::npos) {
        return args;
    }
    
    std::string args_str = func_call.substr(open_paren + 1, close_paren - open_paren - 1);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter comma_splitter(",");
    
    std::vector<std::string> parts = comma_splitter.split(args_str);
    for (size_t i = 0; i < parts.size(); i++) {
        std::string arg = trimmer.trim(parts[i]);
        args.push_back(arg);
    }
    
    return args;
}

static bool parse_effect(const std::string& effect_str, Effect& effect) {
    StringTrimmer trimmer(" \t\r\n");
    std::string trimmed = trimmer.trim(effect_str);
    
    if (trimmed.find("build(") == 0) {
        std::vector<std::string> args = parse_function_args(trimmed);
        if (args.size() == 4) {
            effect.type = EffectType::BUILD;
            // TODO: Resolve building name to index
            effect.data.build.building_index = 0;
            effect.data.build.scope = static_cast<u16>(static_cast<int>(parse_scope(args[1])));
            effect.data.build.auto_build = static_cast<u16>(static_cast<int>(parse_auto_build(args[2])));
            effect.data.build.no_upkeep = static_cast<u16>(static_cast<int>(parse_no_upkeep(args[3])));
            return true;
        }
    } else if (trimmed.find("researchTech(") == 0) {
        std::vector<std::string> args = parse_function_args(trimmed);
        if (args.size() == 1) {
            effect.type = EffectType::RESEARCH_TECH;
            effect.data.research_tech.tech_count = static_cast<u16>(std::atoi(args[0].c_str()));
            return true;
        }
    } else if (trimmed.find("booster(") == 0) {
        std::vector<std::string> args = parse_function_args(trimmed);
        if (args.size() == 4) {
            effect.type = EffectType::BOOSTER;
            effect.data.booster.target = static_cast<u16>(static_cast<int>(parse_target(args[0])));
            effect.data.booster.value = static_cast<u16>(std::atoi(args[1].c_str()));
            effect.data.booster.scope = static_cast<u16>(static_cast<int>(parse_scope(args[2])));
            effect.data.booster.value_type = static_cast<u16>(static_cast<int>(parse_value_type(args[3])));
            return true;
        }
    } else if (trimmed.find("train(") == 0) {
        std::vector<std::string> args = parse_function_args(trimmed);
        if (args.size() == 2) {
            effect.type = EffectType::TRAIN;
            // TODO: Resolve unit name to index
            effect.data.train.unit_index = 0;
            effect.data.train.count = static_cast<u16>(std::atoi(args[1].c_str()));
            return true;
        }
    }
    
    return false;
}

//================================================================================================================================
//=> - EffectIO implementation -
//================================================================================================================================

int EffectIO::validate_and_count () const {
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        return 0;
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    int count = 0;
    
    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() < 2) {
            continue;
        }
        for (size_t j = 1; j < parts.size(); j++) {
            std::string effect_str = trimmer.trim(parts[j]);
            if (!effect_str.empty()) {
                Effect test_effect;
                if (parse_effect(effect_str, test_effect)) {
                    count++;
                }
            }
        }
    }
    return count;
}

void EffectIO::print_content () const {
    if (effect_data_array == nullptr || effect_names_array == nullptr || effect_data_count == 0) {
        printf("No effects loaded.\n");
        return;
    }
    
    for (u32 i = 0; i < effect_data_count; i++) {
        if (effect_names_array != nullptr) {
            printf("Name: %s ", effect_names_array[i]);
        } else {
            printf("Name: NULL ");
        }
        
        const Effect& e = effect_data_array[i];
        printf("Effect: ");
        switch (e.type) {
            case EffectType::BUILD:
                printf("build(building_index=%u, scope=%u, auto_build=%u, no_upkeep=%u)",
                       e.data.build.building_index, e.data.build.scope,
                       e.data.build.auto_build, e.data.build.no_upkeep);
                break;
            case EffectType::RESEARCH_TECH:
                printf("researchTech(%u)", e.data.research_tech.tech_count);
                break;
            case EffectType::BOOSTER:
                printf("booster(target=%u, value=%u, scope=%u, value_type=%u)",
                       e.data.booster.target, e.data.booster.value,
                       e.data.booster.scope, e.data.booster.value_type);
                break;
            case EffectType::TRAIN:
                printf("train(unit_index=%u, count=%u)",
                       e.data.train.unit_index, e.data.train.count);
                break;
        }
        printf("\n");
    }
}

void EffectIO::parse_and_allocate () const {
    if (effect_data_array != nullptr) {
        return;
    }
    int count = validate_and_count();
    if (count == 0) {
        return;
    }
    effect_data_count = static_cast<u32>(count);
    effect_data_array = new Effect[effect_data_count];
    effect_names_array = new char*[effect_data_count];
    
    StringReader reader(filename);
    std::string content = reader.read();
    if (content.empty()) {
        return;
    }
    StringSplitter line_splitter("\n");
    std::vector<std::string> lines = line_splitter.split(content);
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter colon_splitter(":");
    u32 idx = 0;
    
    for (size_t i = 0; i < lines.size() && idx < effect_data_count; i++) {
        std::string line = trimmer.trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        
        std::vector<std::string> parts = colon_splitter.split(line);
        if (parts.size() < 2) {
            continue;
        }
        std::string effect_name = trimmer.trim(parts[0]);
        for (size_t j = 1; j < parts.size() && idx < effect_data_count; j++) {
            std::string effect_str = trimmer.trim(parts[j]);
            if (!effect_str.empty()) {
                if (parse_effect(effect_str, effect_data_array[idx])) {
                    size_t name_len = effect_name.length();
                    effect_names_array[idx] = new char[name_len + 1];
                    std::strncpy(effect_names_array[idx], effect_name.c_str(), name_len);
                    effect_names_array[idx][name_len] = '\0';
                    idx++;
                }
            }
        }
    }
}

//================================================================================================================================
//=> - EffectVector implementation -
//================================================================================================================================

const Effect* EffectVector::get_effect_data_array () {
    return effect_data_array;
}

const Effect& EffectVector::get_effect (u32 idx) {
    return effect_data_array[idx];
}

u32 EffectVector::get_count () {
    return effect_data_count;
}

void EffectVector::deallocate_names () {
    if (effect_names_array != nullptr) {
        for (u32 i = 0; i < effect_data_count; i++) {
            delete[] effect_names_array[i];
        }
        delete[] effect_names_array;
        effect_names_array = nullptr;
    }
}

void EffectVector::find_effects_by_name (EffectIndices* indices, const char* name) {
    for (u32 i = 0; i < MAX_EFFECTS_PER_ENTITY; i++) {
        indices->indices[i] = 0;
    }
    u32 found_count = 0;
    for (u32 i = 0; i < effect_data_count && found_count < 4; i++) {
        if (std::strcmp(effect_names_array[i], name) == 0) {
            indices->indices[found_count] = static_cast<u16>(i);
            found_count++;
            if (found_count == MAX_EFFECTS_PER_ENTITY) {
                break;
            }
        }
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
