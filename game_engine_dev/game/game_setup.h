//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_SETUP_H
#define GAME_SETUP_H

#include "game_primitives.h"
#include "map_gen_api.h"
#include "starting_point_generator.h"

class GameState;
class GameArraySimple;
class RuntimeStatics;

//================================================================================================================================
//=> - MapPpmPaths -
//================================================================================================================================
//
//  Four PPM inputs produced by adv_map_gen and res_dist for a new game map.
//
//================================================================================================================================

struct MapPpmPaths {
    cstr m_terr; // Terrain PPM from adv_map_gen
    cstr m_clim; // Climate PPM from adv_map_gen
    cstr m_riv; // River PPM from adv_map_gen
    cstr m_ov; // Overlay PPM from adv_map_gen; may be null
    cstr m_res; // Resource PPM from res_dist; may be null
};

//================================================================================================================================
//=> - GameSetup -
//================================================================================================================================
//
//  Setup-phase orchestrator. Builds or restores a GameState before play:
//  new game — generate map via map_gen.so or load PPMs (Factory_GameArraySimple),
//  pick starts, init players and overlays; saved game — load GameState from file.
//  map_gen.so stays loaded across sequential setups; call release_map_gen at
//  GameLoop handoff. Does not run turns.
//
//================================================================================================================================

class GameSetup {
public:
    GameSetup ();
    ~GameSetup ();

    bool setup_new_game (GameState* state, const MapGenReq& req, u16 player_n);
    bool setup_new_game (GameState* state, const MapPpmPaths& paths, u16 player_n);
    bool setup_from_cache (GameState* state, cstr map_path, cstr starts_path, u16 player_n);
    bool pick_starts (const GameArraySimple& map, u16 player_n, SpgPickCoords* out_starts);
    bool finish_with_starts (GameState* state, const SpgPickCoords& starts, u16 player_n);
    void release_map_gen ();
    bool save_game (cstr path, const GameState* state);
    bool load_game (cstr path, GameState* state);

private:
    GameSetup (const GameSetup& other) = delete;
    GameSetup& operator= (const GameSetup& other) = delete;
    GameSetup (GameSetup&& other) = delete;
    GameSetup& operator= (GameSetup&& other) = delete;

    bool complete_new_game (GameState* state, u16 player_n);
    bool run_start_placement (const GameArraySimple& map, u16 player_n, SpgPickCoords* out_starts);
    bool init_players (GameState* state, u16 player_n, u16 small_wonder_n);
    void cache_unit_type_idxs (GameState* state);
};

#endif // GAME_SETUP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
