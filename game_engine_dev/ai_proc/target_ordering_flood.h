//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TARGET_ORDERING_FLOOD_H
#define TARGET_ORDERING_FLOOD_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - TargetOrderingFlood -
//================================================================================================================================
//
//  BFS over walkable tiles matching ownership filters (enemy / own / neutral). Seeds at (sx,sy).
//  Appends city indices in visit order into caller buffer; stops when cap is reached.
//  Cities are resolved from CityArray positions (not map overlays). Requires WhiteboardMng.
//
//================================================================================================================================

class TargetOrderingFlood {
public:
    TargetOrderingFlood ();

    void set_enemy (u8 seat);
    void set_own (u8 seat);
    void set_neutral (bool on);

    u16 fill (GameState& st, u16 sx, u16 sy, u16* out, u16 cap);

private:
    bool own_ok (u8 o) const;
    bool walk_ok (const GameState& st, u16 x, u16 y) const;

    u8 m_enemy;
    u8 m_own;
    bool m_neut;
};

#endif // TARGET_ORDERING_FLOOD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
