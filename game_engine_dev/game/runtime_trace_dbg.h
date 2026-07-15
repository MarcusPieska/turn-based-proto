//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RUNTIME_TRACE_DBG_H
#define RUNTIME_TRACE_DBG_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Toggles -
//================================================================================================================================

#define RUNTIME_TRACE_DBG
#define ENABLE_FLUSH_AFTER_PRINT

#define ENABLED_TRACE_SETUP
#define ENABLED_TRACE_CITY_FOUNDATION
#define ENABLED_TRACE_CIV_SPAWN_PT
#define ENABLED_TRACE_UNIT_SPAWN
#define ENABLED_TRACE_NEW_TURN
#define ENABLED_TRACE_EXPLORE_DISCOVER
#define ENABLED_TRACE_PATH_FAILURE
//#define ENABLED_TRACE_P2P_MK4_OVL
#define ENABLED_TRACE_P2P_MK4_WALK
#define ENABLED_TRACE_P2P_MK3_WALK
#define ENABLED_MAP_ARRAY_ACCESS_CHK

//================================================================================================================================
//=> - Macro-to-function mappings -
//================================================================================================================================

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_SETUP)
    void trace_setup(cstr label);
#define TRACE_SETUP(args) trace_setup args
#else
    #define TRACE_SETUP(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_CITY_FOUNDATION)
    void trace_city_foundation(u16 x, u16 y, u16 player);
#define TRACE_CITY_FOUNDATION(args) trace_city_foundation args
#else
    #define TRACE_CITY_FOUNDATION(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_CIV_SPAWN_PT)
    void trace_civ_spawn_pt (u16 x, u16 y, u16 civ_idx);
#define TRACE_CIV_SPAWN_PT(args) trace_civ_spawn_pt args
#else
    #define TRACE_CIV_SPAWN_PT(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_UNIT_SPAWN)
    void trace_unit_spawn (u16 typ_idx, u16 civ_idx, u16 x, u16 y);
#define TRACE_UNIT_SPAWN(args) trace_unit_spawn args
#else
    #define TRACE_UNIT_SPAWN(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_NEW_TURN)
    void trace_new_turn(u16 turn);
#define TRACE_NEW_TURN(args) trace_new_turn args
#else
    #define TRACE_NEW_TURN(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_EXPLORE_DISCOVER)
    void trace_explore_discover (u16 x, u16 y, u16 player);
#define TRACE_EXPLORE_DISCOVER(args) trace_explore_discover args
#else
    #define TRACE_EXPLORE_DISCOVER(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_PATH_FAILURE)
    void trace_path_failure (cstr msg);
#define TRACE_PATH_FAILURE(args) trace_path_failure args
#else
    #define TRACE_PATH_FAILURE(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_P2P_MK4_OVL)
    void trace_p2p_mk4_enter (u16 ux, u16 uy, u16 vx, u16 vy, u16 ct, u16 nt);
    void trace_p2p_mk4_leave (u16 sx, u16 sy, u16 st);
    void trace_p2p_mk4_block (u16 vx, u16 vy, u16 d);
    void trace_p2p_mk4_skip (u16 sx, u16 sy, u16 st);
#define TRACE_P2P_MK4_ENTER(args) trace_p2p_mk4_enter args
#define TRACE_P2P_MK4_LEAVE(args) trace_p2p_mk4_leave args
#define TRACE_P2P_MK4_BLOCK(args) trace_p2p_mk4_block args
#define TRACE_P2P_MK4_SKIP(args) trace_p2p_mk4_skip args
#else
    #define TRACE_P2P_MK4_ENTER(args) ((void)0)
    #define TRACE_P2P_MK4_LEAVE(args) ((void)0)
    #define TRACE_P2P_MK4_BLOCK(args) ((void)0)
    #define TRACE_P2P_MK4_SKIP(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_P2P_MK4_WALK)
    void trace_p2p_mk4_walk_step (u16 ux, u16 uy, u16 vx, u16 vy, u16 cost, u16 mp, u32 turn);
    void trace_p2p_mk4_walk_stall (u16 x, u16 y, u16 mp, u32 turn);
    void trace_p2p_mk4_walk_done (u16 x, u16 y, u32 steps, u32 turns);
#define TRACE_P2P_MK4_WALK_STEP(args) trace_p2p_mk4_walk_step args
#define TRACE_P2P_MK4_WALK_STALL(args) trace_p2p_mk4_walk_stall args
#define TRACE_P2P_MK4_WALK_DONE(args) trace_p2p_mk4_walk_done args
#else
    #define TRACE_P2P_MK4_WALK_STEP(args) ((void)0)
    #define TRACE_P2P_MK4_WALK_STALL(args) ((void)0)
    #define TRACE_P2P_MK4_WALK_DONE(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_P2P_MK3_WALK)
    void trace_p2p_mk3_walk_step (u16 ux, u16 uy, u16 vx, u16 vy, u16 cost, u16 mp, u32 turn);
    void trace_p2p_mk3_walk_stall (u16 x, u16 y, u16 mp, u32 turn);
    void trace_p2p_mk3_walk_done (u16 x, u16 y, u32 steps, u32 turns);
#define TRACE_P2P_MK3_WALK_STEP(args) trace_p2p_mk3_walk_step args
#define TRACE_P2P_MK3_WALK_STALL(args) trace_p2p_mk3_walk_stall args
#define TRACE_P2P_MK3_WALK_DONE(args) trace_p2p_mk3_walk_done args
#else
    #define TRACE_P2P_MK3_WALK_STEP(args) ((void)0)
    #define TRACE_P2P_MK3_WALK_STALL(args) ((void)0)
    #define TRACE_P2P_MK3_WALK_DONE(args) ((void)0)
#endif

//================================================================================================================================
//=> - Map array access -
//================================================================================================================================

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_MAP_ARRAY_ACCESS_CHK)
    void check_map_array_access (u16 w, u16 h, u16 x, u16 y);
#define CHECK_MAP_ARRAY_ACCESS(args) check_map_array_access args
#else
    #define CHECK_MAP_ARRAY_ACCESS(args) ((void)0)
#endif

#endif // RUNTIME_TRACE_DBG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

