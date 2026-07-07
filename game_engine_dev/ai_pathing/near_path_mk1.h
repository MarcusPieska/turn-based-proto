//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef NEAR_PATH_MK1_H
#define NEAR_PATH_MK1_H

#include "game_array_simple.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - NearPathMk1 -
//================================================================================================================================
//
// - Short-range land pathing for near-unit repositioning and handoff checks
// - cheb: Chebyshev distance between two tiles
// - near_pt: true when cheb distance is at most one
// - can_reach: 4-neighbor BFS on walkable land (non-water, non-mountain)
// - one_step: first tile on a BFS path from start toward goal
// - Designed to not need: STL or persistent path storage; par[] alloc per query
//
//================================================================================================================================
//=> - Header -
//================================================================================================================================

class NearPathMk1 {
public:
    explicit NearPathMk1 (const GameArraySimple& map);
    static u32 cheb (u16 x0, u16 y0, u16 x1, u16 y1);
    bool near_pt (u16 x, u16 y, u16 tx, u16 ty) const;
    bool can_reach (u16 sx, u16 sy, u16 gx, u16 gy) const;
    bool one_step (u16 sx, u16 sy, u16 gx, u16 gy, u16& ox, u16& oy) const;

private:
    const GameArraySimple& m_map; // Terrain input
};

#endif // NEAR_PATH_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
