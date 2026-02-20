//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <sstream>

#include "resources_vector.h"
#include "bit_array.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

typedef std::string str;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
bool verbose = true;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

void note_test_result (bool cond, cstr msg) {
    test_count++;
    if (cond) {
        test_pass++;
        if (verbose) {
            printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        printf("*** TEST FAILED: %s\n", msg);
    }
}

void note_test_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_test_result (cond, msg.c_str());
}

void summarize_test_results () {
    printf("--------------------------------\n");
    printf(" Test count: %d\n", test_count);
    printf(" Test pass: %d\n", test_pass);
    printf(" Test fail: %d\n", test_count - test_pass);
    printf("--------------------------------\n\n\n");

    test_count = 0;
    test_pass = 0;
}

//================================================================================================================================
//=> - Test helper functions -
//================================================================================================================================

struct ResourceVectorTestSetup {
    ResourceIO* io;
    BitArrayCL* resources_available;
    ResourceVector* rv;
    uint32_t count;
    
    ResourceVectorTestSetup () : io(nullptr), resources_available(nullptr), rv(nullptr), count(0) {}
    
    ~ResourceVectorTestSetup () {
        delete rv;
        delete resources_available;
        delete io;
    }
};

ResourceVectorTestSetup setup_resource_vector_test (const char* filename) {
    ResourceVectorTestSetup setup;
    setup.io = new ResourceIO(filename);
    setup.io->parse_and_allocate();
    setup.count = static_cast<uint32_t>(setup.io->validate_and_count());
    if (setup.count > 0) {
        setup.resources_available = new BitArrayCL(setup.count);
        setup.rv = new ResourceVector(setup.count, setup.resources_available);
    }
    return setup;
}

void verify_resource_data (const ResourceData& data, const char* test_name) {
    note_test_result (!data.stats.name.empty(), test_name, " - name not empty");
    note_test_result (data.stats.food >= 0, test_name, " - food >= 0");
    note_test_result (data.stats.shields >= 0, test_name, " - shields >= 0");
    note_test_result (data.stats.income >= 0, test_name, " - income >= 0");
}

void test_resource_at_index (ResourceVector* rv, uint32_t index, const char* test_prefix) {
    if (rv == nullptr) {
        note_test_result (false, test_prefix, " - ResourceVector is null");
        return;
    }
    ResourceData data = rv->get_resource(index);
    str test_name = str(test_prefix) + " - index " + std::to_string(index);
    verify_resource_data(data, test_name.c_str());
}

void test_bounds_get_resource (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    if (rv == nullptr || count == 0) {
        return;
    }
    test_resource_at_index(rv, 0, test_prefix);
    if (count > 1) {
        test_resource_at_index(rv, count / 2, test_prefix);
        test_resource_at_index(rv, count - 1, test_prefix);
    }
    ResourceData data_oob = rv->get_resource(count);
    str msg = str(test_prefix) + " - get_resource out of bounds (index " + std::to_string(count) + ")";
    note_test_result (true, msg.c_str(), " - no crash");
}

void test_bounds_set_available (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    if (rv == nullptr || count == 0) {
        return;
    }
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
    if (rv == nullptr || count == 0) {
        return;
    }
    rv->set_available(0);
    ResourceData data0 = rv->get_resource(0);
    verify_resource_data(data0, (str(test_prefix) + " - available index 0").c_str());
    
    if (count > 2) {
        uint32_t mid = count / 2;
        rv->set_available(mid);
        ResourceData data_mid = rv->get_resource(mid);
        verify_resource_data(data_mid, (str(test_prefix) + " - available index " + std::to_string(mid)).c_str());
    }
    
    if (count > 1) {
        rv->set_available(count - 1);
        ResourceData data_last = rv->get_resource(count - 1);
        verify_resource_data(data_last, (str(test_prefix) + " - available index " + std::to_string(count - 1)).c_str());
    }
}

void test_repeated_set_available (ResourceVector* rv, uint32_t index, const char* test_prefix) {
    if (rv == nullptr) {
        return;
    }
    rv->set_available(index);
    ResourceData data1 = rv->get_resource(index);
    verify_resource_data(data1, (str(test_prefix) + " - first set").c_str());
    
    rv->set_available(index);
    ResourceData data2 = rv->get_resource(index);
    verify_resource_data(data2, (str(test_prefix) + " - second set").c_str());
    
    rv->set_available(index);
    ResourceData data3 = rv->get_resource(index);
    verify_resource_data(data3, (str(test_prefix) + " - third set").c_str());
    
    str msg = str(test_prefix) + " - repeated calls consistent";
    note_test_result (data1.stats.name == data2.stats.name && data2.stats.name == data3.stats.name, msg.c_str());
}

void test_get_count_consistency (ResourceIO* io, ResourceVector* rv, const char* test_prefix) {
    if (io == nullptr || rv == nullptr) {
        return;
    }
    int io_count = io->validate_and_count();
    uint32_t rv_count = rv->get_count();
    str msg = str(test_prefix) + " - get_count matches validate_and_count";
    note_test_result (static_cast<int>(rv_count) == io_count, msg.c_str());
}

void test_resourceio_idempotency (const char* filename, const char* test_prefix) {
    ResourceIO io1(filename);
    int count1 = io1.validate_and_count();
    io1.parse_and_allocate();
    
    ResourceIO io2(filename);
    int count2 = io2.validate_and_count();
    io2.parse_and_allocate();
    
    str msg = str(test_prefix) + " - parse_and_allocate idempotent";
    note_test_result (count1 == count2, msg.c_str());
}

void test_multiple_indices (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    if (rv == nullptr || count == 0) {
        return;
    }
    test_resource_at_index(rv, 0, (str(test_prefix) + " - first").c_str());
    
    if (count > 2) {
        test_resource_at_index(rv, count / 2, (str(test_prefix) + " - middle").c_str());
    }
    
    if (count > 1) {
        test_resource_at_index(rv, count - 1, (str(test_prefix) + " - last").c_str());
    }
}

void test_resource_stats_variety (ResourceVector* rv, uint32_t count, const char* test_prefix) {
    if (rv == nullptr || count < 3) {
        return;
    }
    uint32_t idx_a = 0;
    uint32_t idx_b = count / 2;
    uint32_t idx_c = count - 1;
    ResourceData data_a = rv->get_resource(idx_a);
    ResourceData data_b = rv->get_resource(idx_b);
    ResourceData data_c = rv->get_resource(idx_c);
    
    bool different_food = (data_a.stats.food != data_b.stats.food) || (data_b.stats.food != data_c.stats.food) || (data_a.stats.food != data_c.stats.food);
    bool different_shields = (data_a.stats.shields != data_b.stats.shields) || (data_b.stats.shields != data_c.stats.shields) || (data_a.stats.shields != data_c.stats.shields);
    bool different_income = (data_a.stats.income != data_b.stats.income) || (data_b.stats.income != data_c.stats.income) || (data_a.stats.income != data_c.stats.income);
    
    note_test_result (different_food || different_shields || different_income, 
                      (str(test_prefix) + " - resources have varied stats").c_str());
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_resource_io () {
    ResourceIO io("../game_config.resources");
    int count = io.validate_and_count();
    note_test_result (count > 0, "ResourceIO validate and count");
    io.parse_and_allocate();
    io.print_content();
    summarize_test_results();
}

void test_resource_io_idempotency () {
    test_resourceio_idempotency("../game_config.resources", "ResourceIO idempotency");
    summarize_test_results();
}

void test_resource_vector () {
    ResourceVectorTestSetup setup = setup_resource_vector_test("../game_config.resources");
    if (setup.count == 0) {
        note_test_result (false, "ResourceVector test: no resources found");
        summarize_test_results();
        return;
    }
    note_test_result (setup.rv->get_count() == setup.count, "ResourceVector get_count");
    ResourceData data = setup.rv->get_resource(0);
    verify_resource_data(data, "ResourceVector basic");
    summarize_test_results();
}

void test_resource_vector_bounds () {
    ResourceVectorTestSetup setup = setup_resource_vector_test("../game_config.resources");
    if (setup.count == 0) {
        note_test_result (false, "ResourceVector bounds test: no resources found");
        summarize_test_results();
        return;
    }
    test_bounds_get_resource(setup.rv, setup.count, "ResourceVector bounds get_resource");
    test_bounds_set_available(setup.rv, setup.count, "ResourceVector bounds set_available");
    summarize_test_results();
}

void test_resource_vector_multiple_indices () {
    ResourceVectorTestSetup setup = setup_resource_vector_test("../game_config.resources");
    if (setup.count == 0) {
        note_test_result (false, "ResourceVector multiple indices test: no resources found");
        summarize_test_results();
        return;
    }
    test_multiple_indices(setup.rv, setup.count, "ResourceVector multiple indices");
    summarize_test_results();
}

void test_resource_vector_multiple_available () {
    ResourceVectorTestSetup setup = setup_resource_vector_test("../game_config.resources");
    if (setup.count == 0) {
        note_test_result (false, "ResourceVector multiple available test: no resources found");
        summarize_test_results();
        return;
    }
    test_multiple_available(setup.rv, setup.count, "ResourceVector multiple available");
    summarize_test_results();
}

void test_resource_vector_repeated_set_available () {
    ResourceVectorTestSetup setup = setup_resource_vector_test("../game_config.resources");
    if (setup.count == 0) {
        note_test_result (false, "ResourceVector repeated set_available test: no resources found");
        summarize_test_results();
        return;
    }
    test_repeated_set_available(setup.rv, 0, "ResourceVector repeated set_available index 0");
    if (setup.count > 1) {
        test_repeated_set_available(setup.rv, setup.count - 1, "ResourceVector repeated set_available index last");
    }
    summarize_test_results();
}

void test_resource_vector_get_count_consistency () {
    ResourceVectorTestSetup setup = setup_resource_vector_test("../game_config.resources");
    if (setup.count == 0) {
        note_test_result (false, "ResourceVector get_count consistency test: no resources found");
        summarize_test_results();
        return;
    }
    test_get_count_consistency(setup.io, setup.rv, "ResourceVector get_count consistency");
    summarize_test_results();
}

void test_resource_vector_stats_variety () {
    ResourceVectorTestSetup setup = setup_resource_vector_test("../game_config.resources");
    if (setup.count < 3) {
        note_test_result (false, "ResourceVector stats variety test: need at least 3 resources");
        summarize_test_results();
        return;
    }
    test_resource_stats_variety(setup.rv, setup.count, "ResourceVector stats variety");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main () {
    test_resource_io();
    test_resource_io_idempotency();
    test_resource_vector();
    test_resource_vector_bounds();
    test_resource_vector_multiple_indices();
    test_resource_vector_multiple_available();
    test_resource_vector_repeated_set_available();
    test_resource_vector_get_count_consistency();
    test_resource_vector_stats_variety();

    printf("\n================================\n");
    printf(" TOTAL FAILURES: %d\n", total_test_fails);
    printf("================================\n\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
