//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SETTLER_MISSION_MANAGER_H
#define SETTLER_MISSION_MANAGER_H

#include "game_primitives.h"
#include "point_seq_flood_walker.h"

class GameArraySimple;
class GenSettlementOrder;
class SectorNetwork;
class SectorNetworkRouter;

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

#define SMM_SLOT_N 20
#define SMM_WIN 21
#define SMM_SCAN 21
#define SMM_PATH_N 256

#define SMM_GO 0
#define SMM_FOUND 1
#define SMM_DROP 2

//================================================================================================================================
//=> - SettlerMissionManager -
//================================================================================================================================
//
//  One instance per player. Couples a settler to a planned site. If opp, local SMM_WIN window first,
//  else first remaining site on that player's order list. Pathing body is cpp-included from
//  impl/settler_mission_manager_impl_mkNN.cpp via SETTLER_MISSION_MANAGER_IMPL. Founds if on a free
//  planned tile; every SMM_SCAN steps, rescans the local window. Stack slots, cap SMM_SLOT_N.
//
//================================================================================================================================

class SettlerMissionManager {
public:
    SettlerMissionManager ();
    ~SettlerMissionManager () = default;
    bool begin (const SectorNetwork& net, const SectorNetworkRouter& rt, const u8* terr, u16 w, u16 h);
    void clr ();
    void opp (bool v);
    static void punch (GameArraySimple& map);
    bool ok () const;
    u16 idle () const;
    u16 asgn (GameArraySimple& map, const GenSettlementOrder& ord, u16 pl, u16 x, u16 y);
    u8 step (GameArraySimple& map, u16 s);
    void drop (u16 s);
    bool on (u16 s) const;
    u16 pl (u16 s) const;
    u16 x (u16 s) const;
    u16 y (u16 s) const;
    u16 tx (u16 s) const;
    u16 ty (u16 s) const;

private:
    SettlerMissionManager (const SettlerMissionManager& other) = delete;
    SettlerMissionManager& operator= (const SettlerMissionManager& other) = delete;
    SettlerMissionManager (SettlerMissionManager&& other) = delete;
    SettlerMissionManager& operator= (SettlerMissionManager&& other) = delete;

    struct Slot {
        PointSeqFloodWalker m_walk; // Sector-routed walk to m_tx, m_ty (mk01)
        u8 m_ps[SMM_PATH_N]; // Walk dirs (mk02)
        u16 m_pl; // Owner player
        u16 m_x; // Current settler x
        u16 m_y; // Current settler y
        u16 m_tx; // Claimed target x
        u16 m_ty; // Claimed target y
        u16 m_steps; // Steps since assign
        u16 m_pn; // Path dir count
        u16 m_pi; // Next path dir
        u8 m_on; // 1 if slot live
    };

    bool taken (u16 x, u16 y, u16 skip) const;
    bool ok_site (const GameArraySimple& map, u16 x, u16 y, u16 skip) const;
    bool wbeg (const SectorNetwork& net, const SectorNetworkRouter& rt, const u8* terr, u16 w, u16 h);
    bool aim (u16 s, u16 x0, u16 y0, u16 tx, u16 ty);
    bool wgo (u16 s);
    bool wdn (u16 s) const;
    bool pick_loc (GameArraySimple& map, u16 s, u16 x0, u16 y0);
    bool pick_ord (GameArraySimple& map, const GenSettlementOrder& ord, u16 s, u16 pl, u16 x0, u16 y0);
    void found (GameArraySimple& map, u16 s);
    void rel (u16 s);

    Slot m_slot[SMM_SLOT_N];
    u8 m_fs[SMM_SLOT_N]; // Free slot ids
    u8 m_fn; // Free slot count
    u32 m_oi; // First still-planned order index
    u16 m_w; // Map width
    u16 m_h; // Map height
    bool m_ok; // True after successful begin
    bool m_opp; // If set, local window scan on assign
};

#endif // SETTLER_MISSION_MANAGER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
