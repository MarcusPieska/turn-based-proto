//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SECTOR_SUPPORT_H
#define SECTOR_SUPPORT_H

#include "game_primitives.h"

class GameState;
class LandSectorNetwork;
class Whiteboard_2B;
struct LandSectorSeeds;

//================================================================================================================================
//=> - SectorPresence -
//================================================================================================================================
//
//  City ownership snapshot for one sector relative to a seat. m_rival is the other seat with the
//  most cities in the sector (lowest index on ties); U16_KEY_NULL if none.
//
//================================================================================================================================

struct SectorPresence {
    u16 m_own; // Cities owned by the queried seat
    u16 m_other; // Cities owned by anyone else
    u16 m_rival; // Strongest foreign seat; U16_KEY_NULL if none
};

//================================================================================================================================
//=> - SectorSupport -
//================================================================================================================================
//
//  Static helpers over GenLandSectors paint + LandSectorNetwork + city pool. bind wires non-owning
//  pointers; clr clears them. Full own means >=1 city and m_other == 0. Empty sectors are never full.
//  find_own_seed: own city in T, else two-sector probe from a controlled neighbor S into T that must
//  touch a non-lucky foe city in T (step: own / free / non-lucky). flood_foreign walks T only.
//
//================================================================================================================================

class SectorSupport {
public:
    static const u16 k_seat_cap = 256u;

    static bool bind (
        const GameState* st,
        const Whiteboard_2B* sec,
        const LandSectorSeeds* seeds,
        const LandSectorNetwork* net); 
    static void clr ();

    static SectorPresence sector_presence (u16 player, u16 sector);
    static u16 best_integral_sector (u16 player);
    static u16 best_defensible_sector (u16 player);
    static u16 get_shared_sector (u16 player);
    static u16 enemy_cities (u16 sector, u16 enemy, u16* tgts, u16 cap);
    static bool has_own_city (u16 seat, u16 sector);
    static bool find_own_seed (u16 seat, u16 sector, u16* ox, u16* oy);
    static u16 flood_foreign (u16 seat, u16 sector, u16 sx, u16 sy, u16* out, u16 cap);

private:
    SectorSupport () = delete;

    static bool ready ();
    static bool tally_all (u16 player, u16* own, u16* oth);
    static bool probe_adj (u16 seat, u16 src, u16 tgt, u16* ox, u16* oy);

    static const GameState* m_st; // Bound match state
    static const Whiteboard_2B* m_sec; // Sector id + 1 paint
    static const LandSectorSeeds* m_seeds; // Persistent sector array
    static const LandSectorNetwork* m_net; // Undirected adjacency
};

#endif // SECTOR_SUPPORT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
