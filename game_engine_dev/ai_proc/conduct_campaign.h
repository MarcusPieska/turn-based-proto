//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CONDUCT_CAMPAIGN_H
#define CONDUCT_CAMPAIGN_H

#include "game_primitives.h"
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
//=> - ConductCampaign -
//================================================================================================================================
//
//  Per-seat campaign: muster, form army, march, assault (AttackCity), claim city, two-step rejoin
//  of the split remainder, then retarget from the captured city until no fight left or no targets.
//
//================================================================================================================================

class ConductCampaign {
public:
    static const u16 k_grp_cap = 1000u;
    static const u16 k_atk_cap = 20u;
    static const u8 k_exp_lim = 10u;

    ConductCampaign (GameState& s, u16 seat);
    ~ConductCampaign ();

    bool ok () const;
    bool make_muster_gradient (u16 x, u16 y);
    u16 do_total_muster ();
    bool determine_exposure (u16 enemy);
    bool walk_muster ();
    bool form_army ();
    bool set_target_city (u16 enemy, u16* ox, u16* oy);
    bool walk_army ();
    bool assault_city (u16 city_x, u16 city_y, u16 army_i = 0);
    bool rejoin_move (u16 army_i = 0);
    bool rejoin_link (u16 army_i = 0);
    bool army_can_fight (u16 army_i = 0) const;

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
    ConductCampaign (const ConductCampaign& o) = delete;
    ConductCampaign (ConductCampaign&& o) = delete;
    ConductCampaign& operator= (const ConductCampaign& o) = delete;
    ConductCampaign& operator= (ConductCampaign&& o) = delete;

    bool set_goal (u16 x1, u16 y1, u16 x2, u16 y2);
    void refill_grp (u16 head_idx);
    void claim_city (u16 x, u16 y);
    static bool pick_target_city (const GameState& s, u16 seat, u16 enemy, u16 from_x, u16 from_y, u16* ox, u16* oy);

    GameState& m_st;
    u16 m_seat;
    WalkP2P m_walk;
    WalkP2P m_mob;
    MusterCity m_grp[k_grp_cap];
    u16 m_grp_n;
    u16 m_atk[k_atk_cap];
    u16 m_atk_n;
    u16 m_split[k_atk_cap]; // Remainder outside city after army i split; U16_KEY_NULL if none
    u16 m_sx;
    u16 m_sy;
    u16 m_tx;
    u16 m_ty;
    bool m_ready;
    bool m_mob_ok;
};

#endif // CONDUCT_CAMPAIGN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
