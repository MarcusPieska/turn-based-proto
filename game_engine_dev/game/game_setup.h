//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_SETUP_H
#define GAME_SETUP_H

#include "game_primitives.h"
#include "map_terrain_data.h"
#include "starting_point_generator.h"

//================================================================================================================================
//=> - GameSetup -
//================================================================================================================================

class GameSetup {
public:
    GameSetup ();
    ~GameSetup ();

    bool load_map (cstr path);
    bool set_player_count (u16 n);
    
    const MapTerrainData& get_map () const;
    const SpgPickCoords& get_starts () const;

private:
    GameSetup (const GameSetup& other) = delete;
    GameSetup& operator= (const GameSetup& other) = delete;
    GameSetup (GameSetup&& other) = delete;
    GameSetup& operator= (GameSetup&& other) = delete;
    MapTerrainData m_map;
    SpgPickCoords m_starts;
};

#endif // GAME_SETUP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
