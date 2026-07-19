//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef POINT_SEQ_FLOOD_WALKER_H
#define POINT_SEQ_FLOOD_WALKER_H

#include "game_primitives.h"
#include "sector_network_router.h"

//================================================================================================================================
//=> - PointSeqFloodWalker -
//================================================================================================================================
//
//  Point-to-point land walk. If src and dst fit in one PSF_WIN window, path with a single local
//  flood; otherwise hop SectorPath waypoints, each with a local window flood. Parent, queue, and
//  path buffers are fixed members (no heap / no per-tile flood).
//
//================================================================================================================================

#define PSF_WIN 15u
#define PSF_WIN_N (PSF_WIN * PSF_WIN)

class SectorNetwork;
class SectorNetworkRouter;

class PointSeqFloodWalker {
public:
    PointSeqFloodWalker ();
    ~PointSeqFloodWalker ();

    bool begin (const SectorNetwork& net, const SectorNetworkRouter& router, const u8* terr, u16 w, u16 h);
    bool start (u16 x0, u16 y0, u16 x1, u16 y1);
    bool step ();
    bool done () const;
    bool is_valid () const;
    u16 x () const;
    u16 y () const;

private:
    PointSeqFloodWalker (const PointSeqFloodWalker& other) = delete;
    PointSeqFloodWalker (PointSeqFloodWalker&& other) = delete;

    void clear ();
    bool pass (u16 x, u16 y) const;
    bool win_fit (u16 x0, u16 y0, u16 x1, u16 y1) const;
    void aim_next ();
    bool plan ();

    bool m_ok; // True after successful begin
    bool m_act; // True while a walk is in progress
    bool m_fin; // True after arriving at goal
    bool m_need; // True when a new flood plan is required
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
    u16 m_fn; // Planned tile count for current waypoint
    u16 m_fi; // Next index in the planned tile path
    const u8* m_terr; // Non-owning terrain row-major
    const SectorNetwork* m_net; // Non-owning sector lattice
    const SectorNetworkRouter* m_rt; // Non-owning hop router
    SectorPath m_path; // Packed sector hops for this walk
    u8 m_par[PSF_WIN_N]; // Flood parent indices in the window
    u8 m_q[PSF_WIN_N]; // Flood BFS queue (window indices)
    u16 m_sx[PSF_WIN_N]; // Planned step x coords
    u16 m_sy[PSF_WIN_N]; // Planned step y coords
};

#endif // POINT_SEQ_FLOOD_WALKER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
