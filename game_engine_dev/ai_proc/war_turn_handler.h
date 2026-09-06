//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WAR_TURN_HANDLER_H
#define WAR_TURN_HANDLER_H

#include "game_primitives.h"
#include "walk_p2p.h"

class GameState;

//================================================================================================================================
//=> - WarAssault -
//================================================================================================================================
//
//  Result of assault_city: Ok captured, Stall out of assault MP with fight left, Fail hard stop.
//
//================================================================================================================================

enum class WarAssault : u8 {
    Ok = 0,
    Stall = 1,
    Fail = 2
};

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
//  Per-seat war turn handler: muster, form army, march, assault (CityAttackManager), claim city with
//  TileTransfer, two-step rejoin, then retarget via TargetOrderingFlood until no fight left or no
//  targets. Target list is a flood-ordered city-index queue from an enemy seed near staging.
//  Assault may Stall when offensive units remain but spent this assault's MP; owner retries.
//
//================================================================================================================================

class WarTurnHandler {
public:
    static const u16 k_grp_cap = 1000u;
    static const u16 k_atk_cap = 20u;
    static const u8 k_exp_lim = 10u;
    static const u16 k_tgt_cap = 256u;

    WarTurnHandler (GameState& s, u16 seat);
    ~WarTurnHandler ();

    bool ok () const;
    bool make_muster_gradient (u16 x, u16 y);
    u16 do_total_muster ();
    bool determine_exposure (u16 enemy);
    bool walk_muster ();
    bool form_army ();
    bool set_target_city (u16 enemy, u16* ox, u16* oy);
    bool walk_army ();
    WarAssault assault_city (u16 city_x, u16 city_y, u16 army_i = 0);
    bool rejoin_move (u16 army_i = 0);
    bool rejoin_link (u16 army_i = 0);
    u16 rest_heal (u16 army_i = 0);
    bool army_can_fight (u16 army_i = 0) const;
    bool can_cont (u16 army_i = 0) const;
    bool stalled () const;
    u32 br_tot () const;
    u32 br_last () const;

    static bool pick_staging_city (const GameState& s, u16 seat, u16 enemy, u16* ox, u16* oy);

    u16 muster_n () const;
    bool is_exposed (u16 i) const;
    u16 atk_hd (u16 i) const;
    u16 split_hd (u16 i) const;
    u16 staging_x () const;
    u16 staging_y () const;
    u16 target_x () const;
    u16 target_y () const;

private:
    WarTurnHandler (const WarTurnHandler& o) = delete;
    WarTurnHandler (WarTurnHandler&& o) = delete;
    WarTurnHandler& operator= (const WarTurnHandler& o) = delete;
    WarTurnHandler& operator= (WarTurnHandler&& o) = delete;

    bool set_goal (u16 x1, u16 y1, u16 x2, u16 y2);
    void refill_grp (u16 head_idx);
    void claim_city (u16 x, u16 y);
    bool refill_targets (u16 enemy);
    bool find_enemy_seed (u16 enemy, u16* ox, u16* oy) const;

    GameState& m_st;
    u16 m_seat;
    WalkP2P m_walk;
    WalkP2P m_mob;
    MusterCity m_grp[k_grp_cap];
    u16 m_grp_n;
    u16 m_atk[k_atk_cap];
    u16 m_atk_n;
    u16 m_split[k_atk_cap]; // Remainder outside city after army i split; U16_KEY_NULL if none
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
    bool m_stall; // True when last assault_city returned Stall
    u32 m_br_tot; // Last barrage pass total HP removed
    u32 m_br_last; // Last barrage pass final-shot HP removed
};

#endif // WAR_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
