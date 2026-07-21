//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ANCHORED_FARMER_H
#define ANCHORED_FARMER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - AnchoredFarmer -
//================================================================================================================================
//
//  Worker AI with no per-worker state. Each turn: local CircArea scan for nearest friendly city,
//  then (wx-cx, wy-cy) indexes a static successor step map over the work disk (r=4 / map r=3).
//  Farms owned empty plains via StdAddHelper; advances by the looked-up 8-adj step. Tables from gen_anchored_farmer_path.py.
//
//================================================================================================================================

class AnchoredFarmer {
public:
    AnchoredFarmer () = delete;

    static bool begin (GameState& state);
    static void clear ();
    static void handle (GameState& state, u16 unit_idx);
};

#endif // ANCHORED_FARMER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
