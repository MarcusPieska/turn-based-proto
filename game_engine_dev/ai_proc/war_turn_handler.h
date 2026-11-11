//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WAR_TURN_HANDLER_H
#define WAR_TURN_HANDLER_H

#include "army_turn_handler.h"
#include "game_primitives.h"
#include "sector_path_gradients.h"
#include "walk_p2p.h"

class ExpansionState;
class GameState;
class LandSectorAdj;
class MockMusterSiege;
class Whiteboard_2B;

struct BorderFront;
struct LandSectorSeedPt;
struct WarTargetPlan;

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
//  Per-seat war campaign: muster, form, sector-selector target pick, staging, reinforce. Owns WarArmy
//  group slots (k_grp_use). One sticky BorderFront slot per seat; k_front_use is retained but
//  multi-front peel is defunct. m_flag_i is the main army that carries the advance and updates
//  muster/staging on captures. Wars stay sequential (one WarSlot).
//
//================================================================================================================================

class WarTurnHandler {
public:
    static const u16 k_grp_cap = 1000u;
    static const u16 k_atk_cap = 20u;
    static const u16 k_grp_use = 20u; // Active partitioned groups; raise toward k_atk_cap when ready
    static const u16 k_front_use = 1u; // Defunct multi-front cap; single sticky front only
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
    static const Whiteboard_2B* sec_wb ();
    static u16 sec_n ();
    static const SectorPathGradients* path_grads (u16 pool_i);

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
    bool prep_mocks (MockMusterSiege& mock);
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
    u16 flag_i () const;
    u16 seat () const;
    u16 enemy () const;
    u16 staging_x () const;
    u16 staging_y () const;
    u16 target_x () const;
    u16 target_y () const;
    void clr_campaign ();
    void clr_army_flags ();
    void drop_army (u16 army_i);
    bool absorb_muster_to_flag ();

private:
    friend void prep_eval_muster_grps (const WarTurnHandler& camp);

    WarTurnHandler (const WarTurnHandler& o) = delete;
    WarTurnHandler (WarTurnHandler&& o) = delete;
    WarTurnHandler& operator= (const WarTurnHandler& o) = delete;
    WarTurnHandler& operator= (WarTurnHandler&& o) = delete;

    bool set_goal (u16 x1, u16 y1, u16 x2, u16 y2);
    bool set_goal_army (u16 army_i, u16 x2, u16 y2);
    void refill_grp (u16 head_idx);
    bool ensure_war (u16 other);
    bool apply_plan (const WarTargetPlan& plan);
    bool refill_targets (u16 enemy);
    bool next_enemy_city (u16 enemy, u16* ox, u16* oy);
    bool assign_army_city (u16 army_i, u16 enemy, u16* ox, u16* oy);
    bool park_army (u16 army_i);
    bool rejoin_flag (u16 army_i);
    bool merge_into_flag (u16 army_i);
    bool flag_xy (u16* ox, u16* oy) const;
    bool army_in_own_city (u16 army_i) const;
    void muster_pull_city_armies ();
    bool city_in_use (u16 x, u16 y, u16 skip_army) const;
    bool queue_pending () const;
    bool find_enemy_seed (u16 enemy, u16* ox, u16* oy) const;
    u16 grp_use_n () const;
    u32 army_size (u16 army_i) const;
    u16 weakest_army () const;
    bool start_reinforce (BorderFront* fronts, u16 n);
    bool finish_reinforce ();
    void path_drop (u16 army_i);

    GameState& m_st;
    u16 m_seat;
    WalkP2P m_mob; // Muster gradient to staging / reinforce city
    MusterCity m_grp[k_grp_cap];
    u16 m_grp_n;
    WarArmy m_atk[k_atk_cap]; // Partitioned groups; active count limited by k_grp_use
    u16 m_atk_n;
    u16 m_flag_i; // Main army index; sets muster on capture
    u16 m_tgts[k_tgt_cap]; // Sector-selector city indices
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
    u16 m_sector; // Active SectorFlood sector; U16_KEY_NULL if none
    bool m_assign_busy; // Last assign failed only because cities were in use
    bool m_ready;
    bool m_mob_ok;
};

#endif // WAR_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
