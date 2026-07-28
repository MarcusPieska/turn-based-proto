//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_DISTANCE_P2P_H
#define GENERATE_DISTANCE_P2P_H

#include "game_primitives.h"

class GameState;
class RuntimeStatics;
class WalkP2P;
class Whiteboard_1B;

//================================================================================================================================
//=> - GenerateDistanceP2P -
//================================================================================================================================
//
//  Floods turn + arrival-rem boards from dst (UnitMovementMng, 8-neighbor). Enter only when local
//  budget >= cost; Dial buckets process lower turns first. With src: stops once src is settled and
//  no <=src-turn work remains. Without src: floods until the open set is empty. Optional 1B mask.
//
//================================================================================================================================

class GenerateDistanceP2P {
public:
    static const u16 k_turn_sent = 0xFFFFu;

    static bool generate (
        const GameState& s,
        const RuntimeStatics& st,
        u16 src_x,
        u16 src_y,
        u16 dst_x,
        u16 dst_y,
        WalkP2P& walk,
        const Whiteboard_1B* mask,
        u16* out_max);

    static bool generate (
        const GameState& s,
        const RuntimeStatics& st,
        u16 dst_x,
        u16 dst_y,
        WalkP2P& walk,
        const Whiteboard_1B* mask,
        u16* out_max);

private:
    GenerateDistanceP2P () = delete;
    GenerateDistanceP2P (const GenerateDistanceP2P& other) = delete;
    GenerateDistanceP2P (GenerateDistanceP2P&& other) = delete;
};

#endif // GENERATE_DISTANCE_P2P_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
