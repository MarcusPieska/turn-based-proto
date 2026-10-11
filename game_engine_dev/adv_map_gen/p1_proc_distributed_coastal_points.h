//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_PROC_DISTRIBUTED_COASTAL_POINTS_H
#define P1_PROC_DISTRIBUTED_COASTAL_POINTS_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Proc_DistributedCoastalPoints -
//================================================================================================================================
//
//  Procedure (not gen/adj): for each landmass >= 10% of the largest, pick two coastal seeds on
//  roughly opposite sides. Fills trip.m_cen (stalls m_a/m_b are filled later by coastal flood).
//
//================================================================================================================================

struct P1_CoastPt {
    u16 m_x; // Tile x
    u16 m_y; // Tile y
};

struct P1_CoastPtTrip {
    P1_CoastPt m_cen; // Initial selected coastal seed
    P1_CoastPt m_a; // Fringe stall A (from coastal flood step)
    P1_CoastPt m_b; // Fringe stall B (from coastal flood step)
};

class P1_Proc_DistributedCoastalPoints {
public:
    explicit P1_Proc_DistributedCoastalPoints (const P1_RunPrm& prm);

    u32 generate (const u8* terrain, u16 w, u16 h, P1_CoastPtTrip* out, u32 cap);

private:
    P1_Proc_DistributedCoastalPoints (const P1_Proc_DistributedCoastalPoints& other) = delete;
    P1_Proc_DistributedCoastalPoints (P1_Proc_DistributedCoastalPoints&& other) = delete;

    P1_RunPrm m_prm; // Run size (and seed if needed later)
};

#endif // P1_PROC_DISTRIBUTED_COASTAL_POINTS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
