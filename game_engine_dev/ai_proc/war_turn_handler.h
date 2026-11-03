//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WAR_TURN_HANDLER_H
#define WAR_TURN_HANDLER_H

#include "army_turn_handler.h"
#include "game_primitives.h"
#include "path_gradients.h"
#include "walk_p2p.h"

class ExpansionState;
class GameState;
class LandSectorAdj;
struct BorderFront;
struct LandSectorSeedPt;

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
//  Per-seat war campaign: muster, form, target pick, staging, peel, reinforce. Owns WarArmy group
//  slots (k_grp_use) and sticky BorderFront[k_front_use] per seat (mk06). Peel opens a new front when
//  mk06 fills a need and a front has >1 group. Reinforce tops up the weakest group by total size.
//  k_front_use=1 + k_grp_use=10 matches pre-multi-front behavior (many groups, one front).
//  in_army = standing front troops; in_campaign = active war. Wars stay sequential (one WarSlot).
//
//================================================================================================================================

class WarTurnHandler {
public:
    static const u16 k_grp_cap = 1000u;
    static const u16 k_atk_cap = 20u;
    static const u16 k_grp_use = 20u; // Active partitioned groups; raise toward k_atk_cap when ready
    static const u16 k_front_use = 4u; // Active fronts / sticky BorderFront slots per seat
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
    static ExpansionState* sec_exp (u16 seat);
    static const LandSectorAdj* sec_adj ();
    static const LandSectorSeedPt* sec_pts ();

    bool ok () const;
    bool make_muster_gradient (u16 x, u16 y);
    u16 do_total_muster ();
    bool determine_exposure (u16 enemy);
    bool walk_muster ();
    bool flatten_muster ();
    bool form_army ();
    bool bind_standing (u16 i, u16 hd, u16 front);
    bool try_peel_fronts (BorderFront* fronts, u16 n);
    bool tick_reassemble (BorderFront* fronts, u16 n);
    bool tick_reinforce (BorderFront* fronts, u16 n);
    bool has_armies () const;
    bool set_target_city (u16 enemy, u16* ox, u16* oy);
    bool retarget_idle (u16 enemy, u16* ox, u16* oy);
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
    void clr_army_flags ();

private:
    friend void prep_eval_muster_grps (const WarTurnHandler& camp);

    WarTurnHandler (const WarTurnHandler& o) = delete;
    WarTurnHandler (WarTurnHandler&& o) = delete;
    WarTurnHandler& operator= (const WarTurnHandler& o) = delete;
    WarTurnHandler& operator= (WarTurnHandler&& o) = delete;

    bool set_goal (u16 x1, u16 y1, u16 x2, u16 y2);
    bool set_goal_army (u16 army_i, u16 x2, u16 y2);
    void refill_grp (u16 head_idx);
    bool refill_targets (u16 enemy);
    bool next_enemy_city (u16 enemy, u16* ox, u16* oy);
    bool city_in_use (u16 x, u16 y, u16 skip_army) const;
    bool find_enemy_seed (u16 enemy, u16* ox, u16* oy) const;
    u16 grp_use_n () const;
    u32 army_size (u16 army_i) const;
    u16 weakest_army () const;
    bool start_reinforce (BorderFront* fronts, u16 n);
    bool finish_reinforce ();

    GameState& m_st;
    u16 m_seat;
    PathGradients m_paths; // Per-group march gradients; slot i for m_atk[i]
    WalkP2P m_mob; // Muster gradient to staging / reinforce city
    MusterCity m_grp[k_grp_cap];
    u16 m_grp_n;
    WarArmy m_atk[k_atk_cap]; // Partitioned groups; active count limited by k_grp_use
    u16 m_atk_n;
    u16 m_tgts[k_tgt_cap]; // Flood-ordered enemy city indices
    u16 m_tgt_n; // Valid entries in m_tgts
    u16 m_tgt_i; // Next index to try
    u16 m_sx;
    u16 m_sy;
    u16 m_tx;
    u16 m_ty;
    u16 m_re_i; // Army index under reinforce; U16_KEY_NULL if none
    u16 m_hold_sx; // Staging xy saved around reinforce
    u16 m_hold_sy;
    u8 m_enemy; // Enemy seat for current target queue
    bool m_ready;
    bool m_mob_ok;
};

#endif // WAR_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
