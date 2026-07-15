//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "starting_point_generator.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool validate_results (const StartingPointGenerator& gen) {
    const u32 got = gen.pick_count();
    const u16 want = gen.pick_target();
    u32 expect = static_cast<u32>(want);
    if (gen.candidate_count() < expect) {
        expect = gen.candidate_count();
    }
    if (got != expect) {
        std::printf("\n*** VALIDATE FAILED: pick count %u (expected %u, target %u)\n",
            got, expect, want);
        return false;
    }
    if (!gen.picks_are_start_land()) {
        std::printf("\n*** VALIDATE FAILED: pick not on valid start tile\n");
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
