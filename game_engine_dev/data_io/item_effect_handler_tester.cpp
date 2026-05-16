//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "item_effect_handler.h"
#include "item_effect_helpers.h"
#include "name_to_idx_callbacks.h"
#include "opt_str_mng.h"
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

static const StringManager* g_building_names = nullptr;
static const StringManager* g_unit_names = nullptr;

//================================================================================================================================
//=> - Name resolution (same rules as DataParserBase::name_to_idx / idx_to_name on RawItem lists) -
//================================================================================================================================

static std::string trim_copy(cstr in) {
    if (!in) return "";
    std::string s(in);
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
    return s.substr(b, e - b);
}

static u16 raw_items_name_to_idx (const StringManager& items, cstr name) {
    const std::string target = trim_copy(name);
    for (u32 i = 0; i < items.get_string_count(); ++i) {
        if (trim_copy(items.get_string_content(i)) == target) {
            return static_cast<u16>(i);
        }
    }
    printf("ERROR: could not map name '%s' to index\n", name ? name : "");
    ++g_name_lookup_errors;
    return 0;
}

static std::string raw_items_idx_to_name (const StringManager& items, u16 idx) {
    if (idx >= items.get_string_count()) {
        printf("ERROR: could not map index '%u' to name\n", static_cast<unsigned int>(idx));
        ++g_name_lookup_errors;
        return "";
    }
    return items.get_string_content(idx);
}

static void derive_names_from_lines(const StringManager& lines, StringManager& names) {
    u32 total = 0;
    for (u32 i = 0; i < lines.get_string_count(); ++i) {
        StringManager p;
        p.load_cstr_content(lines.get_string_content(i));
        p.split_string_by_char(0, ':');
        p.trim_head_char(0, ' ');
        p.trim_tail_char(0, ' ');
        p.trim_head_char(0, '\t');
        p.trim_tail_char(0, '\t');
        p.trim_head_char(0, '\r');
        p.trim_tail_char(0, '\r');
        total += (u32)std::strlen(p.get_string_content(0)) + 1;
    }
    char* b = new char[(std::size_t)total];
    u32 k = 0;
    for (u32 i = 0; i < lines.get_string_count(); ++i) {
        StringManager p;
        p.load_cstr_content(lines.get_string_content(i));
        p.split_string_by_char(0, ':');
        p.trim_head_char(0, ' ');
        p.trim_tail_char(0, ' ');
        p.trim_head_char(0, '\t');
        p.trim_tail_char(0, '\t');
        p.trim_head_char(0, '\r');
        p.trim_tail_char(0, '\r');
        const cstr n = p.get_string_content(0);
        std::size_t len = std::strlen(n);
        std::memcpy(b + k, n, len);
        k += (u32)len;
        b[k++] = '\n';
    }
    if (k > 0) --k;
    b[k] = '\0';
    names.load_cstr_content(b);
    names.split_string_by_char(0, '\n');
    names.cull_empty_strings();
    delete[] b;
}

//================================================================================================================================
//=> - Name-to-index callbacks for ItemEffectHandler -
//================================================================================================================================

u16 cb_building_name_to_idx (cstr name) {
    return raw_items_name_to_idx(*g_building_names, name);
}

u16 cb_unit_name_to_idx (cstr name) {
    return raw_items_name_to_idx(*g_unit_names, name);
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
    const StringManager& building_items,
    const StringManager& unit_items) {
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
    StringManager building_lines;
    StringManager unit_lines;
    StringManager effect_lines;
    StringManager building_names;
    StringManager unit_names;

    building_lines.load_file_content(paths.get_path_to_buildings().c_str());
    building_lines.split_string_by_char(0, '\n');
    building_lines.cull_empty_strings();
    unit_lines.load_file_content(paths.get_path_to_units().c_str());
    unit_lines.split_string_by_char(0, '\n');
    unit_lines.cull_empty_strings();
    effect_lines.load_file_content(paths.get_path_to_effects().c_str());
    effect_lines.split_string_by_char(0, '\n');
    effect_lines.cull_empty_strings();

    derive_names_from_lines(building_lines, building_names);
    derive_names_from_lines(unit_lines, unit_names);

    g_building_names = &building_names;
    g_unit_names = &unit_names;

    NameToIdxCbs cbs = {};
    cbs.building_name_to_idx = cb_building_name_to_idx;
    cbs.unit_name_to_idx = cb_unit_name_to_idx;
    cbs.effect_name_to_idx = nullptr;

    ItemEffectHandler handler(&cbs);

    for (u32 i = 0; i < effect_lines.get_string_count(); ++i) {
        g_name_lookup_errors = 0;
        handler.reset_error_count();
        StringManager line_items;
        line_items.load_cstr_content(effect_lines.get_string_content(i));
        line_items.split_string_by_char(0, ':');
        for (u32 t = 0; t < line_items.get_string_count(); ++t) {
            line_items.trim_head_char(t, ' ');
            line_items.trim_tail_char(t, ' ');
            line_items.trim_head_char(t, '\t');
            line_items.trim_tail_char(t, '\t');
            line_items.trim_head_char(t, '\r');
            line_items.trim_tail_char(t, '\r');
            line_items.trim_head_char(t, '\n');
            line_items.trim_tail_char(t, '\n');
        }
        line_items.cull_empty_strings();
        const ItemEffectsStruct parsed = handler.parse_effects_line(line_items);
        const u32 handler_errors = handler.get_error_count();
        const u32 lookup_errors_after = g_name_lookup_errors;
        const bool parse_ok = (handler_errors == 0);

        str msg = str("effects line ") + std::to_string(i + 1) + ": " + line_items.get_string_content(0);
        note_result(parse_ok, msg.c_str());

        if (lookup_errors_after > 0 && print_level >= 1) {
            printf("    (note) %u name lookup issue(s) on this line — check buildings/units config strings\n",
                lookup_errors_after);
        }

        if (print_level >= 2) {
            print_item_effects_readable(line_items.get_string_content(0), parsed, *g_building_names, *g_unit_names);
        } else if (!parse_ok && print_level >= 1) {
            printf("    (raw) %s\n", effect_lines.get_string_content(i));
        }
    }

    g_building_names = nullptr;
    g_unit_names = nullptr;
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
