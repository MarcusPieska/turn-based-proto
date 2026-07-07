//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_SETUP_H
#define GAME_SETUP_H

#include "game_primitives.h"
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
    cstr m_riv;  // River PPM from adv_map_gen
    cstr m_res;  // Resource overlay PPM from res_dist; may be null
};

//================================================================================================================================
//=> - GameSetup -
//================================================================================================================================
//
//  Setup-phase orchestrator. Builds or restores a GameState before play:
//  new game — load map PPMs (via Factory_GameArraySimple), pick starts, init
//  players and overlays; saved game — load GameState from file. Also writes
//  GameState to file on save. Does not run turns; hand off to GameLoop after
//  setup completes.
//
//================================================================================================================================

class GameSetup {
public:
    GameSetup ();
    ~GameSetup ();

    bool setup_new_game (GameState* state, const MapPpmPaths& paths, u16 player_n);
    bool save_game (cstr path, const GameState* state);
    bool load_game (cstr path, GameState* state);

private:
    GameSetup (const GameSetup& other) = delete;
    GameSetup& operator= (const GameSetup& other) = delete;
    GameSetup (GameSetup&& other) = delete;
    GameSetup& operator= (GameSetup&& other) = delete;

    bool run_start_placement (const GameArraySimple& map, u16 player_n, SpgPickCoords* out_starts);
    bool init_players (GameState* state, u16 player_n);
};

#endif // GAME_SETUP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
