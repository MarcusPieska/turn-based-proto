//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_LOOP_CACHE_H
#define GAME_LOOP_CACHE_H

#include "game_array_simple.h"
#include "starting_point_generator.h"

//================================================================================================================================
//=> - GameLoopCache -
//================================================================================================================================
//
//  Binary cache of map geography and start coords for fast game_loop_tester startup.
//  Files live under /home/w/Projects/simple-map-gen; keyed by seed and player count.
//
//================================================================================================================================

class GameLoopCache {
public:
    static bool map_exists (cstr map_path, cstr starts_path);
    static bool save_map (cstr path, const GameArraySimple& map);
    static bool load_map (cstr path, GameArraySimple* out);
    static bool save_starts (cstr path, const SpgPickCoords& starts, u16 player_n);
    static bool load_starts (cstr path, SpgPickCoords* out, u16 player_n);

private:
    GameLoopCache () = delete;
};

#endif // GAME_LOOP_CACHE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
