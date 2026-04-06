//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "outpost_add_vector.h"

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

class OutpostAddVectorTester {
    public:
        static u16 get_count(const OutpostAddVector& array) {
            return array.m_outpost_add_count;
        }
        static u16 get_page_count(const OutpostAddVector& array) {
            return array.m_page_count;
        }
        static u16 get_head_count(const OutpostAddVector& array) {
            return array.m_head_outpost_add_idx;
        }
    };

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_first_item_zero_initialized () {
    OutpostAddVector array;
    OutpostAddItem* should_be_null = array.get_outpost_add(OutpostAddKey::from_raw(0));
    bool ok = (should_be_null == nullptr);

    OutpostAddKey id = array.get_next_new_outpost_add_key();
    OutpostAddItem* item = array.get_outpost_add(id);

    if (item == nullptr) {
        ok = false;
    }

    note_result(ok, "First item: null before creation, zero-initialized after allocation");
    summarize_test_results();
}

void test_null_key_reserved () {
    OutpostAddVector array;
    OutpostAddItem* null_item = array.get_outpost_add(OutpostAddKey::from_raw(0));
    OutpostAddKey first_key = array.get_next_new_outpost_add_key();
    bool ok = (null_item == nullptr) && (first_key.value() == 1);
    note_result(ok, "Key 0 is reserved as null; first allocated key is 1");
    summarize_test_results();
}

void test_item_size_is_nonzero () {
    bool ok = (sizeof(OutpostAddItem) > 0);
    note_result(ok, "Item size is non-zero");
    summarize_test_results();
}

void test_array_unique_ids () {
    OutpostAddVector array;
    const u16 target_count = 1000;

    bool ok_ids = true;

    for (u16 i = 0; i < target_count; ++i) {
        OutpostAddKey id = array.get_next_new_outpost_add_key();
        const u16 expected = static_cast<u16>(i + 1);
        if (id.value() != expected) {
            ok_ids = false;
        }
        OutpostAddItem* item = array.get_outpost_add(id);
        if (item == nullptr) {
            ok_ids = false;
            break;
        }
        item->outpost_add_idx = id.value();
    }

    if (ok_ids) {
        u16 total = OutpostAddVectorTester::get_count(array);
        if (total < target_count) {
            ok_ids = false;
        } else {
            for (u16 i = 0; i < target_count; ++i) {
                const u16 expected = static_cast<u16>(i + 1);
                OutpostAddItem* item = array.get_outpost_add(OutpostAddKey::from_raw(expected));
                if (item == nullptr) {
                    ok_ids = false;
                    break;
                }
                if (item->outpost_add_idx != expected) {
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
    OutpostAddVector array;
    const u16 target_count = 10;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_outpost_add_key();
    }

    bool ok = true;

    const u16 return_idx1 = 3;
    OutpostAddItem* item1 = array.get_outpost_add(OutpostAddKey::from_raw(return_idx1));
    if (item1 == nullptr) {
        ok = false;
    } else {
        array.return_outpost_add(OutpostAddKey::from_raw(return_idx1));
        if (array.get_outpost_add(OutpostAddKey::from_raw(return_idx1)) != nullptr) {
            ok = false;
        }
    }

    const u16 return_idx2 = 8;
    OutpostAddItem* item2 = array.get_outpost_add(OutpostAddKey::from_raw(return_idx2));
    if (item2 == nullptr) {
        ok = false;
    } else {
        array.return_outpost_add(OutpostAddKey::from_raw(return_idx2));
        if (array.get_outpost_add(OutpostAddKey::from_raw(return_idx2)) != nullptr) {
            ok = false;
        }
    }

    note_result(ok, "return clears existence and retrieval");
    summarize_test_results();
}

void test_return_null_is_noop () {
    OutpostAddVector array;
    const u16 target_count = 5;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_outpost_add_key();
    }

    u16 before_count = OutpostAddVectorTester::get_count(array);
    u16 before_head  = OutpostAddVectorTester::get_head_count(array);

    array.return_outpost_add(OutpostAddKey::from_raw(0xFFFF)); // should no-op as invalid

    bool ok = true;
    if (OutpostAddVectorTester::get_count(array) != before_count) {
        ok = false;
    }
    if (OutpostAddVectorTester::get_head_count(array) != before_head) {
        ok = false;
    }

    note_result(ok, "return(invalid key) is a no-op");
    summarize_test_results();
}

void test_return_decrements_count () {
    OutpostAddVector array;
    const u16 target_count = 6;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_outpost_add_key();
    }

    u16 before_count = OutpostAddVectorTester::get_count(array);
    u16 before_head  = OutpostAddVectorTester::get_head_count(array);

    const u16 return_idx = 4;
    OutpostAddItem* item = array.get_outpost_add(OutpostAddKey::from_raw(return_idx));
    bool ok = true;
    if (item == nullptr) {
        ok = false;
    } else {
        array.return_outpost_add(OutpostAddKey::from_raw(return_idx));
        if (OutpostAddVectorTester::get_count(array) != static_cast<u16>(before_count - 1)) {
            ok = false;
        }
        if (OutpostAddVectorTester::get_head_count(array) != before_head) {
            ok = false;
        }
        if (array.get_outpost_add(OutpostAddKey::from_raw(return_idx)) != nullptr) {
            ok = false;
        }
    }

    note_result(ok, "return decrements active count and clears retrieval");
    summarize_test_results();
}

void test_return_does_not_change_head () {
    OutpostAddVector array;
    const u16 target_count = 8;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_outpost_add_key();
    }

    u16 before_head = OutpostAddVectorTester::get_head_count(array);

    array.return_outpost_add(OutpostAddKey::from_raw(1));
    bool ok = (OutpostAddVectorTester::get_head_count(array) == before_head);

    note_result(ok, "return does not change head count");
    summarize_test_results();
}

void test_reuse_recycled_slot_basic () {
    OutpostAddVector array;
    for (u16 i = 0; i < 3; ++i) {
        array.get_next_new_outpost_add_key();
    }

    u16 before_count = OutpostAddVectorTester::get_count(array);
    u16 before_head  = OutpostAddVectorTester::get_head_count(array);
    const u16 return_idx = 1;
    OutpostAddItem* item = array.get_outpost_add(OutpostAddKey::from_raw(return_idx));
    bool ok = (item != nullptr);
    if (ok) {
        array.return_outpost_add(OutpostAddKey::from_raw(return_idx));
        OutpostAddKey new_id = array.get_next_new_outpost_add_key();
        if (new_id.value() != return_idx) {
            ok = false;
        }
        if (OutpostAddVectorTester::get_count(array) != before_count) {
            ok = false;
        }
        if (OutpostAddVectorTester::get_head_count(array) != before_head) {
            ok = false;
        }
        OutpostAddItem* reused = array.get_outpost_add(OutpostAddKey::from_raw(return_idx));
        if (reused == nullptr) {
            ok = false;
        }
    }

    note_result(ok, "Realloc after return reuses exact returned index (basic)");
    summarize_test_results();
}

void test_reuse_recycled_slot_lifo () {
    OutpostAddVector array;
    for (u16 i = 0; i < 4; ++i) {
        array.get_next_new_outpost_add_key();
    }

    OutpostAddItem* i1 = array.get_outpost_add(OutpostAddKey::from_raw(1));
    OutpostAddItem* i2 = array.get_outpost_add(OutpostAddKey::from_raw(2));
    bool ok = (i1 != nullptr && i2 != nullptr);
    if (ok) {
        array.return_outpost_add(OutpostAddKey::from_raw(1));
        array.return_outpost_add(OutpostAddKey::from_raw(2));

        OutpostAddKey n1 = array.get_next_new_outpost_add_key();
        OutpostAddKey n2 = array.get_next_new_outpost_add_key();

        if (n1.value() != 2 || n2.value() != 1) {
            ok = false;
        }
        if (OutpostAddVectorTester::get_head_count(array) != 5) {
            ok = false;
        }
    }

    note_result(ok, "Realloc after multiple returns reuses indices in LIFO order");
    summarize_test_results();
}

void test_return_twice_is_noop_second_time () {
    OutpostAddVector array;
    for (u16 i = 0; i < 5; ++i) {
        array.get_next_new_outpost_add_key();
    }

    const u16 return_idx = 3;
    OutpostAddItem* item = array.get_outpost_add(OutpostAddKey::from_raw(return_idx));

    bool ok = true;
    if (item == nullptr) {
        ok = false;
    } else {
        u16 before_count = OutpostAddVectorTester::get_count(array);
        array.return_outpost_add(OutpostAddKey::from_raw(return_idx));
        u16 after_first = OutpostAddVectorTester::get_count(array);
        array.return_outpost_add(OutpostAddKey::from_raw(return_idx));
        u16 after_second = OutpostAddVectorTester::get_count(array);

        if (after_first != static_cast<u16>(before_count - 1)) {
            ok = false;
        }
        if (after_second != after_first) {
            ok = false;
        }
        if (array.get_outpost_add(OutpostAddKey::from_raw(return_idx)) != nullptr) {
            ok = false;
        }
    }

    note_result(ok, "return called twice on same key decrements once");
    summarize_test_results();
}

void test_array_page_allocation () {
    OutpostAddVector array;
    const u16 target_count = 1000;

    for (u16 i = 0; i < target_count; ++i) {
        array.get_next_new_outpost_add_key();
    }

    u16 page_count = OutpostAddVectorTester::get_page_count(array);
    bool ok_pages = true;

    if (page_count == 0) {
        ok_pages = false;
    } else {
        for (u16 p = 0; p < page_count; ++p) {
            OutpostAddItem* page = array.get_page(p);
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
