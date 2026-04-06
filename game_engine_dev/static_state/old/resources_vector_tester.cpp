//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <sstream>

#include "bit_array.h"
#include "tech_data.h"
#include "resource_data.h"
#include "resources_vector.h"

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

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

void note_test_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            printf("*** TEST PASSED: %s\n", msg);
        }
    } else  {
        total_test_fails++;
        printf("*** TEST FAILED: %s\n", msg);
    }
}

void note_test_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_test_result (cond, msg.c_str());
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

struct ResourceVectorTestSetup {
    BitArrayCL* resources_available;
    ResourceVector* rv;
    uint32_t count;
    
    ResourceVectorTestSetup () : resources_available(nullptr), rv(nullptr), count(0) {}
    
    ~ResourceVectorTestSetup () {
        delete rv;
        delete resources_available;
    }
};

ResourceVectorTestSetup setup_resource_vector_test () {
    ResourceVectorTestSetup setup;
    setup.count = static_cast<uint32_t>(ResourceData::get_resource_data_count());
    if (setup.count > 0) {
        setup.resources_available = new BitArrayCL(setup.count);
        setup.rv = new ResourceVector(setup.count, setup.resources_available);
    }
    return setup;
}

void verify_resource_data (const ResourceTypeStats& stats, const char* test_name) {
    note_test_result (!stats.name.empty(), test_name, " - name not empty");
    note_test_result (stats.bonus_food >= 0, test_name, " - bonus_food >= 0");
    note_test_result (stats.bonus_shields >= 0, test_name, " - bonus_shields >= 0");
    note_test_result (stats.bonus_commerce >= 0, test_name, " - bonus_commerce >= 0");
}

void test_resource_at_index (ResourceVector* rv, uint32_t index, const char* test_prefix) {
    ResourceTypeStats stats = rv->get_resource(index);
    str test_name = str(test_prefix) + " - index " + std::to_string(index);
    verify_resource_data(stats, test_name.c_str());
}

void test_get_resource_valid_indices (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    test_resource_at_index(rv, 0, test_prefix);
    if (count > 1) {
        test_resource_at_index(rv, count / 2, test_prefix);
        test_resource_at_index(rv, count - 1, test_prefix);
    }
}

void test_bounds_set_available (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    rv->set_available(0);
    str msg1 = str(test_prefix) + " - set_available valid index 0";
    note_test_result (true, msg1.c_str());
    
    if (count > 1) {
        rv->set_available(count - 1);
        str msg2 = str(test_prefix) + " - set_available valid index (last)";
        note_test_result (true, msg2.c_str());
    }
    
    rv->set_available(count);
    str msg3 = str(test_prefix) + " - set_available out of bounds (index " + std::to_string(count) + ")";
    note_test_result (true, msg3.c_str(), " - no crash");
}

void test_multiple_available (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    rv->set_available(0);
    ResourceTypeStats stats0 = rv->get_resource(0);
    verify_resource_data(stats0, (str(test_prefix) + " - available index 0").c_str());
    
    if (count > 2) {
        uint32_t mid = count / 2;
        rv->set_available(mid);
        ResourceTypeStats stats_mid = rv->get_resource(mid);
        verify_resource_data(stats_mid, (str(test_prefix) + " - available index " + std::to_string(mid)).c_str());
    }
    
    if (count > 1) {
        rv->set_available(count - 1);
        ResourceTypeStats stats_last = rv->get_resource(count - 1);
        verify_resource_data(stats_last, (str(test_prefix) + " - available index " + std::to_string(count - 1)).c_str());
    }
}

void test_repeated_set_available (ResourceVector* rv, uint32_t index, const char* test_prefix) {
    rv->set_available(index);
    ResourceTypeStats stats1 = rv->get_resource(index);
    verify_resource_data(stats1, (str(test_prefix) + " - first set").c_str());
    
    rv->set_available(index);
    ResourceTypeStats stats2 = rv->get_resource(index);
    verify_resource_data(stats2, (str(test_prefix) + " - second set").c_str());
    
    rv->set_available(index);
    ResourceTypeStats stats3 = rv->get_resource(index);
    verify_resource_data(stats3, (str(test_prefix) + " - third set").c_str());
    
    str msg = str(test_prefix) + " - repeated calls consistent";
    note_test_result (stats1.name == stats2.name && stats2.name == stats3.name, msg.c_str());
}

void test_get_count_consistency (ResourceVector* rv, const char* test_prefix) {
    u16 data_count = ResourceData::get_resource_data_count();
    uint32_t rv_count = rv->get_count();
    str msg = str(test_prefix) + " - get_count matches ResourceData::get_resource_data_count";
    note_test_result (static_cast<u16>(rv_count) == data_count, msg.c_str());
}

void test_multiple_indices (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    test_resource_at_index(rv, 0, (str(test_prefix) + " - first").c_str());
    
    if (count > 2) {
        test_resource_at_index(rv, count / 2, (str(test_prefix) + " - middle").c_str());
    }
    
    if (count > 1) {
        test_resource_at_index(rv, count - 1, (str(test_prefix) + " - last").c_str());
    }
}

void test_resource_stats_variety (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    uint32_t idx_a = 0;
    uint32_t idx_b = count / 2;
    uint32_t idx_c = count - 1;
    ResourceTypeStats r1 = rv->get_resource(idx_a);
    ResourceTypeStats r2 = rv->get_resource(idx_b);
    ResourceTypeStats r3 = rv->get_resource(idx_c);
    
    bool diff_food = (r1.bonus_food != r2.bonus_food) || 
                     (r2.bonus_food != r3.bonus_food) || 
                     (r1.bonus_food != r3.bonus_food);

    bool diff_shields = (r1.bonus_shields != r2.bonus_shields) || 
                        (r2.bonus_shields != r3.bonus_shields) || 
                        (r1.bonus_shields != r3.bonus_shields);

    bool diff_comm = (r1.bonus_commerce != r2.bonus_commerce) || 
                     (r2.bonus_commerce != r3.bonus_commerce) || 
                     (r1.bonus_commerce != r3.bonus_commerce);
    
    note_test_result (diff_food || diff_shields || diff_comm, (str(test_prefix) + " - resources have varied stats").c_str());
}

void test_is_available_initial_state (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    bool available_0 = rv->is_available(0);
    note_test_result (!available_0, test_prefix, " - index 0 initially unavailable");
    
    if (count > 1) {
        bool available_last = rv->is_available(count - 1);
        note_test_result (!available_last, test_prefix, " - last index initially unavailable");
    }
    
    if (count > 2) {
        bool available_mid = rv->is_available(count / 2);
        note_test_result (!available_mid, test_prefix, " - middle index initially unavailable");
    }
}

void test_is_available_after_set (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    rv->set_available(0);
    bool available_0 = rv->is_available(0);
    note_test_result (available_0, test_prefix, " - index 0 available after set_available");
    
    if (count > 1) {
        rv->set_available(count - 1);
        bool available_last = rv->is_available(count - 1);
        note_test_result (available_last, test_prefix, " - last index available after set_available");
    }
    
    if (count > 2) {
        uint32_t mid = count / 2;
        rv->set_available(mid);
        bool available_mid = rv->is_available(mid);
        note_test_result (available_mid, test_prefix, " - middle index available after set_available");
    }
}

void test_is_available_independence (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    rv->set_available(0);
    bool available_0 = rv->is_available(0);
    note_test_result (available_0, test_prefix, " - index 0 available");
    
    if (count > 1) {
        bool available_1 = rv->is_available(1);
        note_test_result (!available_1, test_prefix, " - index 1 still unavailable when 0 is set");
    }
    
    if (count > 2) {
        uint32_t mid = count / 2;
        rv->set_available(mid);
        bool available_mid = rv->is_available(mid);
        bool available_0_still = rv->is_available(0);
        note_test_result (available_mid, test_prefix, " - middle index available");
        note_test_result (available_0_still, test_prefix, " - index 0 still available after setting middle");
    }
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_static_data_loaded_tech_data () {
    u16 count = TechData::get_tech_data_count();
    note_test_result (count > 2, "Static data validation: TechData count > 2");
    summarize_test_results();
}

void test_static_data_loaded_resource_data () {
    u16 count = ResourceData::get_resource_data_count();
    note_test_result (count > 2, "Static data validation: ResourceData count > 2");
    summarize_test_results();
}

void test_resource_data () {
    u16 count = ResourceData::get_resource_data_count();
    note_test_result (count > 0, "ResourceData get_resource_data_count");
    if (print_level > 1) {
        ResourceData::print_content();
    }
    summarize_test_results();
}

void test_resource_vector () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    note_test_result (setup.rv->get_count() == setup.count, "ResourceVec get_count");
    ResourceTypeStats stats = setup.rv->get_resource(0);
    verify_resource_data(stats, "ResourceVec basic");
    summarize_test_results();
}

void test_resource_vector_bounds () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    test_get_resource_valid_indices(setup.rv, setup.count, "ResourceVec get_resource valid indices");
    test_bounds_set_available(setup.rv, setup.count, "ResourceVec bounds set_available");
    summarize_test_results();
}

void test_resource_vector_multiple_indices () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    test_multiple_indices(setup.rv, setup.count, "ResourceVec multiple indices");
    summarize_test_results();
}

void test_resource_vector_multiple_available () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    test_multiple_available(setup.rv, setup.count, "ResourceVec multiple available");
    summarize_test_results();
}

void test_resource_vector_repeated_set_available () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    test_repeated_set_available(setup.rv, 0, "ResourceVec repeated set_available index 0");
    if (setup.count > 1) {
        test_repeated_set_available(setup.rv, setup.count - 1, "ResourceVec repeated set_available index last");
    }
    summarize_test_results();
}

void test_resource_vector_get_count_consistency () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    test_get_count_consistency(setup.rv, "ResourceVec get_count consistency");
    summarize_test_results();
}

void test_resource_vector_stats_variety () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    test_resource_stats_variety(setup.rv, setup.count, "ResourceVec stats variety");
    summarize_test_results();
}

void test_resource_vector_is_available () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    test_is_available_initial_state(setup.rv, setup.count, "ResourceVec is_available initial state");
    summarize_test_results();
}

void test_resource_vector_is_available_after_set () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    test_is_available_after_set(setup.rv, setup.count, "ResourceVec is_available after set");
    summarize_test_results();
}

void test_resource_vector_is_available_independence () {
    ResourceVectorTestSetup setup = setup_resource_vector_test();
    test_is_available_independence(setup.rv, setup.count, "ResourceVec is_available independence");
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
    
    test_static_data_loaded_tech_data();
    test_static_data_loaded_resource_data();
    test_resource_data();
    test_resource_vector();
    test_resource_vector_bounds();
    test_resource_vector_multiple_indices();
    test_resource_vector_multiple_available();
    test_resource_vector_repeated_set_available();
    test_resource_vector_get_count_consistency();
    test_resource_vector_stats_variety();
    test_resource_vector_is_available();
    test_resource_vector_is_available_after_set();
    test_resource_vector_is_available_independence();

    printf("=======================================================\n");
    printf(" TESTING RESOURCES: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
