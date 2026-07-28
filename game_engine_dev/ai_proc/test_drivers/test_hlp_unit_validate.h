//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TEST_HLP_UNIT_VALIDATE_H
#define TEST_HLP_UNIT_VALIDATE_H

#include "game_primitives.h"

class GameState;
class Whiteboard_1B;

//================================================================================================================================
//=> - TestHlpUnitValidate -
//================================================================================================================================
//
//  Prints unit tallies and writes PPM overlays. Image helpers take in_base (map seed folder
//  with terrain.ppm) and out_base (scenario output folder). Campaign frames go to turns/.
//
//================================================================================================================================

class TestHlpUnitValidate {
public:
    static void print_counts (const GameState& s);
    static void print_at_xy (const GameState& s, u16 x, u16 y);

    static bool ensure_out_dirs (cstr out_base);

    static bool write_sectors (
        const GameState& s,
        const u16* ov,
        u16 pa,
        u16 pb,
        u16 stx,
        u16 sty,
        cstr in_base,
        cstr out_base);

    static bool write_access (
        const GameState& s,
        const Whiteboard_1B& msk,
        u16 view,
        u16 stx,
        u16 sty,
        cstr in_base,
        cstr out_base);

    static bool write_turn (
        const GameState& s,
        const u16* ov,
        u16 pa,
        u16 pb,
        u16 focus_x,
        u16 focus_y,
        u32 turn,
        cstr in_base,
        cstr out_base);

private:
    TestHlpUnitValidate () = delete;
};

#endif // TEST_HLP_UNIT_VALIDATE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
