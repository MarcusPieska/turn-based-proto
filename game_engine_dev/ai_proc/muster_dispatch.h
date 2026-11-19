//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MUSTER_DISPATCH_H
#define MUSTER_DISPATCH_H

#include "game_primitives.h"
#include "sector_path_gradients.h"
#include "unit_add_vector_key.h"

class GameState;

//================================================================================================================================
//=> - MusterDispatch -
//================================================================================================================================
//
//  Capital-to-front (or muster-point-to-front) dispatch: packs a path tape, accepts a unit-group
//  head from EmpireMuster, and walks it to the front dest. Owned by value under EmpireMuster (no
//  owning pointers). Near-term: one dispatch per empire muster. Planned: several per muster point
//  (e.g. E/W/N/S fronts) selected by m_front. EmpireMuster may later own several muster points,
//  each with one or more MusterDispatch instances.
//
//================================================================================================================================

class MusterDispatch {
public:
    static const u16 k_front_cap = 4u; // Planned E/W/N/S (or similar) front slots

    MusterDispatch ();
    ~MusterDispatch ();

    void clr ();
    bool set_dest (GameState& st, u16 seat, u16 dx, u16 dy);
    void drop ();
    bool busy () const;
    bool ok () const;
    bool take_hd (UnitAddKey hd);
    UnitAddKey hd () const;
    bool walk_tick (GameState& st, u16 seat);
    SectorPathGradients::StepRes peek (GameState& st, u16 x, u16 y) const;
    u16 dest_x () const;
    u16 dest_y () const;
    u16 front () const;
    void set_front (u16 fi);
    SectorPathGradients* feed ();
    u16 feed_i () const;

private:
    MusterDispatch (const MusterDispatch& o) = delete;
    MusterDispatch& operator= (const MusterDispatch& o) = delete;

    SectorPathGradients* m_feed; // Non-owning SPG bank; tape released in drop()
    u16 m_feed_i; // Slot in m_feed; U16_KEY_NULL if none
    u16 m_hd; // Accepted group head; U16_KEY_NULL if none
    u16 m_dx; // Front dest x
    u16 m_dy; // Front dest y
    u16 m_front; // Front index; 0 now, up to k_front_cap later
    bool m_ok; // True while a path tape is leased
};

#endif // MUSTER_DISPATCH_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
