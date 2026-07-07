//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "unit_array.h"

//================================================================================================================================
//=> - GLOBALS -
//================================================================================================================================

typedef const char* cstr;

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
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        if (print_level > 0) {
            total_test_fails++;
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void summarize_test_results () {
    if (print_level > 0) {
        std::printf("--------------------------------\n");
        std::printf(" Test count: %d\n", test_count);
        std::printf(" Test pass: %d\n", test_pass);
        std::printf(" Test fail: %d\n", test_count - test_pass);
        std::printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_first_unit_zero_initialized () {
    UnitArray array;
    Unit* should_be_null = array.get_unit(0);
    bool ok = (should_be_null == nullptr);

    u16 idx = array.get_next_new_unit_idx();
    Unit* unit = array.get_unit(idx);

    if (unit == nullptr) {
        ok = false;
    } else if (!unit->do_exist()) {
        ok = false;
    } else if (unit->id != 0) {
        ok = false;
    }

    note_result(ok, "First unit: exists==0 before creation, exists==1 after get_next_new_unit_idx()");
    summarize_test_results();
}

void test_unit_array_unique_ids () {
    UnitArray array;
    const u16 target_count = 1000;

    bool ok_ids = true;

    for (u16 i = 0; i < target_count; ++i) {
        u16 idx = array.get_next_new_unit_idx();
        if (idx != i) {
            ok_ids = false;
        }
        Unit* unit = array.get_unit(idx);
        if (unit == nullptr) {
            ok_ids = false;
            break;
        }
        if (!unit->do_exist()) {
            ok_ids = false;
            break;
        }
        unit->id = idx;
    }

    if (ok_ids) {
        u16 total = array.get_unit_count();
        if (total < target_count) {
            ok_ids = false;
        } else {
            for (u16 i = 0; i < target_count; ++i) {
                Unit* unit = array.get_unit(i);
                if (unit == nullptr) {
                    ok_ids = false;
                    break;
                }
                if (!unit->do_exist()) {
                    ok_ids = false;
                    break;
                }
                if (unit->id != i) {
                    ok_ids = false;
                    break;
                }
            }
        }
    }

    note_result(ok_ids, "UnitArray 1000 units have unique IDs via Unit::id");
    summarize_test_results();
}

void test_return_unit_clears_existence () {
    UnitArray array;
    const u16 target_count = 10;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_idx();
    }

    bool ok = true;

    u16 return_idx1 = 3;
    Unit* unit1 = array.get_unit(return_idx1);
    if (unit1 == nullptr || !unit1->do_exist()) {
        ok = false;
    } else {
        array.return_unit(unit1);
        if (unit1->do_exist()) {
            ok = false;
        }
        if (array.get_unit(return_idx1) != nullptr) {
            ok = false;
        }
    }

    u16 return_idx2 = 8;
    Unit* unit2 = array.get_unit(return_idx2);
    if (unit2 == nullptr || !unit2->do_exist()) {
        ok = false;
    } else {
        array.return_unit(unit2);
        if (unit2->do_exist()) {
            ok = false;
        }
        if (array.get_unit(return_idx2) != nullptr) {
            ok = false;
        }
    }

    note_result(ok, "return_unit(Unit*) sets exists==0 and get_unit returns nullptr");
    summarize_test_results();
}

void test_return_null_is_noop () {
    UnitArray array;
    const u16 target_count = 5;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_idx();
    }

    u16 before_count = array.get_unit_count();
    u16 before_head  = array.get_head_unit_count();

    array.return_unit(nullptr);

    bool ok = true;
    if (array.get_unit_count() != before_count) {
        ok = false;
    }
    if (array.get_head_unit_count() != before_head) {
        ok = false;
    }

    note_result(ok, "return_unit(nullptr) is a no-op");
    summarize_test_results();
}

void test_return_decrements_unit_count () {
    UnitArray array;
    const u16 target_count = 6;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_idx();
    }

    u16 before_count = array.get_unit_count();
    u16 before_head  = array.get_head_unit_count();

    const u16 return_idx = 4;
    Unit* unit = array.get_unit(return_idx);
    bool ok = true;
    if (unit == nullptr || !unit->do_exist()) {
        ok = false;
    } else {
        array.return_unit(unit);
        if (array.get_unit_count() != static_cast<u16>(before_count - 1)) {
            ok = false;
        }
        if (array.get_head_unit_count() != before_head) {
            ok = false;
        }
        if (array.get_unit(return_idx) != nullptr) {
            ok = false;
        }
    }

    note_result(ok, "return_unit decrements active count and makes get_unit return nullptr");
    summarize_test_results();
}

void test_return_does_not_change_head () {
    UnitArray array;
    const u16 target_count = 8;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_idx();
    }

    u16 before_head = array.get_head_unit_count();

    Unit* unit = array.get_unit(1);
    bool ok = (unit != nullptr && unit->do_exist());
    if (ok) {
        array.return_unit(unit);
        ok = (array.get_head_unit_count() == before_head);
    }

    note_result(ok, "return_unit does not change get_head_unit_count");
    summarize_test_results();
}

void test_reuse_recycled_slot_basic () {
    UnitArray array;
    for (u16 i = 0; i < 3; ++i) {
        array.get_next_new_unit_idx();
    }

    u16 before_count = array.get_unit_count(); 
    u16 before_head  = array.get_head_unit_count();
    const u16 return_idx = 1;
    Unit* unit = array.get_unit(return_idx);
    bool ok = (unit != nullptr && unit->do_exist());
    if (ok) {
        array.return_unit(unit);
        u16 new_idx = array.get_next_new_unit_idx();
        if (new_idx != return_idx) {
            ok = false;
        }
        if (array.get_unit_count() != before_count) {
            ok = false;
        }
        if (array.get_head_unit_count() != before_head) {
            ok = false;
        }
        Unit* reused = array.get_unit(return_idx);
        if (reused == nullptr || !reused->do_exist()) {
            ok = false;
        }
    }

    note_result(ok, "Realloc after return reuses the exact returned index (basic)");
    summarize_test_results();
}

void test_reuse_recycled_slot_lifo () {
    UnitArray array;
    for (u16 i = 0; i < 4; ++i) {
        array.get_next_new_unit_idx();
    }

    Unit* u1 = array.get_unit(1);
    Unit* u2 = array.get_unit(2);
    bool ok = (u1 != nullptr && u1->do_exist() && u2 != nullptr && u2->do_exist());
    if (ok) {
        array.return_unit(u1);
        array.return_unit(u2);

        u16 n1 = array.get_next_new_unit_idx();
        u16 n2 = array.get_next_new_unit_idx();

        if (n1 != 2 || n2 != 1) {
            ok = false;
        }
        if (array.get_head_unit_count() != 4) {
            ok = false;
        }
    }

    note_result(ok, "Realloc after multiple returns reuses recycled indices in LIFO order");
    summarize_test_results();
}

void test_return_twice_is_noop_second_time () {
    UnitArray array;
    for (u16 i = 0; i < 5; ++i) {
        array.get_next_new_unit_idx();
    }

    const u16 return_idx = 3;
    Unit* unit = array.get_unit(return_idx);

    bool ok = true;
    if (unit == nullptr || !unit->do_exist()) {
        ok = false;
    } else {
        u16 before_count = array.get_unit_count();
        array.return_unit(unit);
        u16 after_first = array.get_unit_count();
        array.return_unit(unit);
        u16 after_second = array.get_unit_count();

        if (after_first != static_cast<u16>(before_count - 1)) {
            ok = false;
        }
        if (after_second != after_first) {
            ok = false;
        }
        if (array.get_unit(return_idx) != nullptr) {
            ok = false;
        }
    }

    note_result(ok, "return_unit called twice on the same Unit* only decrements once");
    summarize_test_results();
}

void test_unit_array_page_allocation () {
    UnitArray array;
    const u16 target_count = 1000;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_idx();
    }

    u16 page_count = array.get_page_count();
    bool ok_pages = true;

    if (page_count == 0) {
        ok_pages = false;
    } else {
        for (u16 p = 0; p < page_count; ++p) {
            Unit* page = array.get_page(p);
            if (page == nullptr) {
                ok_pages = false;
                break;
            }
        }
    }

    note_result(ok_pages, "UnitArray page allocation matches page count");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    
    test_first_unit_zero_initialized();
    test_unit_array_unique_ids();
    test_return_unit_clears_existence();
    test_return_null_is_noop();
    test_return_decrements_unit_count();
    test_return_does_not_change_head();
    test_reuse_recycled_slot_basic();
    test_reuse_recycled_slot_lifo();
    test_return_twice_is_noop_second_time();
    test_unit_array_page_allocation();
    
    std::printf("=======================================================\n");
    std::printf(" TESTING UNIT ARRAY: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    
    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
