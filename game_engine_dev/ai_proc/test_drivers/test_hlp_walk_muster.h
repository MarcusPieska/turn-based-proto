//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TEST_HLP_WALK_MUSTER_H
#define TEST_HLP_WALK_MUSTER_H

#include "game_primitives.h"

class ConductCampaign;
class GameState;

//================================================================================================================================
//=> - TestHlpWalkMuster -
//================================================================================================================================
//
//  Runs ConductCampaign muster walk until arrival/stall: times each turn, optionally prints
//  per-turn staging counts, writes turns/turn_XXXX.ppm via turn_io (increments each frame).
//
//================================================================================================================================

class TestHlpWalkMuster {
public:
    static bool run (
        GameState& s,
        ConductCampaign& camp,
        const u16* ov,
        u16 pa,
        u16 pb,
        u16 stx,
        u16 sty,
        cstr in_base,
        cstr out_base,
        bool print_turn,
        bool chain_check,
        bool chain_print_all,
        u32* turn_io);

private:
    TestHlpWalkMuster () = delete;
};

#endif // TEST_HLP_WALK_MUSTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
