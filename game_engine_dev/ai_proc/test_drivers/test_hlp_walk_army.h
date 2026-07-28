//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TEST_HLP_WALK_ARMY_H
#define TEST_HLP_WALK_ARMY_H

#include "game_primitives.h"

class ConductCampaign;
class GameState;

//================================================================================================================================
//=> - TestHlpWalkArmy -
//================================================================================================================================
//
//  Walks the formed campaign army toward the target city until arrival/stall; times turns and
//  writes turns/turn_XXXX.ppm via turn_io (increments each frame).
//
//================================================================================================================================

class TestHlpWalkArmy {
public:
    static bool run (
        GameState& s,
        ConductCampaign& camp,
        const u16* ov,
        u16 pa,
        u16 pb,
        u16 ttx,
        u16 tty,
        cstr in_base,
        cstr out_base,
        bool print_turn,
        bool chain_check,
        bool chain_print_all,
        u32* turn_io);

private:
    TestHlpWalkArmy () = delete;
};

#endif // TEST_HLP_WALK_ARMY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
