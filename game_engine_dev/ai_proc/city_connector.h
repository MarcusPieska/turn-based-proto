//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_CONNECTOR_H
#define CITY_CONNECTOR_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - CityConnector -
//================================================================================================================================
//
//  Worker AI: uses CityNetwork links on the home city (WorkerHelper::get_data) to pick an unbuilt
//  neighbor, lays ROAD_PATH on tiles, and steps toward the target. Straight 8-adj walk if clear of
//  water/mountains; else a small stack-window flood from the target.
//
//================================================================================================================================

class CityConnector {
public:
    CityConnector () = delete;

    static bool begin (GameState& state);
    static void clear ();
    static void handle (GameState& state, u16 unit_idx);
};

#endif // CITY_CONNECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
