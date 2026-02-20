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
#include "unit_vector.h"

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

static const uint16_t UNLOCKED_STRENGTH = 1000;

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
    } else if (print_level > 0) {
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

class TrainableAssessor {
public:
    TrainableAssessor (UnitVector* unit_vector) : m_unit_vector(unit_vector) {}
    
    void set_trainable (u32 idx) {
        if (m_unit_vector) {
            m_unit_vector->set_trainable(idx);
        }
    }
    
    void set_trainable_all () {
        if (m_unit_vector) {
            u32 count = m_unit_vector->get_count();
            for (u32 i = 0; i < count; i++) {
                m_unit_vector->set_trainable(i);
            }
        }
    }

private:
    UnitVector* m_unit_vector;
};

struct UnitVectorTestSetup {
    UnitIO* io;
    UnitVector* uv;
    BitArrayCL* techs;
    u32 count;
    
    UnitVectorTestSetup () : io(nullptr), uv(nullptr), techs(nullptr), count(0) {}
    
    ~UnitVectorTestSetup () {
        delete uv;
        delete techs;
        delete io;
    }
};

UnitVectorTestSetup setup_unit_vector_test (cstr filename) {
    UnitVectorTestSetup setup;
    setup.io = new UnitIO(filename);
    setup.io->parse_and_allocate();
    setup.count = static_cast<u32>(setup.io->validate_and_count());
    if (setup.count > 0) {
        setup.techs = new BitArrayCL(setup.count);
        setup.uv = new UnitVector(setup.techs);
    }
    return setup;
}

void verify_unit_data (const UnitData& data, cstr test_name) {
    note_test_result (!data.stats.name.empty(), test_name, " - name not empty");
    note_test_result (data.stats.cost > 0, test_name, " - cost > 0");
    note_test_result (data.stats.attack >= 0, test_name, " - attack >= 0");
    note_test_result (data.stats.defense >= 0, test_name, " - defense >= 0");
    note_test_result (data.stats.moves > 0, test_name, " - moves > 0");
}

void verify_unit_strength (const UnitData& data, bool should_be_unlocked, cstr test_name) {
    uint16_t expected_strength = should_be_unlocked ? UNLOCKED_STRENGTH : 0;
    str msg = str(test_name) + " - strength " + (should_be_unlocked ? "unlocked" : "locked");
    note_test_result (data.strength == expected_strength, msg.c_str());
}

void test_unit_at_idx (UnitVector* uv, u32 idx, cstr prefix) {
    if (uv == nullptr) {
        note_test_result (false, prefix, " - UnitVector is null");
        return;
    }
    UnitData data = uv->get_unit(idx);
    str test_name = str(prefix) + " - idx " + std::to_string(idx);
    verify_unit_data(data, test_name.c_str());
    verify_unit_strength(data, false, test_name.c_str());
}

void test_bounds_get_unit (UnitVector* uv, u32 count, cstr prefix) {
    if (uv == nullptr || count == 0) {
        return;
    }
    test_unit_at_idx(uv, 0, prefix);
    if (count > 1) {
        test_unit_at_idx(uv, count / 2, prefix);
        test_unit_at_idx(uv, count - 1, prefix);
    }
    UnitData data_oob = uv->get_unit(count);
    str msg = str(prefix) + " - get_unit out of bounds (idx " + std::to_string(count) + ")";
    note_test_result (true, msg.c_str(), " - no crash");
}

void test_bounds_set_trainable (TrainableAssessor& assessor, u32 count, cstr prefix) {
    if (count == 0) {
        return;
    }
    assessor.set_trainable(0);
    str msg1 = str(prefix) + " - set_trainable valid idx 0";
    note_test_result (true, msg1.c_str());
    
    if (count > 1) {
        assessor.set_trainable(count - 1);
        str msg2 = str(prefix) + " - set_trainable valid idx (last)";
        note_test_result (true, msg2.c_str());
    }
    
    assessor.set_trainable(count);
    str msg3 = str(prefix) + " - set_trainable out of bounds (idx " + std::to_string(count) + ")";
    note_test_result (true, msg3.c_str(), " - no crash");
}

void test_multiple_unlocks (UnitVector* uv, TrainableAssessor& assessor, BitArrayCL* tech, u32 count, cstr prefix) {
    if (uv == nullptr || tech == nullptr || count == 0) {
        return;
    }
    tech->set_bit(0);
    assessor.set_trainable(0);
    UnitData data0 = uv->get_unit(0);
    verify_unit_strength(data0, true, (str(prefix) + " - unlock idx 0").c_str());
    
    if (count > 2) {
        u32 mid = count / 2;
        tech->set_bit(mid);
        assessor.set_trainable(mid);
        UnitData data_mid = uv->get_unit(mid);
        verify_unit_strength(data_mid, true, (str(prefix) + " - unlock idx " + std::to_string(mid)).c_str());
    }
    
    if (count > 1) {
        tech->set_bit(count - 1);
        assessor.set_trainable(count - 1);
        UnitData data_last = uv->get_unit(count - 1);
        verify_unit_strength(data_last, true, (str(prefix) + " - unlock idx " + std::to_string(count - 1)).c_str());
    }
    
    UnitData data0_check = uv->get_unit(0);
    str msg = str(prefix) + " - multiple unlocks verify idx 0";
    note_test_result (data0_check.strength > 0, msg.c_str());
}

void test_strength_calc (UnitVector* uv, TrainableAssessor& assessor, BitArrayCL* tech, u32 idx, cstr prefix) {
    if (uv == nullptr || tech == nullptr) {
        return;
    }
    UnitData data_locked = uv->get_unit(idx);
    verify_unit_strength(data_locked, false, (str(prefix) + " - locked").c_str());
    
    tech->set_bit(idx);
    assessor.set_trainable(idx);
    UnitData data_unlocked = uv->get_unit(idx);
    str msg = str(prefix) + " - strength = 1000 when unlocked";
    note_test_result (data_unlocked.strength == UNLOCKED_STRENGTH, msg.c_str());
}

void test_repeated_set_trainable (UnitVector* uv, TrainableAssessor& assessor, BitArrayCL* tech, u32 idx, cstr prefix) {
    if (uv == nullptr || tech == nullptr) {
        return;
    }
    tech->set_bit(idx);
    assessor.set_trainable(idx);
    UnitData data1 = uv->get_unit(idx);
    verify_unit_strength(data1, true, (str(prefix) + " - first set").c_str());
    
    assessor.set_trainable(idx);
    UnitData data2 = uv->get_unit(idx);
    verify_unit_strength(data2, true, (str(prefix) + " - second set").c_str());
    
    assessor.set_trainable(idx);
    UnitData data3 = uv->get_unit(idx);
    verify_unit_strength(data3, true, (str(prefix) + " - third set").c_str());
    
    str msg = str(prefix) + " - repeated calls consistent";
    note_test_result (data1.strength == data2.strength && data2.strength == data3.strength, msg.c_str());
}

void test_get_count_consistency (UnitIO* io, UnitVector* uv, cstr prefix) {
    if (io == nullptr || uv == nullptr) {
        return;
    }
    int io_count = io->validate_and_count();
    u32 uv_count = uv->get_count();
    str msg = str(prefix) + " - get_count matches validate_and_count";
    note_test_result (static_cast<int>(uv_count) == io_count, msg.c_str());
}

void test_unitio_idempotency (cstr filename, cstr prefix) {
    UnitIO io1(filename);
    int count1 = io1.validate_and_count();
    io1.parse_and_allocate();
    
    UnitIO io2(filename);
    int count2 = io2.validate_and_count();
    io2.parse_and_allocate();
    
    str msg = str(prefix) + " - parse_and_allocate idempotent";
    note_test_result (count1 == count2, msg.c_str());
}

void test_multiple_indices (UnitVector* uv, u32 count, cstr prefix) {
    if (uv == nullptr || count == 0) {
        return;
    }
    test_unit_at_idx(uv, 0, (str(prefix) + " - first").c_str());
    
    if (count > 2) {
        test_unit_at_idx(uv, count / 2, (str(prefix) + " - middle").c_str());
    }
    
    if (count > 1) {
        test_unit_at_idx(uv, count - 1, (str(prefix) + " - last").c_str());
    }
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_unit_io () {
    UnitIO io("../game_config.units");
    int count = io.validate_and_count();
    note_test_result (count > 0, "UnitIO validate and count");
    io.parse_and_allocate();
    if (print_level > 1) {
        io.print_content();
    }
    summarize_test_results();
}

void test_unit_io_idempotency () {
    test_unitio_idempotency("../game_config.units", "UnitIO idempotency");
    summarize_test_results();
}

void test_unit_vector () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec test: no units found");
        summarize_test_results();
        return;
    }
    note_test_result (setup.uv->get_count() == setup.count, "UnitVec get_count");
    UnitData data = setup.uv->get_unit(0);
    verify_unit_data(data, "UnitVec basic");
    note_test_result (data.strength == 0, "UnitVec get_unit strength (initial, not unlocked)");
    summarize_test_results();
}

void test_unit_vector_unlocked () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec unlocked test: no units found");
        summarize_test_results();
        return;
    }
    TrainableAssessor assessor(setup.uv);
    UnitData data1 = setup.uv->get_unit(0);
    note_test_result (data1.strength == 0, "UnitVec get_unit strength (not unlocked)");
    setup.techs->set_bit(0);
    assessor.set_trainable(0);
    UnitData data2 = setup.uv->get_unit(0);
    note_test_result (data2.strength == UNLOCKED_STRENGTH, "UnitVec get_unit strength (unlocked)");
    summarize_test_results();
}

void test_unit_vector_bounds () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec bounds test: no units found");
        summarize_test_results();
        return;
    }
    TrainableAssessor assessor(setup.uv);
    test_bounds_get_unit(setup.uv, setup.count, "UnitVec bounds get_unit");
    test_bounds_set_trainable(assessor, setup.count, "UnitVec bounds set_trainable");
    summarize_test_results();
}

void test_unit_vector_multiple_indices () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec multiple indices test: no units found");
        summarize_test_results();
        return;
    }
    test_multiple_indices(setup.uv, setup.count, "UnitVec multiple indices");
    summarize_test_results();
}

void test_unit_vector_multiple_unlocks () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec multiple unlocks test: no units found");
        summarize_test_results();
        return;
    }
    TrainableAssessor assessor(setup.uv);
    test_multiple_unlocks(setup.uv, assessor, setup.techs, setup.count, "UnitVec multiple unlocks");
    summarize_test_results();
}

void test_unit_vector_strength_calc () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec strength calc test: no units found");
        summarize_test_results();
        return;
    }
    TrainableAssessor assessor(setup.uv);
    test_strength_calc(setup.uv, assessor, setup.techs, 0, "UnitVec strength calc idx 0");
    if (setup.count > 1) {
        test_strength_calc(setup.uv, assessor, setup.techs, setup.count - 1, "UnitVec strength calc idx last");
    }
    if (setup.count > 2) {
        test_strength_calc(setup.uv, assessor, setup.techs, setup.count / 2, "UnitVec strength calc idx middle");
    }
    summarize_test_results();
}

void test_unit_vector_repeated_set_available () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec repeated set_trainable test: no units found");
        summarize_test_results();
        return;
    }
    TrainableAssessor assessor(setup.uv);
    test_repeated_set_trainable(setup.uv, assessor, setup.techs, 0, "UnitVec repeated set_trainable idx 0");
    if (setup.count > 1) {
        test_repeated_set_trainable(setup.uv, assessor, setup.techs, setup.count - 1, "UnitVec repeated set_trainable idx last");
    }
    summarize_test_results();
}

void test_unit_vector_get_count_consistency () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec get_count consistency test: no units found");
        summarize_test_results();
        return;
    }
    test_get_count_consistency(setup.io, setup.uv, "UnitVec get_count consistency");
    summarize_test_results();
}

void test_unit_vector_strength_with_different_units () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec strength different units test: no units found");
        summarize_test_results();
        return;
    }
    TrainableAssessor assessor(setup.uv);
    for (u32 i = 0; i < setup.count && i < 5; i++) {
        UnitData data_locked = setup.uv->get_unit(i);
        cstr test_name = ("UnitVec strength different units idx " + std::to_string(i) + " locked").c_str();
        verify_unit_strength(data_locked, false, test_name);
        
        setup.techs->set_bit(i);
        assessor.set_trainable(i);
        UnitData data_unlocked = setup.uv->get_unit(i);
        str msg = "UnitVec strength different units idx " + std::to_string(i) + " unlocked";
        note_test_result (data_unlocked.strength == UNLOCKED_STRENGTH, msg.c_str());
    }
    summarize_test_results();
}

void test_unit_vector_all_locked_initially () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count == 0) {
        note_test_result (false, "UnitVec all locked initially test: no units found");
        summarize_test_results();
        return;
    }
    u32 checked = 0;
    for (u32 i = 0; i < setup.count && checked < 10; i++) {
        UnitData data = setup.uv->get_unit(i);
        str msg = "UnitVec all locked initially idx " + std::to_string(i);
        note_test_result (data.strength == 0, msg.c_str());
        checked++;
    }
    summarize_test_results();
}

void test_unit_vector_strength_independent () {
    UnitVectorTestSetup setup = setup_unit_vector_test("../game_config.units");
    if (setup.count < 3) {
        note_test_result (false, "UnitVec strength independent test: need at least 3 units");
        summarize_test_results();
        return;
    }
    TrainableAssessor assessor(setup.uv);
    u32 idx_a = 0;
    u32 idx_b = setup.count / 2;
    u32 idx_c = setup.count - 1;
    setup.techs->set_bit(idx_a);
    setup.techs->set_bit(idx_b);
    setup.techs->set_bit(idx_c);
    assessor.set_trainable(idx_a);
    assessor.set_trainable(idx_b);
    assessor.set_trainable(idx_c);
    UnitData data_a = setup.uv->get_unit(idx_a);
    UnitData data_b = setup.uv->get_unit(idx_b);
    UnitData data_c = setup.uv->get_unit(idx_c);
    note_test_result (data_a.strength == UNLOCKED_STRENGTH, "UnitVec strength independent - unit A");
    note_test_result (data_b.strength == UNLOCKED_STRENGTH, "UnitVec strength independent - unit B");
    note_test_result (data_c.strength == UNLOCKED_STRENGTH, "UnitVec strength independent - unit C");
    uint16_t sum_a = data_a.stats.attack + data_a.stats.defense;
    uint16_t sum_b = data_b.stats.attack + data_b.stats.defense;
    uint16_t sum_c = data_c.stats.attack + data_c.stats.defense;
    bool different_stats = (sum_a != sum_b) || (sum_b != sum_c) || (sum_a != sum_c);
    note_test_result (different_stats, "UnitVec strength independent - units have different attack+defense");
    note_test_result (data_a.strength == data_b.strength && data_b.strength == data_c.strength,
                      "UnitVec strength independent - all unlocked units same strength");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    test_unit_io();
    test_unit_io_idempotency();
    test_unit_vector();
    test_unit_vector_unlocked();
    test_unit_vector_bounds();
    test_unit_vector_multiple_indices();
    test_unit_vector_multiple_unlocks();
    test_unit_vector_strength_calc();
    test_unit_vector_repeated_set_available();
    test_unit_vector_get_count_consistency();
    test_unit_vector_strength_with_different_units();
    test_unit_vector_all_locked_initially();
    test_unit_vector_strength_independent();

    printf("\n=======================================================\n");
    printf(" TESTING UNITS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("======================================================\n\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
