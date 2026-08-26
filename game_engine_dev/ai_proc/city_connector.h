//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_CONNECTOR_H
#define CITY_CONNECTOR_H

#include "game_primitives.h"

class GameState;
class City;

//================================================================================================================================
//=> - CityConnector -
//================================================================================================================================
//
//  Worker AI: on link lock stamps ROAD_VIRTUAL along the planned spine; workers promote virtual
//  tiles to ROAD_PATH and step toward the nearest virtual on pending home-city links.
//  step_toward is shared one-step pathing toward a tile (line then local flood).
//
//================================================================================================================================

class CityConnector {
public:
    CityConnector () = delete;

    static bool begin (GameState& state);
    static void clear ();
    static void on_city_net_changed (GameState& state, u16 city_idx);
    static void clear_idle_flag (GameState& state, u16 city_idx, City* city, u16 cx, u16 cy, bool imp_disk_done);
    static bool has_virtual_at (const GameState& state, u16 x, u16 y);
    static bool step_toward (GameState& state, u16 unit_idx, u16 tx, u16 ty);
    static bool handle (GameState& state, u16 unit_idx);
};

#endif // CITY_CONNECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
