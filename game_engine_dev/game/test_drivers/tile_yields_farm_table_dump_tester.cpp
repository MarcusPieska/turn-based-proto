//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "map_overlay_enum.h"
#include "runtime_static_loader.h"
#include "tile_yields.h"
#include "tile_yields_imp_dump.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";

#ifndef TILE_YIELDS_IMPL_TAG
#define TILE_YIELDS_IMPL_TAG "mk01"
#endif

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

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

//================================================================================================================================
//=> - TileYieldsImpDump::run -
//================================================================================================================================

int TileYieldsImpDump::run () {
    std::printf("tile_yields impl tag=%s\n", TILE_YIELDS_IMPL_TAG);
    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "load runtime statics");
    if (total_test_fails > 0) {
        return 1;
    }
    note_result(TileYields::setup(loader.statics()), "setup tile yields");
    if (total_test_fails > 0) {
        return 1;
    }
    const u16 farm = static_cast<u16>(MapOverlay::Farm);
    const u16 nz = dump_job(farm, stdout);
    note_result(nz >= 4u, "farm dump has clim+riv nonzero slots");
    std::printf("=======================================================\n");
    std::printf(" TESTING FARM TABLE DUMP: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    return TileYieldsImpDump::run();
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
