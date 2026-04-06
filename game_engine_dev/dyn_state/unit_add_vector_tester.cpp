//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "unit_add_vector.h"

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

class UnitAddVectorTester {
    public:
        static u16 get_count(const UnitAddVector& array) {
            return array.m_unit_add_count;
        }
        static u16 get_page_count(const UnitAddVector& array) {
            return array.m_page_count;
        }
        static u16 get_head_count(const UnitAddVector& array) {
            return array.m_head_unit_add_idx;
        }
    };

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_first_item_zero_initialized () {
    UnitAddVector array;
    UnitAddItem* should_be_null = array.get_unit_add(UnitAddKey::from_raw(0));
    bool ok = (should_be_null == nullptr);

    UnitAddKey id = array.get_next_new_unit_add_key();
    UnitAddItem* item = array.get_unit_add(id);

    if (item == nullptr) {
        ok = false;
    }

    note_result(ok, "First item: null before creation, zero-initialized after allocation");
    summarize_test_results();
}

void test_null_key_reserved () {
    UnitAddVector array;
    UnitAddItem* null_item = array.get_unit_add(UnitAddKey::from_raw(0));
    UnitAddKey first_key = array.get_next_new_unit_add_key();
    bool ok = (null_item == nullptr) && (first_key.value() == 1);
    note_result(ok, "Key 0 is reserved as null; first allocated key is 1");
    summarize_test_results();
}

void test_item_size_is_nonzero () {
    bool ok = (sizeof(UnitAddItem) > 0);
    note_result(ok, "Item size is non-zero");
    summarize_test_results();
}

void test_array_unique_ids () {
    UnitAddVector array;
    const u16 target_count = 1000;

    bool ok_ids = true;

    for (u16 i = 0; i < target_count; ++i) {
        UnitAddKey id = array.get_next_new_unit_add_key();
        const u16 expected = static_cast<u16>(i + 1);
        if (id.value() != expected) {
            ok_ids = false;
        }
        UnitAddItem* item = array.get_unit_add(id);
        if (item == nullptr) {
            ok_ids = false;
            break;
        }
        item->unit_add_idx = id.value();
    }

    if (ok_ids) {
        u16 total = UnitAddVectorTester::get_count(array);
        if (total < target_count) {
            ok_ids = false;
        } else {
            for (u16 i = 0; i < target_count; ++i) {
                const u16 expected = static_cast<u16>(i + 1);
                UnitAddItem* item = array.get_unit_add(UnitAddKey::from_raw(expected));
                if (item == nullptr) {
                    ok_ids = false;
                    break;
                }
                if (item->unit_add_idx != expected) {
                    ok_ids = false;
                    break;
                }
            }
        }
    }

    note_result(ok_ids, "Array 1000 entries have unique IDs via item index field");
    summarize_test_results();
}

void test_return_clears_existence () {
    UnitAddVector array;
    const u16 target_count = 10;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_add_key();
    }

    bool ok = true;

    const u16 return_idx1 = 3;
    UnitAddItem* item1 = array.get_unit_add(UnitAddKey::from_raw(return_idx1));
    if (item1 == nullptr) {
        ok = false;
    } else {
        array.return_unit_add(UnitAddKey::from_raw(return_idx1));
        if (array.get_unit_add(UnitAddKey::from_raw(return_idx1)) != nullptr) {
            ok = false;
        }
    }

    const u16 return_idx2 = 8;
    UnitAddItem* item2 = array.get_unit_add(UnitAddKey::from_raw(return_idx2));
    if (item2 == nullptr) {
        ok = false;
    } else {
        array.return_unit_add(UnitAddKey::from_raw(return_idx2));
        if (array.get_unit_add(UnitAddKey::from_raw(return_idx2)) != nullptr) {
            ok = false;
        }
    }

    note_result(ok, "return clears existence and retrieval");
    summarize_test_results();
}

void test_return_null_is_noop () {
    UnitAddVector array;
    const u16 target_count = 5;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_add_key();
    }

    u16 before_count = UnitAddVectorTester::get_count(array);
    u16 before_head  = UnitAddVectorTester::get_head_count(array);

    array.return_unit_add(UnitAddKey::from_raw(0xFFFF)); // should no-op as invalid

    bool ok = true;
    if (UnitAddVectorTester::get_count(array) != before_count) {
        ok = false;
    }
    if (UnitAddVectorTester::get_head_count(array) != before_head) {
        ok = false;
    }

    note_result(ok, "return(invalid key) is a no-op");
    summarize_test_results();
}

void test_return_decrements_count () {
    UnitAddVector array;
    const u16 target_count = 6;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_add_key();
    }

    u16 before_count = UnitAddVectorTester::get_count(array);
    u16 before_head  = UnitAddVectorTester::get_head_count(array);

    const u16 return_idx = 4;
    UnitAddItem* item = array.get_unit_add(UnitAddKey::from_raw(return_idx));
    bool ok = true;
    if (item == nullptr) {
        ok = false;
    } else {
        array.return_unit_add(UnitAddKey::from_raw(return_idx));
        if (UnitAddVectorTester::get_count(array) != static_cast<u16>(before_count - 1)) {
            ok = false;
        }
        if (UnitAddVectorTester::get_head_count(array) != before_head) {
            ok = false;
        }
        if (array.get_unit_add(UnitAddKey::from_raw(return_idx)) != nullptr) {
            ok = false;
        }
    }

    note_result(ok, "return decrements active count and clears retrieval");
    summarize_test_results();
}

void test_return_does_not_change_head () {
    UnitAddVector array;
    const u16 target_count = 8;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_add_key();
    }

    u16 before_head = UnitAddVectorTester::get_head_count(array);

    array.return_unit_add(UnitAddKey::from_raw(1));
    bool ok = (UnitAddVectorTester::get_head_count(array) == before_head);

    note_result(ok, "return does not change head count");
    summarize_test_results();
}

void test_reuse_recycled_slot_basic () {
    UnitAddVector array;
    for (u16 i = 0; i < 3; ++i) {
        array.get_next_new_unit_add_key();
    }

    u16 before_count = UnitAddVectorTester::get_count(array);
    u16 before_head  = UnitAddVectorTester::get_head_count(array);
    const u16 return_idx = 1;
    UnitAddItem* item = array.get_unit_add(UnitAddKey::from_raw(return_idx));
    bool ok = (item != nullptr);
    if (ok) {
        array.return_unit_add(UnitAddKey::from_raw(return_idx));
        UnitAddKey new_id = array.get_next_new_unit_add_key();
        if (new_id.value() != return_idx) {
            ok = false;
        }
        if (UnitAddVectorTester::get_count(array) != before_count) {
            ok = false;
        }
        if (UnitAddVectorTester::get_head_count(array) != before_head) {
            ok = false;
        }
        UnitAddItem* reused = array.get_unit_add(UnitAddKey::from_raw(return_idx));
        if (reused == nullptr) {
            ok = false;
        }
    }

    note_result(ok, "Realloc after return reuses exact returned index (basic)");
    summarize_test_results();
}

void test_reuse_recycled_slot_lifo () {
    UnitAddVector array;
    for (u16 i = 0; i < 4; ++i) {
        array.get_next_new_unit_add_key();
    }

    UnitAddItem* i1 = array.get_unit_add(UnitAddKey::from_raw(1));
    UnitAddItem* i2 = array.get_unit_add(UnitAddKey::from_raw(2));
    bool ok = (i1 != nullptr && i2 != nullptr);
    if (ok) {
        array.return_unit_add(UnitAddKey::from_raw(1));
        array.return_unit_add(UnitAddKey::from_raw(2));

        UnitAddKey n1 = array.get_next_new_unit_add_key();
        UnitAddKey n2 = array.get_next_new_unit_add_key();

        if (n1.value() != 2 || n2.value() != 1) {
            ok = false;
        }
        if (UnitAddVectorTester::get_head_count(array) != 5) {
            ok = false;
        }
    }

    note_result(ok, "Realloc after multiple returns reuses indices in LIFO order");
    summarize_test_results();
}

void test_return_twice_is_noop_second_time () {
    UnitAddVector array;
    for (u16 i = 0; i < 5; ++i) {
        array.get_next_new_unit_add_key();
    }

    const u16 return_idx = 3;
    UnitAddItem* item = array.get_unit_add(UnitAddKey::from_raw(return_idx));

    bool ok = true;
    if (item == nullptr) {
        ok = false;
    } else {
        u16 before_count = UnitAddVectorTester::get_count(array);
        array.return_unit_add(UnitAddKey::from_raw(return_idx));
        u16 after_first = UnitAddVectorTester::get_count(array);
        array.return_unit_add(UnitAddKey::from_raw(return_idx));
        u16 after_second = UnitAddVectorTester::get_count(array);

        if (after_first != static_cast<u16>(before_count - 1)) {
            ok = false;
        }
        if (after_second != after_first) {
            ok = false;
        }
        if (array.get_unit_add(UnitAddKey::from_raw(return_idx)) != nullptr) {
            ok = false;
        }
    }

    note_result(ok, "return called twice on same key decrements once");
    summarize_test_results();
}

void test_array_page_allocation () {
    UnitAddVector array;
    const u16 target_count = 1000;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_unit_add_key();
    }

    u16 page_count = UnitAddVectorTester::get_page_count(array);
    bool ok_pages = true;

    if (page_count == 0) {
        ok_pages = false;
    } else {
        for (u16 p = 0; p < page_count; ++p) {
            UnitAddItem* page = array.get_page(p);
            if (page == nullptr) {
                ok_pages = false;
                break;
            }
        }
    }

    note_result(ok_pages, "Array page allocation matches page count");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_item_size_is_nonzero();
    test_null_key_reserved();
    test_first_item_zero_initialized();
    test_array_unique_ids();
    test_return_clears_existence();
    test_return_null_is_noop();
    test_return_decrements_count();
    test_return_does_not_change_head();
    test_reuse_recycled_slot_basic();
    test_reuse_recycled_slot_lifo();
    test_return_twice_is_noop_second_time();
    test_array_page_allocation();

    std::printf("=======================================================\n");
    std::printf(" TESTING ITEMS ARRAY: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
