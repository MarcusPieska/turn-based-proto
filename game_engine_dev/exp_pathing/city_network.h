//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_NETWORK_H
#define CITY_NETWORK_H

#include "city.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - CityNetwork -
//================================================================================================================================
//
//  Builds bidirectional city hop links stored on each City. A new city force-binds its nearest
//  quadrant neighbors (displacing prior partners). Displaced cities are tracked in a small stack
//  buffer (id, cleared slot, prior CityNetLinks); repairs only rebind among that set.
//
//================================================================================================================================

#define CN_LINK_RANGE 25u
#define CN_WIN (CN_LINK_RANGE * 2u + 1u)
#define CN_WIN_N (CN_WIN * CN_WIN)
#define CN_ADD_TYP_CITY 1u
#define CN_DISC_MAX 16u

class CityArray;
class GameArraySimple;

struct CnDisc {
    u16 m_id; // Disconnected city index
    u8 m_q; // Quadrant slot cleared on that city
    CityNetLinks m_was; // Link table before that clear
};

class CityNetwork {
public:
    CityNetwork ();
    ~CityNetwork ();

    bool begin (CityArray& cities, GameArraySimple& map);
    bool add (u16 city_idx);
    bool is_valid () const;
    u16 city_n () const;
    u32 flood_n () const;
    const CityArray* cities () const;

private:
    CityNetwork (const CityNetwork& other) = delete;
    CityNetwork (CityNetwork&& other) = delete;

    void clear ();
    u16 pos_x (u16 i) const;
    u16 pos_y (u16 i) const;
    bool tile_pass (u16 x, u16 y) const;
    bool in_rng (u16 a, u16 b) const;
    u8 link_n (u16 i) const;
    void push_disc (CnDisc* disc, u32* disc_n, u16 id, int q);
    u16 pick_rep (u16 src, int q, const CnDisc* disc, u32 disc_n) const;
    void flood_link (u16 src);
    void bind_pair (u16 a, u16 b, CnDisc* disc, u32* disc_n);
    void drain_repairs (CnDisc* disc, u32* disc_n);

    bool m_ok; // True after successful begin
    u16 m_city_n; // Networked city count (indices 0 .. m_city_n-1)
    u16 m_cap; // Max city index capacity
    u32 m_flood_n; // Total local-window floods
    CityArray* m_cities; // Non-owning city storage (links live on City)
    GameArraySimple* m_map; // Non-owning game map
    u16 m_wd[CN_WIN_N]; // Flood window distance; U16_KEY_NULL = unseen
    u8 m_qx[CN_WIN_N]; // Flood queue local x
    u8 m_qy[CN_WIN_N]; // Flood queue local y
};

#endif // CITY_NETWORK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
