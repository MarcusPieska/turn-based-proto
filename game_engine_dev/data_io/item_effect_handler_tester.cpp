//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "data_reader.h"
#include "item_effect_handler.h"
#include "item_effect_helpers.h"
#include "name_to_idx_callbacks.h"
#include "path_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;
typedef std::string str;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

static u32 g_name_lookup_errors = 0;

static const std::vector<RawItem>* g_building_items = nullptr;
static const std::vector<RawItem>* g_unit_items = nullptr;

//================================================================================================================================
//=> - Name resolution (same rules as DataParserBase::name_to_idx / idx_to_name on RawItem lists) -
//================================================================================================================================

static u16 raw_items_name_to_idx (const std::vector<RawItem>& items, const std::string& name) {
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].name == name) {
            return static_cast<u16>(i);
        }
    }
    printf("ERROR: could not map name '%s' to index\n", name.c_str());
    ++g_name_lookup_errors;
    return 0;
}

static std::string raw_items_idx_to_name (const std::vector<RawItem>& items, u16 idx) {
    if (idx >= items.size()) {
        printf("ERROR: could not map index '%u' to name\n", static_cast<unsigned int>(idx));
        ++g_name_lookup_errors;
        return "";
    }
    return items[idx].name;
}

//================================================================================================================================
//=> - Name-to-index callbacks for ItemEffectHandler -
//================================================================================================================================

u16 cb_building_name_to_idx (cstr name) {
    return raw_items_name_to_idx(*g_building_items, name);
}

u16 cb_unit_name_to_idx (cstr name) {
    return raw_items_name_to_idx(*g_unit_items, name);
}

//================================================================================================================================
//=> - Test helpers (logging, CLI) -
//================================================================================================================================

void note_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        if (print_level > 0) {
            printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void note_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_result(cond, msg.c_str());
}

void summarize_test_results () {
    if (print_level > 0) {
        printf("--------------------------------\n");
        printf(" Test count: %d\n", test_count);
        printf(" Test pass: %d\n", test_pass);
        printf(" Test fail: %d\n", test_count - test_pass);
        printf("--------------------------------\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

//================================================================================================================================
//=> - Readable printing for parsed effects -
//================================================================================================================================

void print_item_effects_readable (
    cstr item_name,
    const ItemEffectsStruct& effects,
    const std::vector<RawItem>& building_items,
    const std::vector<RawItem>& unit_items) {
    printf("*** %s\n", item_name);
    for (u32 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        const ItemEffectStruct& e = effects.items[j];
        const ItemEffectType t = static_cast<ItemEffectType>(e.type);
        if (t == ItemEffectType::NONE) {
            continue;
        }
        printf("  [%u] %s\n", j, ItemEffectHelper::type_enum_to_str(t).c_str());
        switch (t) {
        case ItemEffectType::BOOSTER:
            printf("      target: %s\n", ItemEffectHelper::booster_type_enum_to_str(e.effect.booster.target_id).c_str());
            printf("      amount: %d\n", static_cast<int>(e.effect.booster.amount));
            printf("      scope:  %s\n", ItemEffectHelper::effects_scope_enum_to_str(e.effect.booster.scope).c_str());
            printf("      mode:   %s\n", ItemEffectHelper::amount_mode_enum_to_str(e.effect.booster.amount_mode).c_str());
            break;
        case ItemEffectType::BUILD: {
            std::string bname = raw_items_idx_to_name(building_items, e.effect.build.building_id);
            printf("      building: %s (id %u)\n", bname.c_str(), e.effect.build.building_id);
            printf("      scope:    %s\n", ItemEffectHelper::effects_scope_enum_to_str(e.effect.build.scope).c_str());
            printf("      build:    %s\n", ItemEffectHelper::build_mode_enum_to_str(e.effect.build.build_mode).c_str());
            printf("      upkeep:   %s\n", ItemEffectHelper::upkeep_mode_enum_to_str(e.effect.build.upkeep_mode).c_str());
            break;
        }
        case ItemEffectType::ENABLE:
            printf("      feature_id: %u\n", e.effect.enable.feature_id);
            printf("      scope:      %s\n", ItemEffectHelper::effects_scope_enum_to_str(e.effect.enable.scope).c_str());
            break;
        case ItemEffectType::RESEARCH_TECH:
            printf("      tech_count: %u\n", e.effect.research_tech.tech_count);
            break;
        case ItemEffectType::TRAIN: {
            std::string uname = raw_items_idx_to_name(unit_items, e.effect.train.unit_id);
            printf("      unit:     %s (id %u)\n", uname.c_str(), e.effect.train.unit_id);
            printf("      interval: %u turns\n", e.effect.train.turns_interval);
            break;
        }
        default:
            printf("      (no detail printer for this type)\n");
            break;
        }
    }
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_parse_each_game_config_effects_line () {
    PathMng paths("../");
    DataReader building_reader(paths.get_path_to_buildings());
    DataReader unit_reader(paths.get_path_to_units());
    DataReader effect_reader(paths.get_path_to_effects());

    g_building_items = &building_reader.get_raw_items();
    g_unit_items = &unit_reader.get_raw_items();

    NameToIdxCbs cbs = {};
    cbs.building_name_to_idx = cb_building_name_to_idx;
    cbs.unit_name_to_idx = cb_unit_name_to_idx;
    cbs.effect_name_to_idx = nullptr;

    ItemEffectHandler handler(&cbs);

    const std::vector<RawItem>& rows = effect_reader.get_raw_items();
    for (size_t i = 0; i < rows.size(); ++i) {
        g_name_lookup_errors = 0;
        handler.reset_error_count();

        const ItemEffectsStruct parsed = handler.parse_effects_line(rows[i].raw_line);
        const u32 handler_errors = handler.get_error_count();
        const u32 lookup_errors_after = g_name_lookup_errors;
        const bool parse_ok = (handler_errors == 0);

        str msg = str("effects line ") + std::to_string(i + 1) + ": " + rows[i].name;
        note_result(parse_ok, msg.c_str());

        if (lookup_errors_after > 0 && print_level >= 1) {
            printf("    (note) %u name lookup issue(s) on this line — check buildings/units config strings\n",
                lookup_errors_after);
        }

        if (print_level >= 2) {
            print_item_effects_readable(rows[i].name.c_str(), parsed, *g_building_items, *g_unit_items);
        } else if (!parse_ok && print_level >= 1) {
            printf("    (raw) %s\n", rows[i].raw_line.c_str());
        }
    }

    g_building_items = nullptr;
    g_unit_items = nullptr;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_parse_each_game_config_effects_line();
    summarize_test_results();

    printf("=======================================================\n");
    printf(" ITEM EFFECT HANDLER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
