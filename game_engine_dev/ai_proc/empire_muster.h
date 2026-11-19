//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EMPIRE_MUSTER_H
#define EMPIRE_MUSTER_H

#include "game_primitives.h"
#include "muster_dispatch.h"
#include "unit_add_vector_key.h"

class GameState;

//================================================================================================================================
//=> - EmpireMuster -
//================================================================================================================================
//
//  Thin empire-side muster policy: own-land pathing refresh (CityMusterGrad), assembly gate, and
//  handoff of a group head into an owned MusterDispatch. One primary MusterDispatch by value
//  today (no owning pointers). Planned: k_pt_cap muster points, each with up to
//  MusterDispatch::k_front_cap dispatches; CityMusterGrad::flood_multi will feed parallel sources.
//
//================================================================================================================================

class EmpireMuster {
public:
    static const u16 k_pt_cap = 4u; // Planned parallel muster points per seat
    static const u16 k_seat_cap = 256u;

    EmpireMuster ();
    ~EmpireMuster ();

    static bool setup (u16 min_n);
    static u16 min_n ();
    static bool begin (GameState& st);
    static void clear ();
    static void handle (GameState& st);
    static EmpireMuster* at (u16 seat);

    void bind (GameState& st, u16 seat);
    void clr ();
    bool ok () const;
    bool update_pathing ();
    bool check_dispatch () const;
    bool try_dispatch ();
    bool set_front_dest (u16 dx, u16 dy);
    MusterDispatch& disp ();
    const MusterDispatch& disp () const;
    u16 seat () const;
    u16 pt_n () const;
    u16 disp_n () const;

private:
    EmpireMuster (const EmpireMuster& o) = delete;
    EmpireMuster& operator= (const EmpireMuster& o) = delete;

    u16 count_cap () const;
    bool peel_cap (UnitAddKey* out_hd);

    GameState* m_st; // Bound match state
    u16 m_seat; // Owner seat
    MusterDispatch m_disp; // Primary dispatch; future: per-point / per-front array
    u16 m_pt_n; // Active muster points; 1 now, up to k_pt_cap later
    u16 m_disp_n; // Active dispatches on primary point; 1 now
    bool m_ok; // Bound and ready
};

#endif // EMPIRE_MUSTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
