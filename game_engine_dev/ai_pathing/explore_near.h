//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EXPLORE_NEAR_H
#define EXPLORE_NEAR_H

#include "game_array_simple.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"
#include "near_path_mk1.h"
#include "walk_near_mk1.h"

class WalkCoastMk1;
class WalkMtnMk1;
class WalkRiverMk2;

//================================================================================================================================
//=> - ExOptMode -
//================================================================================================================================

enum ExOptMode : u16 {
    EX_OPT_IGNORE = 0u,
    EX_OPT_PURSUE = 1u,
    EX_OPT_RECENTER = 2u
};

//================================================================================================================================
//=> - ExOpt -
//================================================================================================================================

struct ExOpt {
    u16 mode;
    u16 param;
};

//================================================================================================================================
//=> - ExploreNear -
//================================================================================================================================
//
// - Compound near explorer: WalkNearMk1 core with optional coast, mtn, river side quests
// - ExOpt.mode: 0=ignore, 1=pursue and resume near at side-quest end, 2=pursue and recenter near home
// - ExOpt.param: river moves per tick when in river phase
// - Stops when near is exhausted; side quests not taken after near done
// - can_ai_start_from: 0=no; 1-2=near; 10-11=coast; 20-21=mtn; 30-31=river (when opt enabled)
//
//================================================================================================================================
//=> - Header -
//================================================================================================================================

class ExploreNear {
public:
    static u16 can_ai_start_from (
        const GameArraySimple& map,
        MapBitOverlay& ov,
        u16 x,
        u16 y,
        u16 sight,
        const ExOpt& coast,
        const ExOpt& mtn,
        const ExOpt& riv);

    ExploreNear (
        const GameArraySimple& map,
        MapBitOverlay& ov,
        u16 sx,
        u16 sy,
        u16 sight,
        u8 player,
        WalkNearBias bias,
        const ExOpt& coast,
        const ExOpt& mtn,
        const ExOpt& riv);
    
    ~ExploreNear ();
    
    void move (u16 moves);
    u16 x () const;
    u16 y () const;
    u8 st () const;
    bool done () const;
    u32 near_n () const;
    u32 path_n () const;
    u32 coast_n () const;
    u32 mtn_n () const;
    u32 riv_n () const;
    u32 riv_done () const;

private:
    static const u8 k_st_done = 0u;
    static const u8 k_st_near = 1u;
    static const u8 k_st_coast = 2u;
    static const u8 k_st_mtn = 3u;
    static const u8 k_st_path = 4u;
    static const u8 k_st_river = 5u;
    static const u8 k_riv_done = 3u;
    const GameArraySimple& m_map;

    MapBitOverlay& m_ov;
    NearPathMk1 m_path;
    ExOpt m_oc;
    ExOpt m_om;
    ExOpt m_or;
    u16 m_x;
    u16 m_y;
    u16 m_rtx;
    u16 m_rty;
    u16 m_hhx;
    u16 m_hhy;
    u16 m_sight;
    u8 m_player;
    u8 m_bias;
    u8 m_st;
    u32 m_near_n;
    u32 m_path_n;
    u32 m_coast_n;
    u32 m_mtn_n;
    u32 m_riv_n;
    u32 m_riv_done;
    WalkNearMk1* m_wn;
    WalkCoastMk1* m_wc;
    WalkMtnMk1* m_wm;
    WalkRiverMk2* m_wr;
    
    void rev_around (u16 x, u16 y);
    void stop ();
    void spawn_near (u16 sx, u16 sy);
    void spawn_coast (u16 sx, u16 sy);
    void spawn_mtn (u16 sx, u16 sy);
    void spawn_river (u16 sx, u16 sy);
    void resume_near_or_stop (u16 sx, u16 sy);
    void finish_coast ();
    void finish_mtn ();
    void finish_river ();
    bool try_coast ();
    bool try_mtn ();
    bool try_river ();
    void tick_near ();
    void tick_coast ();
    void tick_mtn ();
    void tick_path ();
    void tick_river ();
};

#endif // EXPLORE_NEAR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
