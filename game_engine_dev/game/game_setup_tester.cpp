//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "game_setup.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";

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

void test_load_map () {
    GameSetup setup;
    const bool ok = setup.load_map(g_map_path);
    note_result(ok, "load_map");
    note_result(setup.get_map().data() != nullptr, "map data");
    note_result(setup.get_map().width() > 0, "map width");
    note_result(setup.get_map().height() > 0, "map height");
    summarize_test_results();
}

void test_set_player_count () {
    GameSetup setup;
    if (!setup.load_map(g_map_path)) {
        note_result(false, "load for set_player_count");
        summarize_test_results();
        return;
    }
    const u16 players = 8;
    const bool ok = setup.set_player_count(players);
    note_result(ok, "set_player_count");
    note_result(setup.get_starts().n == static_cast<u32>(players), "start count");
    summarize_test_results();
}

void test_set_player_count_needs_map () {
    GameSetup setup;
    note_result(!setup.set_player_count(4), "set_player_count without map");
    note_result(setup.get_starts().n == 0, "starts empty");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    test_load_map();
    test_set_player_count_needs_map();
    test_set_player_count();
    std::printf("=======================================================\n");
    std::printf(" TESTING GAME SETUP: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
