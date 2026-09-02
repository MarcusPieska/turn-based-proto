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
//  Worker AI: when plan_spines is true (default), on link lock stamps ROAD_VIRTUAL along a planned
//  spine; workers promote virtuals to ROAD_PATH and step toward the nearest on pending home links.
//  When plan_spines is false (GenRoadNetwork prestamp), no new spines — mount opportunistically on
//  ROAD_VIRTUAL (work-disk pick or standing on one), then sticky-follow adjacent virtuals (8-neigh)
//  within max border disc (r=20) / until a CityNetwork neighbor. step_toward is shared one-step pathing.
//
//================================================================================================================================

class CityConnector {
public:
    CityConnector () = delete;

    static bool begin (GameState& state, bool plan_spines = true);
    static void set_plan_spines (bool plan);
    static bool plan_spines ();
    static void sync_road_arms (GameState& state);
    static void clear ();
    static void on_city_net_changed (GameState& state, u16 city_idx);
    static void clear_idle_flag (GameState& state, u16 city_idx, City* city, u16 cx, u16 cy, bool imp_disk_done);
    static bool has_virtual_at (const GameState& state, u16 x, u16 y);
    static bool on_road_tile (const GameState& state, u16 x, u16 y);
    static bool step_toward (GameState& state, u16 unit_idx, u16 tx, u16 ty);
    static bool handle (GameState& state, u16 unit_idx);
};

#endif // CITY_CONNECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
