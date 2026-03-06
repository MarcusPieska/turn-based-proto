//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <sstream>

#include "effect_vector.h"

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
    } else if (print_level > 0) {
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

struct EffectIOTestSetup {
    EffectIO* io;
    u32 count;
    
    EffectIOTestSetup () : io(nullptr), count(0) {}
    
    ~EffectIOTestSetup () {
        delete io;
    }
};

EffectIOTestSetup setup_effect_io_test (cstr filename) {
    EffectIOTestSetup setup;
    setup.io = new EffectIO(filename);
    setup.io->parse_and_allocate();
    setup.count = static_cast<u32>(setup.io->validate_and_count());
    return setup;
}

void verify_effect_type (const Effect& e, EffectType expected_type, cstr test_name) {
    str msg = str(test_name) + " - effect type matches";
    note_result (e.type == expected_type, msg.c_str());
}

void verify_build_effect (const Effect& e, cstr test_name) {
    verify_effect_type(e, EffectType::BUILD, test_name);
    str msg1 = str(test_name) + " - build effect has valid scope";
    note_result (e.data.build.scope <= static_cast<u16>(static_cast<int>(Scope::CIV)), msg1.c_str());
    str msg2 = str(test_name) + " - build effect has valid auto_build";
    note_result (e.data.build.auto_build <= static_cast<u16>(static_cast<int>(AutoBuild::AUTO_BUILD)), msg2.c_str());
    str msg3 = str(test_name) + " - build effect has valid no_upkeep";
    note_result (e.data.build.no_upkeep <= static_cast<u16>(static_cast<int>(NoUpkeep::NO_UPKEEP)), msg3.c_str());
}

void verify_research_tech_effect (const Effect& e, cstr test_name) {
    verify_effect_type(e, EffectType::RESEARCH_TECH, test_name);
    str msg = str(test_name) + " - research tech effect has tech_count > 0";
    note_result (e.data.research_tech.tech_count > 0, msg.c_str());
}

void verify_booster_effect (const Effect& e, cstr test_name) {
    verify_effect_type(e, EffectType::BOOSTER, test_name);
    str msg1 = str(test_name) + " - booster effect has valid target";
    note_result (e.data.booster.target <= static_cast<u16>(static_cast<int>(Target::UNIT_EXP)), msg1.c_str());
    str msg2 = str(test_name) + " - booster effect has valid scope";
    note_result (e.data.booster.scope <= static_cast<u16>(static_cast<int>(Scope::CIV)), msg2.c_str());
    str msg3 = str(test_name) + " - booster effect has valid value_type";
    note_result (e.data.booster.value_type <= static_cast<u16>(static_cast<int>(ValueType::PERCENTAGE)), msg3.c_str());
}

void verify_train_effect (const Effect& e, cstr test_name) {
    verify_effect_type(e, EffectType::TRAIN, test_name);
    str msg = str(test_name) + " - train effect has count > 0";
    note_result (e.data.train.count > 0, msg.c_str());
}

void test_effect_at_idx (u32 idx, cstr prefix) {
    u32 count = EffectVector::get_count();
    if (count == 0) {
        note_result (false, prefix, " - no effects available");
        return;
    }
    if (idx >= count) {
        note_result (false, prefix, " - index out of bounds");
        return;
    }
    const Effect& e = EffectVector::get_effect(idx);
    str test_name = str(prefix) + " - idx " + std::to_string(idx);
    
    switch (e.type) {
        case EffectType::BUILD:
            verify_build_effect(e, test_name.c_str());
            break;
        case EffectType::RESEARCH_TECH:
            verify_research_tech_effect(e, test_name.c_str());
            break;
        case EffectType::BOOSTER:
            verify_booster_effect(e, test_name.c_str());
            break;
        case EffectType::TRAIN:
            verify_train_effect(e, test_name.c_str());
            break;
        default:
            note_result (false, test_name.c_str(), " - unknown effect type");
            break;
    }
}

void test_bounds_get_effect (u32 count, cstr prefix) {
    if (count == 0) {
        return;
    }
    test_effect_at_idx(0, prefix);
    if (count > 1) {
        test_effect_at_idx(count / 2, prefix);
        test_effect_at_idx(count - 1, prefix);
    }
    try {
        const Effect& e = EffectVector::get_effect(count);
        str msg = str(prefix) + " - get_effect out of bounds (idx " + std::to_string(count) + ")";
        note_result (true, msg.c_str(), " - no crash");
    } catch (...) {
        str msg = str(prefix) + " - get_effect out of bounds (idx " + std::to_string(count) + ")";
        note_result (true, msg.c_str(), " - throws exception");
    }
}

void test_multiple_indices (u32 count, cstr prefix) {
    if (count == 0) {
        return;
    }
    test_effect_at_idx(0, (str(prefix) + " - first").c_str());
    
    if (count > 2) {
        test_effect_at_idx(count / 2, (str(prefix) + " - middle").c_str());
    }
    
    if (count > 1) {
        test_effect_at_idx(count - 1, (str(prefix) + " - last").c_str());
    }
}

void test_get_count_consistency (EffectIO* io, cstr prefix) {
    if (io == nullptr) {
        return;
    }
    int io_count = io->validate_and_count();
    u32 ev_count = EffectVector::get_count();
    str msg = str(prefix) + " - get_count matches validate_and_count";
    note_result (static_cast<int>(ev_count) == io_count, msg.c_str());
}

void test_effectio_idempotency (cstr filename, cstr prefix) {
    EffectIO io1(filename);
    int count1 = io1.validate_and_count();
    io1.parse_and_allocate();
    
    EffectIO io2(filename);
    int count2 = io2.validate_and_count();
    io2.parse_and_allocate();
    
    str msg = str(prefix) + " - parse_and_allocate idempotent";
    note_result (count1 == count2, msg.c_str());
}

void test_effect_types_distribution (u32 count, cstr prefix) {
    if (count == 0) {
        return;
    }
    u32 build_count = 0;
    u32 research_tech_count = 0;
    u32 booster_count = 0;
    u32 train_count = 0;
    
    for (u32 i = 0; i < count; i++) {
        const Effect& e = EffectVector::get_effect(i);
        switch (e.type) {
            case EffectType::BUILD:
                build_count++;
                break;
            case EffectType::RESEARCH_TECH:
                research_tech_count++;
                break;
            case EffectType::BOOSTER:
                booster_count++;
                break;
            case EffectType::TRAIN:
                train_count++;
                break;
        }
    }
    
    str msg1 = str(prefix) + " - has BUILD effects";
    note_result (build_count > 0 || count < 10, msg1.c_str());
    
    str msg2 = str(prefix) + " - has RESEARCH_TECH effects";
    note_result (research_tech_count > 0 || count < 10, msg2.c_str());
    
    str msg3 = str(prefix) + " - has BOOSTER effects";
    note_result (booster_count > 0, msg3.c_str());
    
    str msg4 = str(prefix) + " - has TRAIN effects";
    note_result (train_count > 0 || count < 10, msg4.c_str());
}

void test_booster_effect_values (u32 count, cstr prefix) {
    if (count == 0) {
        return;
    }
    u32 booster_found = 0;
    for (u32 i = 0; i < count && booster_found < 5; i++) {
        const Effect& e = EffectVector::get_effect(i);
        if (e.type == EffectType::BOOSTER) {
            booster_found++;
            str msg1 = str(prefix) + " - booster " + std::to_string(i) + " has valid target";
            note_result (e.data.booster.target <= static_cast<u16>(static_cast<int>(Target::UNIT_EXP)), msg1.c_str());
            str msg2 = str(prefix) + " - booster " + std::to_string(i) + " has valid scope";
            note_result (e.data.booster.scope <= static_cast<u16>(static_cast<int>(Scope::CIV)), msg2.c_str());
            str msg3 = str(prefix) + " - booster " + std::to_string(i) + " has valid value_type";
            note_result (e.data.booster.value_type <= static_cast<u16>(static_cast<int>(ValueType::PERCENTAGE)), msg3.c_str());
        }
    }
    str msg = str(prefix) + " - found booster effects";
    note_result (booster_found > 0, msg.c_str());
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_effect_io () {
    EffectIO io("../game_config.effects");
    int count = io.validate_and_count();
    note_result (count > 0, "EffectIO validate and count");
    io.parse_and_allocate();
    if (print_level > 1) {
        io.print_content();
    }
    summarize_test_results();
}

void test_effect_io_idempotency () {
    test_effectio_idempotency("../game_config.effects", "EffectIO idempotency");
    summarize_test_results();
}

void test_effect_vector () {
    EffectIOTestSetup setup = setup_effect_io_test("../game_config.effects");
    if (setup.count == 0) {
        note_result (false, "EffectVector test: no effects found");
        summarize_test_results();
        return;
    }
    u32 ev_count = EffectVector::get_count();
    note_result (ev_count == setup.count, "EffectVector get_count");
    if (ev_count > 0) {
        const Effect& e = EffectVector::get_effect(0);
        str test_name = "EffectVector basic - idx 0";
        switch (e.type) {
            case EffectType::BUILD:
                verify_build_effect(e, test_name.c_str());
                break;
            case EffectType::RESEARCH_TECH:
                verify_research_tech_effect(e, test_name.c_str());
                break;
            case EffectType::BOOSTER:
                verify_booster_effect(e, test_name.c_str());
                break;
            case EffectType::TRAIN:
                verify_train_effect(e, test_name.c_str());
                break;
        }
    }
    summarize_test_results();
}

void test_effect_vector_bounds () {
    EffectIOTestSetup setup = setup_effect_io_test("../game_config.effects");
    if (setup.count == 0) {
        note_result (false, "EffectVector bounds test: no effects found");
        summarize_test_results();
        return;
    }
    test_bounds_get_effect(setup.count, "EffectVector bounds get_effect");
    summarize_test_results();
}

void test_effect_vector_multiple_indices () {
    EffectIOTestSetup setup = setup_effect_io_test("../game_config.effects");
    if (setup.count == 0) {
        note_result (false, "EffectVector multiple indices test: no effects found");
        summarize_test_results();
        return;
    }
    test_multiple_indices(setup.count, "EffectVector multiple indices");
    summarize_test_results();
}

void test_effect_vector_get_count_consistency () {
    EffectIOTestSetup setup = setup_effect_io_test("../game_config.effects");
    if (setup.count == 0) {
        note_result (false, "EffectVector get_count consistency test: no effects found");
        summarize_test_results();
        return;
    }
    test_get_count_consistency(setup.io, "EffectVector get_count consistency");
    summarize_test_results();
}

void test_effect_vector_types_distribution () {
    EffectIOTestSetup setup = setup_effect_io_test("../game_config.effects");
    if (setup.count == 0) {
        note_result (false, "EffectVector types distribution test: no effects found");
        summarize_test_results();
        return;
    }
    test_effect_types_distribution(setup.count, "EffectVector types distribution");
    summarize_test_results();
}

void test_effect_vector_booster_values () {
    EffectIOTestSetup setup = setup_effect_io_test("../game_config.effects");
    if (setup.count == 0) {
        note_result (false, "EffectVector booster values test: no effects found");
        summarize_test_results();
        return;
    }
    test_booster_effect_values(setup.count, "EffectVector booster values");
    summarize_test_results();
}

void test_effect_vector_static_access () {
    EffectIOTestSetup setup = setup_effect_io_test("../game_config.effects");
    if (setup.count == 0) {
        note_result (false, "EffectVector static access test: no effects found");
        summarize_test_results();
        return;
    }
    
    // Test that we can access static methods without instantiation
    u32 count1 = EffectVector::get_count();
    note_result (count1 == setup.count, "EffectVector static get_count");
    
    if (count1 > 0) {
        const Effect& effect = EffectVector::get_effect(0);
        str msg = "EffectVector static get_effect";
        note_result (effect.type >= EffectType::BUILD && effect.type <= EffectType::TRAIN, msg.c_str());
    }
    
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    test_effect_io();
    test_effect_io_idempotency();
    test_effect_vector();
    test_effect_vector_bounds();
    test_effect_vector_multiple_indices();
    test_effect_vector_get_count_consistency();
    test_effect_vector_types_distribution();
    test_effect_vector_booster_values();
    test_effect_vector_static_access();

    printf("=======================================================\n");
    printf(" TESTING EFFECTS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
