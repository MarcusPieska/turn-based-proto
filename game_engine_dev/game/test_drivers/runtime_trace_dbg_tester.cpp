//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "runtime_trace_dbg.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* k_trace_path = "trace_test.trace";

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

//================================================================================================================================
//=> - Helpers -
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
        total_test_fails++;
        if (print_level > 0) {
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

static bool file_has_line (cstr path, cstr needle) {
    std::FILE* f = std::fopen(path, "r");
    if (f == nullptr) {
        return false;
    }
    char buf[256];
    bool found = false;
    while (std::fgets(buf, static_cast<int>(sizeof(buf)), f) != nullptr) {
        if (std::strstr(buf, needle) != nullptr) {
            found = true;
            break;
        }
    }
    std::fclose(f);
    return found;
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_trace_setup_and_city_foundation () {
    std::remove(k_trace_path);

    TRACE_SETUP((k_trace_path));
    TRACE_CITY_FOUNDATION((static_cast<u16>(12), static_cast<u16>(34), static_cast<u16>(2)));

    std::FILE* probe = std::fopen(k_trace_path, "r");
    note_result(probe != nullptr, "trace_setup creates trace file");
    if (probe != nullptr) {
        std::fclose(probe);
    }

    note_result(file_has_line(k_trace_path, "CITY_FOUNDATION:12:34:2"), "trace_city_foundation writes expected line");
    TRACE_NEW_TURN((static_cast<u16>(7)));
    note_result(file_has_line(k_trace_path, "NEW_TURN:7"), "trace_new_turn writes expected line");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_trace_setup_and_city_foundation();

    std::printf("=======================================================\n");
    std::printf(" TESTING RUNTIME TRACE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

