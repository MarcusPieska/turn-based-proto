//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "resource_io.h"

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
        std::printf("--------------------------------\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_res_io_path () {
    ResIoPath paths("../");
    note_result(paths.resources_path() != nullptr, "resources path set");
    std::FILE* f = std::fopen(paths.resources_path(), "rb");
    note_result(f != nullptr, "resources file exists");
    if (f != nullptr) {
        std::fclose(f);
    }
}

void test_res_io_load () {
    ResIoPath paths("../");
    ResIoFile file;
    note_result(file.load(paths.resources_path()), "load resources file");
    note_result(file.line_n() == 60u, "resource line count");
    note_result(file.line(0) != nullptr, "first line readable");
    note_result(file.line(59) != nullptr, "last line readable");
}

void test_suite_res_io () {
    test_res_io_path();
    summarize_test_results();
    test_res_io_load();
    summarize_test_results();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    test_suite_res_io();
    std::printf("=======================================================\n");
    std::printf(" TESTING RESOURCE IO: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
