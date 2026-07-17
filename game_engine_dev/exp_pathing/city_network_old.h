//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_NETWORK_H
#define CITY_NETWORK_H

#include "game_primitives.h"

//================================================================================================================================
//=> - CityNetLinks -
//================================================================================================================================
//
//  Parallel to city index: nearest same-landmass neighbor in each map quadrant within range
//  CN_LINK_RANGE (U16_KEY_NULL if none). Optional checks use CN_WIN flood or straight line.
//  Spans all cities; caller decides friendliness when consuming a path.
//
//================================================================================================================================

#define CN_LINK_RANGE 20u
#define CN_WIN 17u
#define CN_WIN_N (CN_WIN * CN_WIN)
#define CN_CELL CN_LINK_RANGE

enum CnCheckMode : u8 {
    CN_CHECK_OFF = 0,
    CN_CHECK_PATHING = 1,
    CN_CHECK_LINE = 2,
    CN_CHECK_HYBRID = 3
};

struct CityNetLinks {
    u16 m_ne; // Nearest city NE (dx >= 0, dy < 0)
    u16 m_nw; // Nearest city NW (dx < 0, dy <= 0)
    u16 m_se; // Nearest city SE (dx > 0, dy >= 0)
    u16 m_sw; // Nearest city SW (dx <= 0, dy > 0)
};

//================================================================================================================================
//=> - CityNetwork -
//================================================================================================================================
//
//  Sparse bidirectional city hop network over CityArray. Binding A<->B displaces prior partners,
//  who then recalculate that quadrant and may cascade further repairs. CN_CHECK_HYBRID tries
//  line then flood. Positions are read from CityArray; this class owns only link/scratch data.
//
//================================================================================================================================

struct LandMassIndexRslt;
class CityArray;

class CityNetwork {
public:
    CityNetwork ();
    ~CityNetwork ();

    bool begin (CityArray& cities, const LandMassIndexRslt& mass, const u8* terr, u16 tw, u16 th, CnCheckMode check);
    bool add (u16 city_idx);
    bool is_valid () const;
    u16 city_n () const;
    u32 line_n () const;
    u32 flood_n () const;
    const CityArray* cities () const;
    const CityNetLinks* links () const;

private:
    CityNetwork (const CityNetwork& other) = delete;
    CityNetwork (CityNetwork&& other) = delete;

    void clear ();
    void init_grid (u16 map_w, u16 map_h);
    void grid_ins (u16 i);
    u16 pos_x (u16 i) const;
    u16 pos_y (u16 i) const;
    bool pair_ok (u16 a, u16 b);
    bool would_accept (u16 owner, u16 cand) const;
    u16 find_best (u16 src, int q, bool need_accept);
    void enq_repair (u16 i, int q);
    void bind_pair (u16 a, u16 b);
    void drain_repairs ();
    void try_link (u16 src, u16 cand, int q);
    bool reach (u16 ax, u16 ay, u16 bx, u16 by);
    bool line_ok (u16 ax, u16 ay, u16 bx, u16 by);

    bool m_ok; // True after successful begin
    CnCheckMode m_check; // Neighbor validity mode
    u16 m_city_n; // Networked city count (indices 0 .. m_city_n-1)
    u16 m_cap; // Link table capacity (CityArray max slots)
    u16 m_tw; // Terrain width
    u16 m_th; // Terrain height
    u16 m_gw; // Spatial grid width in cells
    u16 m_gh; // Spatial grid height in cells
    u32 m_line_n; // Total line_ok calls
    u32 m_flood_n; // Total reach (flood) calls
    CityArray* m_cities; // Non-owning city storage
    CityNetLinks* m_links; // Quadrant neighbor table; length m_cap
    u16* m_masses; // Per-city landmass id; length m_cap
    u16* m_cell_head; // Grid cell list heads; length m_gw * m_gh
    u16* m_cell_next; // Next city in cell; length m_cap
    const LandMassIndexRslt* m_mass; // Non-owning landmass overlay
    const u8* m_terr; // Non-owning terrain classes; length m_tw * m_th
    u8 m_win[CN_WIN_N]; // Local flood window (0 block, 1 open, 2 visited)
    u8 m_qx[CN_WIN_N]; // Flood queue local x
    u8 m_qy[CN_WIN_N]; // Flood queue local y
    u16* m_rep_id; // Repair queue city indices
    u8* m_rep_q; // Repair queue quadrant ids
    u32 m_rep_n; // Repair queue length
    u32 m_rep_cap; // Repair queue capacity
};

#endif // CITY_NETWORK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
