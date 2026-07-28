//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_CHAIN_VALIDATION_H
#define UNIT_CHAIN_VALIDATION_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - UnitChainScan -
//================================================================================================================================
//
//  Counts from the vector pass and the map+group sweep so callers can compare both pictures.
//
//================================================================================================================================

struct UnitChainScan {
    u32 vec_n; // Live units in the vector
    u32 vec_pos; // Units with a real tile position
    u32 vec_tail; // Group tails (null xy)
    u32 map_stack; // Tile-stack entries only
    u32 map_full; // Stack entries plus every group-chain follower
    u32 err_n; // Error count printed this run
};

//================================================================================================================================
//=> - TestHlpUnitChainValidation -
//================================================================================================================================
//
//  Cross-checks unit vector vs map stacks/group chains: unique heads, no cycles, no positioned
//  units in tails, no multi-group membership. Mark arrays are zeroed once per run and kept for
//  the whole map pass so revisits catch cycles and duplicates. print_all dumps every check;
//  errors always print.
//
//================================================================================================================================

class TestHlpUnitChainValidation {
public:
    static bool run (GameState& s, bool print_all, UnitChainScan* out);

private:
    TestHlpUnitChainValidation () = delete;
};

#endif // UNIT_CHAIN_VALIDATION_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
