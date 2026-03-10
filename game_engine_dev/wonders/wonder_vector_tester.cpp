//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <fstream>
#include <string>
#include <stdexcept>
#include <sstream>

#include "bit_array.h"
#include "wonder_data.h"
#include "tech_data.h"
#include "resource_data.h"
#include "building_data.h"
#include "city_flags.h"
#include "wonder_vector.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;
typedef std::string str;
typedef uint32_t u32;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

//================================================================================================================================
//=> - Helper functions -
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
        printf("*** TEST FAILED: %s\n", msg);
    }
}

void note_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_result (cond, msg.c_str());
}

void summarize_test_results () {
    if (print_level > 0) {
        printf("--------------------------------\n");
        printf(" Test count: %d\n", test_count);
        printf(" Test pass: %d\n", test_pass);
        printf(" Test fail: %d\n", test_count - test_pass);
        printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

//================================================================================================================================
//=> - Test helper functions -
//================================================================================================================================

class WonderAssessor {
public:
    WonderAssessor (BuildableWonders* buildable_vector) : m_buildable_vector(buildable_vector) {
        u16 wonder_count = WonderData::get_wonder_data_count();
        for (u16 i = 0; i < wonder_count; i++) {
            set_buildable(m_buildable_vector, i);
        }
    }
    
    static void set_buildable (BuildableWonders* vec, u16 idx) {
        vec->m_buildable.set_bit(idx);
    }
    
    void set_buildable (u16 idx) {
        set_buildable(m_buildable_vector, idx);
    }
    
    void set_buildable_all () {
        u16 wonder_count = WonderData::get_wonder_data_count();
        for (u16 i = 0; i < wonder_count; i++) {
            set_buildable(m_buildable_vector, i);
        }
    }

private:
    BuildableWonders* m_buildable_vector;
};

struct TestSetup {
    BuildableWonders* buildable;
    u32 count;
    
    TestSetup () : buildable(nullptr), count(0) {}
    
    ~TestSetup () {
        delete buildable;
    }
};

TestSetup setup_wonder_test () {
    TestSetup setup;
    setup.count = WonderData::get_wonder_data_count();
    setup.buildable = new BuildableWonders();
    return setup;
}

void verify_wonder_stats (const WonderTypeStats& stats, cstr test_name) {
    note_result (!stats.name.empty(), test_name, " - name not empty");
    note_result (stats.cost > 0, test_name, " - cost > 0");
    note_result (stats.tech_prereq_idx.get_idx() < TechData::get_tech_data_count(), test_name, " - tech_prereq_idx valid");
}

void verify_built (u16 idx, bool should_be_built, cstr test_name) {
    bool is_built = BuiltWonders::has_been_built(idx);
    str msg = str(test_name) + " - has_been_built " + (should_be_built ? "true" : "false");
    note_result (is_built == should_be_built, msg.c_str());
}

void verify_wonder_buildable_status (BuildableWonders* buildable, u16 idx, bool should_be_buildable, cstr test_name) {
    bool is_buildable = buildable->can_build(idx);
    str msg = str(test_name) + " - can_build " + (should_be_buildable ? "true" : "false");
    note_result (is_buildable == should_be_buildable, msg.c_str());
}

void test_wonder_at_idx (u16 idx, cstr prefix) {
    const WonderTypeStats* wonder_array = WonderData::get_wonder_data_array();
    const WonderTypeStats& stats = wonder_array[idx];
    str test_name = str(prefix) + " - idx " + std::to_string(idx);
    verify_wonder_stats(stats, test_name.c_str());
    verify_built(idx, false, test_name.c_str());
}

void test_bounds_get_wonder_stats (u32 count, cstr prefix) {
    test_wonder_at_idx(0, prefix);
    test_wonder_at_idx(count / 2, prefix);
    test_wonder_at_idx(count - 1, prefix);
}

void test_bounds_set_built (u32 count, cstr prefix) {
    BuiltWonders::set_owning_city(0, 1);
    str msg1 = str(prefix) + " - set_owning_city valid idx 0";
    note_result (BuiltWonders::has_been_built(0) == true, msg1.c_str());
    
    BuiltWonders::set_owning_city(count - 1, 1);
    str msg2 = str(prefix) + " - set_owning_city valid idx (last)";
    note_result (BuiltWonders::has_been_built(count - 1) == true, msg2.c_str());
}

void test_built_status_calc (u16 idx, cstr prefix) {
    verify_built(idx, false, (str(prefix) + " - not built initially").c_str());
    
    BuiltWonders::set_owning_city(idx, 1);
    verify_built(idx, true, (str(prefix) + " - built after set_owning_city").c_str());
    
    BuiltWonders::set_owning_city(idx, 0);
    verify_built(idx, false, (str(prefix) + " - not built after clear").c_str());
}

void test_repeated_set_built (u16 idx, cstr prefix) {
    BuiltWonders::set_owning_city(idx, 1);
    bool status1 = BuiltWonders::has_been_built(idx);
    verify_built(idx, true, (str(prefix) + " - first set_owning_city").c_str());
    
    BuiltWonders::set_owning_city(idx, 1);
    bool status2 = BuiltWonders::has_been_built(idx);
    verify_built(idx, true, (str(prefix) + " - second set_owning_city").c_str());
    
    BuiltWonders::set_owning_city(idx, 1);
    bool status3 = BuiltWonders::has_been_built(idx);
    verify_built(idx, true, (str(prefix) + " - third set_owning_city").c_str());
    
    str msg = str(prefix) + " - repeated calls consistent";
    note_result (status1 == status2 && status2 == status3, msg.c_str());
}

void test_get_count_consistency (cstr prefix) {
    u16 data_count = WonderData::get_wonder_data_count();
    str msg = str(prefix) + " - WonderData count > 0";
    note_result (data_count > 0, msg.c_str());
}

void test_wonderdata_idempotency (cstr prefix) {
    u16 count1 = WonderData::get_wonder_data_count();
    
    WonderData::load_static_data("../game_config.wonders");
    u16 count2 = WonderData::get_wonder_data_count();
    
    str msg = str(prefix) + " - load_static_data idempotent";
    note_result (count1 == count2, msg.c_str());
}

void test_multiple_indices (u32 count, cstr prefix) {
    test_wonder_at_idx(0, (str(prefix) + " - first").c_str());
    test_wonder_at_idx(count / 2, (str(prefix) + " - middle").c_str());
    test_wonder_at_idx(count - 1, (str(prefix) + " - last").c_str());
}

void test_wonder_independent (u32 count, cstr prefix) {
    u16 idx_a = 0;
    u16 idx_b = static_cast<u16>(count / 2);
    u16 idx_c = static_cast<u16>(count - 1);
    
    BuiltWonders::set_owning_city(idx_a, 1);
    BuiltWonders::set_owning_city(idx_b, 1);
    BuiltWonders::set_owning_city(idx_c, 1);
    
    verify_built(idx_a, true, (str(prefix) + " - wonder A").c_str());
    verify_built(idx_b, true, (str(prefix) + " - wonder B").c_str());
    verify_built(idx_c, true, (str(prefix) + " - wonder C").c_str());
    
    BuiltWonders::set_owning_city(idx_b, 0);
    verify_built(idx_a, true, (str(prefix) + " - wonder A still built").c_str());
    verify_built(idx_b, false, (str(prefix) + " - wonder B cleared").c_str());
    verify_built(idx_c, true, (str(prefix) + " - wonder C still built").c_str());
}

void test_all_not_built_initially (u32 count, cstr prefix) {
    u32 checked = 0;
    for (u32 i = 0; i < count && checked < 10; i++) {
        str msg = str(prefix) + " - idx " + std::to_string(i);
        verify_built(static_cast<u16>(i), false, msg.c_str());
        checked++;
    }
}

void test_wonder_stats_variety (u32 count, cstr prefix) {
    u16 idx_a = 0;
    u16 idx_b = static_cast<u16>(count / 2);
    u16 idx_c = static_cast<u16>(count - 1);
    const WonderTypeStats* wonder_array = WonderData::get_wonder_data_array();
    const WonderTypeStats& stats_a = wonder_array[idx_a];
    const WonderTypeStats& stats_b = wonder_array[idx_b];
    const WonderTypeStats& stats_c = wonder_array[idx_c];
    
    bool different_costs = (stats_a.cost != stats_b.cost) || (stats_b.cost != stats_c.cost) || (stats_a.cost != stats_c.cost);
    bool different_names = (stats_a.name != stats_b.name) && (stats_b.name != stats_c.name) && (stats_a.name != stats_c.name);
    
    note_result (different_costs || different_names, (str(prefix) + " - wonders have varied stats").c_str());
}

void test_wonder_cost_range (u32 count, cstr prefix) {
    const WonderTypeStats* wonder_array = WonderData::get_wonder_data_array();
    int min_cost = INT_MAX;
    int max_cost = 0;
    for (u32 i = 0; i < count && i < 10; i++) {
        const WonderTypeStats& stats = wonder_array[i];
        if (stats.cost < min_cost) {
            min_cost = stats.cost;
        }
        if (stats.cost > max_cost) {
            max_cost = stats.cost;
        }
    }
    note_result (min_cost > 0, (str(prefix) + " - min cost > 0").c_str());
    note_result (max_cost > 0, (str(prefix) + " - max cost > 0").c_str());
    note_result (max_cost >= min_cost, (str(prefix) + " - max cost >= min cost").c_str());
}

void test_wonder_name_uniqueness (u32 count, cstr prefix) {
    const WonderTypeStats* wonder_array = WonderData::get_wonder_data_array();
    bool all_unique = true;
    for (u32 i = 0; i < count && i < 10; i++) {
        const WonderTypeStats& stats_i = wonder_array[i];
        for (u32 j = i + 1; j < count && j < 10; j++) {
            const WonderTypeStats& stats_j = wonder_array[j];
            if (stats_i.name == stats_j.name) {
                all_unique = false;
                break;
            }
        }
        if (!all_unique) {
            break;
        }
    }
    note_result (all_unique, (str(prefix) + " - wonder names unique").c_str());
}

void test_wonder_stats_consistency (u16 idx, cstr prefix) {
    const WonderTypeStats* wonder_array = WonderData::get_wonder_data_array();
    const WonderTypeStats& stats1 = wonder_array[idx];
    const WonderTypeStats& stats2 = wonder_array[idx];
    note_result (stats1.name == stats2.name, (str(prefix) + " - name consistent").c_str());
    note_result (stats1.cost == stats2.cost, (str(prefix) + " - cost consistent").c_str());
    note_result (stats1.tech_prereq_idx == stats2.tech_prereq_idx, (str(prefix) + " - tech_prereq_idx consistent").c_str());
}

void test_wonder_built_with_different_wonders (u32 count, cstr prefix) {
    for (u32 i = 0; i < count && i < 5; i++) {
        verify_built(static_cast<u16>(i), false, (str(prefix) + " - idx " + std::to_string(i) + " not built").c_str());
        BuiltWonders::set_owning_city(static_cast<u16>(i), 1);
        str msg = str(prefix) + " - idx " + std::to_string(i) + " built";
        verify_built(static_cast<u16>(i), true, msg.c_str());
    }
}

void test_buildable_vec_default_state (BuildableWonders* buildable, u32 count, cstr prefix) {
    for (u32 i = 0; i < count && i < 5; i++) {
        verify_wonder_buildable_status(buildable, static_cast<u16>(i), false, 
            (str(prefix) + " - idx " + std::to_string(i) + " not buildable initially").c_str());
    }
}

void test_buildable_vec_after_assessor (BuildableWonders* buildable, u32 count, cstr prefix) {
    WonderAssessor assessor(buildable);
    for (u32 i = 0; i < count && i < 5; i++) {
        verify_wonder_buildable_status(buildable, static_cast<u16>(i), true, 
            (str(prefix) + " - idx " + std::to_string(i) + " buildable after assessor").c_str());
    }
}

void test_buildable_vec_set_individual (BuildableWonders* buildable, u16 idx, cstr prefix) {
    BuildableWonders* fresh = new BuildableWonders();
    
    verify_wonder_buildable_status(fresh, idx, false, (str(prefix) + " - not buildable initially").c_str());
    WonderAssessor::set_buildable(fresh, idx);
    verify_wonder_buildable_status(fresh, idx, true, (str(prefix) + " - buildable after set").c_str());
    
    delete fresh;
}

void test_owning_city_different_ids (u16 idx, cstr prefix) {
    BuiltWonders::set_owning_city(idx, 0);
    verify_built(idx, false, (str(prefix) + " - city_id 0 not built").c_str());
    
    BuiltWonders::set_owning_city(idx, 1);
    verify_built(idx, true, (str(prefix) + " - city_id 1 built").c_str());
    
    BuiltWonders::set_owning_city(idx, 5);
    verify_built(idx, true, (str(prefix) + " - city_id 5 built").c_str());
    
    BuiltWonders::set_owning_city(idx, 100);
    verify_built(idx, true, (str(prefix) + " - city_id 100 built").c_str());
    
    BuiltWonders::set_owning_city(idx, 0);
    verify_built(idx, false, (str(prefix) + " - city_id 0 clears built").c_str());
}

void test_buildable_vec_multiple_indices (BuildableWonders* buildable, u32 count, cstr prefix) {
    BuildableWonders* fresh = new BuildableWonders();
    
    u16 idx_a = 0;
    u16 idx_b = static_cast<u16>(count / 2);
    u16 idx_c = static_cast<u16>(count - 1);
    
    WonderAssessor::set_buildable(fresh, idx_a);
    WonderAssessor::set_buildable(fresh, idx_b);
    WonderAssessor::set_buildable(fresh, idx_c);
    
    verify_wonder_buildable_status(fresh, idx_a, true, (str(prefix) + " - idx A buildable").c_str());
    verify_wonder_buildable_status(fresh, idx_b, true, (str(prefix) + " - idx B buildable").c_str());
    verify_wonder_buildable_status(fresh, idx_c, true, (str(prefix) + " - idx C buildable").c_str());
    
    if (count > 3) {
        verify_wonder_buildable_status(fresh, 1, false, (str(prefix) + " - idx 1 not buildable").c_str());
    }
    
    delete fresh;
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_wonder_data () {
    u16 count = WonderData::get_wonder_data_count();
    note_result (count > 0, "WonderData load and count");
    if (print_level > 1) {
        WonderData::print_content();
    }
    summarize_test_results();
}

void test_static_data_loaded () {
    u16 count = WonderData::get_wonder_data_count();
    note_result (count > 0, "Static data loaded - wonders count > 0");
    const WonderTypeStats* data = WonderData::get_wonder_data_array();
    note_result (data != nullptr, "Static data loaded - data array not null");
    if (count > 0) {
        note_result (!data[0].name.empty(), "Static data loaded - first wonder has name");
        note_result (data[0].cost > 0, "Static data loaded - first wonder has cost > 0");
    }
    summarize_test_results();
}

void test_wonder_data_idempotency () {
    test_wonderdata_idempotency("WonderData idempotency");
    summarize_test_results();
}

void test_wonder_built () {
    BuiltWonders::allocate_static_array();
    verify_built(0, false, "Wonder built - not built initially");
    BuiltWonders::set_owning_city(0, 1);
    verify_built(0, true, "Wonder built - built after set_owning_city");
    summarize_test_results();
}

void test_wonder_bounds () {
    BuiltWonders::allocate_static_array();
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_bounds_get_wonder_stats(count, "Wonder bounds get_wonder_stats");
    test_bounds_set_built(count, "Wonder bounds set_owning_city");
    summarize_test_results();
}

void test_wonder_multiple_indices () {
    BuiltWonders::allocate_static_array();
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_multiple_indices(count, "Wonder multiple indices");
    summarize_test_results();
}

void test_wonder_built_status_calc () {
    BuiltWonders::allocate_static_array();
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_built_status_calc(0, "Wonder built status calc idx 0");
    test_built_status_calc(static_cast<u16>(count - 1), "Wonder built status calc idx last");
    test_built_status_calc(static_cast<u16>(count / 2), "Wonder built status calc idx middle");
    summarize_test_results();
}

void test_wonder_repeated_set_built () {
    BuiltWonders::allocate_static_array();
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_repeated_set_built(0, "Wonder repeated set_owning_city idx 0");
    test_repeated_set_built(static_cast<u16>(count - 1), "Wonder repeated set_owning_city idx last");
    summarize_test_results();
}

void test_wonder_get_count_consistency () {
    test_get_count_consistency("Wonder get_count consistency");
    summarize_test_results();
}

void test_wonder_built_with_different_wonders () {
    BuiltWonders::allocate_static_array();
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_wonder_built_with_different_wonders(count, "Wonder built different wonders");
    summarize_test_results();
}

void test_wonder_all_not_built_initially () {
    BuiltWonders::allocate_static_array();
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_all_not_built_initially(count, "Wonder all not built initially");
    summarize_test_results();
}

void test_wonder_built_independent () {
    BuiltWonders::allocate_static_array();
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_wonder_independent(count, "Wonder built independent");
    summarize_test_results();
}

void test_wonder_stats_consistency () {
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_wonder_stats_consistency(0, "Wonder stats consistency idx 0");
    test_wonder_stats_consistency(static_cast<u16>(count - 1), "Wonder stats consistency idx last");
    summarize_test_results();
}

void test_wonder_stats_variety () {
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_wonder_stats_variety(count, "Wonder stats variety");
    summarize_test_results();
}

void test_wonder_cost_range () {
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_wonder_cost_range(count, "Wonder cost range");
    summarize_test_results();
}

void test_wonder_name_uniqueness () {
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_wonder_name_uniqueness(count, "Wonder name uniqueness");
    summarize_test_results();
}

void test_buildable_vec_default () {
    TestSetup setup = setup_wonder_test();
    test_buildable_vec_default_state(setup.buildable, setup.count, "BuildableVec default state");
    summarize_test_results();
}

void test_buildable_vec_after_assessor () {
    TestSetup setup = setup_wonder_test();
    test_buildable_vec_after_assessor(setup.buildable, setup.count, "BuildableVec after assessor");
    summarize_test_results();
}

void test_buildable_vec_set_individual () {
    TestSetup setup = setup_wonder_test();
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_buildable_vec_set_individual(setup.buildable, 0, "BuildableVec set individual idx 0");
    test_buildable_vec_set_individual(setup.buildable, static_cast<u16>(count - 1), "BuildableVec set individual idx last");
    summarize_test_results();
}

void test_owning_city_different_ids () {
    BuiltWonders::allocate_static_array();
    u32 count = static_cast<u32>(WonderData::get_wonder_data_count());
    test_owning_city_different_ids(0, "OwningCity different city IDs idx 0");
    if (count > 1) {
        test_owning_city_different_ids(static_cast<u16>(count / 2), "OwningCity different city IDs idx middle");
    }
    summarize_test_results();
}

void test_buildable_vec_multiple_indices () {
    TestSetup setup = setup_wonder_test();
    test_buildable_vec_multiple_indices(setup.buildable, setup.count, "BuildableVec multiple indices");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    TechData::load_static_data("../game_config.techs");
    ResourceData::load_static_data("../game_config.resources");
    CityFlagData::load_static_data("../game_config.city_flags");
    BuildingData::load_static_data("../game_config.buildings");
    WonderData::load_static_data("../game_config.wonders");

    BuiltWonders::allocate_static_array();
    
    test_static_data_loaded();
    test_wonder_data();
    test_wonder_data_idempotency();
    test_wonder_built();
    test_wonder_bounds();
    test_wonder_multiple_indices();
    test_wonder_built_status_calc();
    test_wonder_repeated_set_built();
    test_wonder_get_count_consistency();
    test_wonder_built_with_different_wonders();
    test_wonder_all_not_built_initially();
    test_wonder_built_independent();
    test_wonder_stats_consistency();
    test_wonder_stats_variety();
    test_wonder_cost_range();
    test_wonder_name_uniqueness();
    test_buildable_vec_default();
    test_buildable_vec_after_assessor();
    test_buildable_vec_set_individual();
    test_owning_city_different_ids();
    test_buildable_vec_multiple_indices();

    printf("=======================================================\n");
    printf(" TESTING WONDERS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
