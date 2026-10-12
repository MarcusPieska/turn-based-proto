//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WAR_TURN_HANDLER_H
#define WAR_TURN_HANDLER_H

#include "army_turn_handler.h"
#include "game_primitives.h"
#include "path_gradients.h"
#include "walk_p2p.h"

class GameState;

//================================================================================================================================
//=> - MusterCity -
//================================================================================================================================
//
//  Per-city muster slot: departing group head plus front-line exposure distance (GenerateExposure).
//
//================================================================================================================================

struct MusterCity {
    u16 m_hd; // Departing group head; U16_KEY_NULL if none
    u8 m_exp; // Border distance on own tiles; GenerateExposure::k_none if unset/unreachable
};

//================================================================================================================================
//=> - WarTurnHandler -
//================================================================================================================================
//
//  Per-seat war campaign: muster, form, target pick, staging, and phase orchestration. Owns WarArmy
//  slots (one army for now); army march/assault/rejoin is ArmyTurnHandler::handle only. begin builds
//  GenLandSectors + LandSectorNetwork and binds SectorSupport; pick_enemy / refill_targets use
//  war_target_pick (default WarTargetKind::SecureCapital). Hot path: begin/clear with GameLoop;
//  handle per engaged AI seat.
//
//================================================================================================================================

class WarTurnHandler {
public:
    static const u16 k_grp_cap = 1000u;
    static const u16 k_atk_cap = 20u;
    static const u8 k_exp_lim = 10u;
    static const u16 k_tgt_cap = 256u;
    static const u16 k_seat_cap = 256u;

    WarTurnHandler (GameState& s, u16 seat);
    ~WarTurnHandler ();

    static bool begin (GameState& state);
    static void clear ();
    static bool pick_enemy (GameState& state, u16 seat, u16* out_enemy);
    static bool engage (GameState& state, u16 seat, u16 enemy);
    static bool is_engaged (u16 seat);
    static void handle (GameState& state);

    bool ok () const;
    bool make_muster_gradient (u16 x, u16 y);
    u16 do_total_muster ();
    bool determine_exposure (u16 enemy);
    bool walk_muster ();
    bool flatten_muster ();
    bool form_army ();
    bool set_target_city (u16 enemy, u16* ox, u16* oy);
    ArmyTurnRslt army_handle (u16 army_i = 0);
    void sync_from_army (u16 army_i = 0);
    bool stalled () const;
    u32 br_tot () const;
    u32 br_last () const;

    static bool pick_staging_city (const GameState& s, u16 seat, u16 enemy, u16* ox, u16* oy);

    u16 muster_n () const;
    void muster_units (u32* unit_n, u32* size_sum) const;
    bool is_exposed (u16 i) const;
    WarArmy* army (u16 i);
    const WarArmy* army (u16 i) const;
    u16 atk_hd (u16 i) const;
    u16 split_hd (u16 i) const;
    u16 seat () const;
    u16 enemy () const;
    u16 staging_x () const;
    u16 staging_y () const;
    u16 target_x () const;
    u16 target_y () const;
    void clr_campaign ();

private:
    friend void prep_eval_muster_grps (const WarTurnHandler& camp);

    WarTurnHandler (const WarTurnHandler& o) = delete;
    WarTurnHandler (WarTurnHandler&& o) = delete;
    WarTurnHandler& operator= (const WarTurnHandler& o) = delete;
    WarTurnHandler& operator= (WarTurnHandler&& o) = delete;

    bool set_goal (u16 x1, u16 y1, u16 x2, u16 y2);
    void refill_grp (u16 head_idx);
    bool refill_targets (u16 enemy);
    bool find_enemy_seed (u16 enemy, u16* ox, u16* oy) const;

    GameState& m_st;
    u16 m_seat;
    PathGradients m_paths; // Per-army march gradients; slot i for m_atk[i]
    WalkP2P m_mob; // Muster gradient to staging
    MusterCity m_grp[k_grp_cap];
    u16 m_grp_n;
    WarArmy m_atk[k_atk_cap]; // Partitioned armies; hot path still drives slot 0 until multi advance
    u16 m_atk_n;
    u16 m_tgts[k_tgt_cap]; // Flood-ordered enemy city indices
    u16 m_tgt_n; // Valid entries in m_tgts
    u16 m_tgt_i; // Next index to try
    u16 m_sx;
    u16 m_sy;
    u16 m_tx;
    u16 m_ty;
    u8 m_enemy; // Enemy seat for current target queue
    bool m_ready;
    bool m_mob_ok;
};

#endif // WAR_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
