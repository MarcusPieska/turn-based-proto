//================================================================================================================================
//=> - UnitArray visual tester -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

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
        if (print_level > 1 && print_level < 3) {
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        if (print_level > 0 && print_level < 3) {
            total_test_fails++;
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void summarize_test_results () {
    if (print_level > 0 && print_level < 3) {
        std::printf("--------------------------------\n");
        std::printf(" Test count: %d\n", test_count);
        std::printf(" Test pass: %d\n", test_pass);
        std::printf(" Test fail: %d\n", test_count - test_pass);
        std::printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

class UnitArrayTester {
public:
    void print_state(UnitArray& array) {
        u16 head_count = array.m_head_unit_idx; // friend access

        u16 limit = static_cast<u16>(((head_count + 99) / 100) * 100);
        if (limit == 0) {
            limit = 100;
        }

        for (u16 i = 0; i < limit; ++i) {
            char c = '-';
            Unit* unit = array.get_unit(i);
            if (unit != nullptr) {
                c = '1';
            } else if (i < head_count) {
                c = '0';
            }
            if (c == '1') {
                std::printf("\033[32m%c\033[0m", c);
            } else if (c == '0') {
                std::printf("\033[31m%c\033[0m", c);
            } else {
                std::printf("%c", c);
            }
            if (i % 100 == 99) {
                std::printf("\n");
            } else if (i % 10 == 9) {
                std::printf(" ");
            }
        }

        if (limit % 100 != 0) {
            std::printf("\n");
        }
        std::printf("\n");
    }
};

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

static void print_state_and_sleep(UnitArrayTester& tester, UnitArray& array) {
    if (print_level > 0) {
        tester.print_state(array);
        usleep(100000); 
    }
}

static bool assert_unit_count_increments(UnitArray& array, u16 before) {
    (void)array.get_next_new_unit_idx();
    u16 after = array.get_unit_count();
    return after == static_cast<u16>(before + 1);
}

static u16 allocate_and_assert(UnitArray& array, u16 before) {
    u16 idx = array.get_next_new_unit_idx();
    u16 after = array.get_unit_count();
    if (after != static_cast<u16>(before + 1)) {
        return 0;
    }
    return idx;
}

static void test_large_return_and_reuse () {
    UnitArray array;
    UnitArrayTester tester;

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    const u16 start_count = 925;
    const u16 return_count = 100;
    const u16 target_count = 1000;
    for (u16 i = 0; i < start_count; ++i) {
        u16 before = array.get_unit_count();
        bool ok = assert_unit_count_increments(array, before);
        note_result(ok, "get_next_new_unit_idx increments active unit count by 1");
    }

    u16 head_before_returns = array.get_head_unit_count();
    note_result(head_before_returns == start_count, "After creating start_count units, head/frontier == start_count");

    print_state_and_sleep(tester, array);
    bool picked[925];
    for (u16 i = 0; i < start_count; ++i) {
        picked[i] = false;
    }

    u16 picked_count = 0;
    while (picked_count < return_count) {
        u16 r = static_cast<u16>(std::rand() % start_count);
        if (!picked[r]) {
            picked[r] = true;
            ++picked_count;
        }
    }

    u16 returned_list[return_count];
    u16 returned_pos = 0;
    for (u16 i = 0; i < start_count; ++i) {
        if (!picked[i]) {
            continue;
        }

        Unit* unit = array.get_unit(i);
        bool ok = (unit != nullptr && unit->do_exist());
        if (ok) {
            u16 before = array.get_unit_count();
            array.return_unit(unit);
            u16 after = array.get_unit_count();
            ok = (after == static_cast<u16>(before - 1));
        }

        // Returned units should become non-retrievable immediately.
        if (ok) {
            if (array.get_unit(i) != nullptr) {
                ok = false;
            }
        }
        note_result(ok, "return_unit decrements active unit count by 1");
        if (ok) {
            returned_list[returned_pos] = i;
            ++returned_pos;
        }
    }

    note_result((returned_pos == return_count), "Returned exactly return_count unique units");
    note_result((array.get_unit_count() == static_cast<u16>(start_count - return_count)),
                 "Active unit count after returns == start_count - return_count");
    note_result((array.get_head_unit_count() == head_before_returns),
                 "return_unit does not change head/frontier count");

    print_state_and_sleep(tester, array);

    const u16 post_return_count = static_cast<u16>(start_count - return_count);
    const u16 allocs_needed = static_cast<u16>(target_count - post_return_count); // should be 175
    note_result((array.get_unit_count() == post_return_count),
                 "Active unit count correct before filling to 1000");

    for (u16 a = 0; a < allocs_needed; ++a) {
        u16 before = array.get_unit_count();
        u16 idx = array.get_next_new_unit_idx();
        u16 after = array.get_unit_count();
        bool ok = (after == static_cast<u16>(before + 1));

        if (a < return_count) {
            // Reuse recycled indices in LIFO order.
            u16 expected = 0;
            if (returned_pos == return_count) {
                expected = returned_list[static_cast<u16>(return_count - 1 - a)];
                if (idx != expected) {
                    ok = false;
                }
            } else {
                ok = false;
            }
        } else {
            // New indices created at the head frontier.
            u16 expected = static_cast<u16>(start_count + (a - return_count));
            if (idx != expected) {
                ok = false;
            }
        }

        // The newly allocated index must exist.
        if (ok) {
            Unit* unit = array.get_unit(idx);
            if (unit == nullptr || !unit->do_exist()) {
                ok = false;
            }
        }

        note_result(ok, "get_next_new_unit_idx increments count and reuses/allocates expected indices");
        print_state_and_sleep(tester, array);
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

    test_large_return_and_reuse();
    
    std::printf("=======================================================\n");
    std::printf(" TESTING UNIT ARRAY: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
