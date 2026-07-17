//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef POINT_SEQ_WALKER_H
#define POINT_SEQ_WALKER_H

#include "game_primitives.h"
#include "sector_network_router.h"

//================================================================================================================================
//=> - PointSeqWalker -
//================================================================================================================================
//
//  Greedy local walk along a SectorPath: tile steps toward successive sector centers, then the
//  final goal. Optimal routes are not required. No heap on the hot path; SectorPath lives here.
//  When hill-climbing stalls, a tiny stack BFS (escape) finds one improving step.
//
//================================================================================================================================

class SectorNetwork;
class SectorNetworkRouter;

class PointSeqWalker {
public:
    PointSeqWalker ();
    ~PointSeqWalker ();

    bool begin (const SectorNetwork& net, const SectorNetworkRouter& router, const u8* terr, u16 w, u16 h);
    bool start (u16 x0, u16 y0, u16 x1, u16 y1);
    bool step ();
    bool done () const;
    bool is_valid () const;
    u16 x () const;
    u16 y () const;

private:
    PointSeqWalker (const PointSeqWalker& other) = delete;
    PointSeqWalker (PointSeqWalker&& other) = delete;

    void clear ();
    bool pass (u16 x, u16 y) const;
    void aim_next ();
    bool greedy ();
    bool escape ();

    bool m_ok; // True after successful begin
    bool m_act; // True while a walk is in progress
    bool m_fin; // True after arriving at goal
    u16 m_w; // Map width
    u16 m_h; // Map height
    u16 m_x; // Current x
    u16 m_y; // Current y
    u16 m_gx; // Goal x
    u16 m_gy; // Goal y
    u16 m_tx; // Current waypoint x
    u16 m_ty; // Current waypoint y
    u16 m_sid; // Sector id along the hop path
    u8 m_pi; // Next hop index in m_path
    u32 m_best; // Best dist2 to current waypoint so far (escape floor)
    const u8* m_terr; // Non-owning terrain row-major
    const SectorNetwork* m_net; // Non-owning sector lattice
    const SectorNetworkRouter* m_rt; // Non-owning hop router
    SectorPath m_path; // Packed sector hops for this walk
};

#endif // POINT_SEQ_WALKER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
