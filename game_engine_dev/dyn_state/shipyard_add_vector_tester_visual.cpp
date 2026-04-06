//================================================================================================================================
//=> - ShipyardAddVector visual tester -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#include "shipyard_add_vector.h"

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
        total_test_fails++;
        if (print_level > 0 && print_level < 3) {
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

class ShipyardAddVectorTester {
public:
    static u16 get_count(const ShipyardAddVector& array) {
        return array.m_shipyard_add_count;
    }
    static u16 get_page_count(const ShipyardAddVector& array) {
        return array.m_page_count;
    }
    void print_state(ShipyardAddVector& array) {
        u16 head_count = array.m_head_shipyard_add_idx;

        u16 limit = static_cast<u16>(((head_count + 99) / 100) * 100);
        if (limit == 0) {
            limit = 100;
        }

        for (u16 i = 0; i < limit; ++i) {
            char c = '-';
            ShipyardAddItem* item = array.get_shipyard_add(ShipyardAddKey::from_raw(i));
            if (item != nullptr) {
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

    static u16 get_head_count(const ShipyardAddVector& array) {
        return array.m_head_shipyard_add_idx;
    }
};

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

static void print_state_and_sleep(ShipyardAddVectorTester& tester, ShipyardAddVector& array) {
    if (print_level > 2) {
        tester.print_state(array);
        usleep(100000);
    }
}

static bool assert_count_increments(ShipyardAddVector& array, u16 before) {
    (void)array.get_next_new_shipyard_add_key();
    u16 after = ShipyardAddVectorTester::get_count(array);
    return after == static_cast<u16>(before + 1);
}

static void test_large_return_and_reuse () {
    ShipyardAddVector array;
    ShipyardAddVectorTester tester;

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    const u16 start_count = 925;
    const u16 return_count = 100;
    const u16 target_count = 1000;
    for (u16 i = 1; i <= start_count; ++i) {
        u16 before = ShipyardAddVectorTester::get_count(array);
        bool ok = assert_count_increments(array, before);
        note_result(ok, "get_next_new_item_key increments active count by 1");
    }

    u16 head_before_returns = ShipyardAddVectorTester::get_head_count(array);
    note_result(head_before_returns == static_cast<u16>(start_count + 1), "After creating start_count entries, head/frontier == start_count + 1");

    print_state_and_sleep(tester, array);

    bool picked[926];
    for (u16 i = 0; i <= start_count; ++i) {
        picked[i] = false;
    }

    u16 picked_count = 0;
    while (picked_count < return_count) {
        u16 r = static_cast<u16>((std::rand() % start_count) + 1);
        if (!picked[r]) {
            picked[r] = true;
            ++picked_count;
        }
    }

    u16 returned_list[return_count];
    u16 returned_pos = 0;
    for (u16 i = 1; i <= start_count; ++i) {
        if (!picked[i]) {
            continue;
        }

        bool ok = true;
        u16 before = ShipyardAddVectorTester::get_count(array);
        array.return_shipyard_add(ShipyardAddKey::from_raw(i));
        u16 after = ShipyardAddVectorTester::get_count(array);
        ok = (after == static_cast<u16>(before - 1));

        if (ok && array.get_shipyard_add(ShipyardAddKey::from_raw(i)) != nullptr) {
            ok = false;
        }

        note_result(ok, "return decrements active count by 1");
        if (ok) {
            returned_list[returned_pos] = i;
            ++returned_pos;
        }
    }

    note_result((returned_pos == return_count), "Returned exactly return_count unique entries");
    note_result((ShipyardAddVectorTester::get_count(array) == static_cast<u16>(start_count - return_count)),
                 "Active count after returns == start_count - return_count");
    note_result((ShipyardAddVectorTester::get_head_count(array) == head_before_returns),
                 "return does not change head/frontier count");

    print_state_and_sleep(tester, array);

    const u16 post_return_count = static_cast<u16>(start_count - return_count);
    const u16 allocs_needed = static_cast<u16>(target_count - post_return_count);
    note_result((ShipyardAddVectorTester::get_count(array) == post_return_count),
                 "Active count correct before filling to target_count");

    for (u16 a = 0; a < allocs_needed; ++a) {
        u16 before = ShipyardAddVectorTester::get_count(array);
        ShipyardAddKey id = array.get_next_new_shipyard_add_key();
        u16 after = ShipyardAddVectorTester::get_count(array);
        bool ok = (after == static_cast<u16>(before + 1));

        if (a < return_count) {
            u16 expected = returned_list[static_cast<u16>(return_count - 1 - a)];
            if (id.value() != expected) {
                ok = false;
            }
        } else {
            u16 expected = static_cast<u16>((start_count + 1) + (a - return_count));
            if (id.value() != expected) {
                ok = false;
            }
        }

        if (ok) {
            ShipyardAddItem* item = array.get_shipyard_add(id);
            if (item == nullptr) {
                ok = false;
            }
        }

        note_result(ok, "allocation increments count and reuses/allocates expected indices");
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
    std::printf(" TESTING ITEMS ARRAY: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
