//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_TILE_MANAGER_H
#define CITY_TILE_MANAGER_H

#include "circular_tile_areas.h"
#include "game_primitives.h"

class CityArray;

//================================================================================================================================
//=> - TotalTileYield -
//================================================================================================================================
//
//  Summed yields from tiles assigned to a city work pass. Returned by value from CityTileManager maximize helpers.
//
//================================================================================================================================

struct TotalTileYield {
    u32 m_food; // Total food from worked tiles
    u32 m_production; // Total production from worked tiles
    u32 m_commerce; // Total commerce from worked tiles
};

//================================================================================================================================
//=> - CityTileManager -
//================================================================================================================================
//
//  Static tile-work selection for cities. bind_cities wires the active CityArray; reach uses CircularTileAreas at r=4.
//  Maximize helpers clear reach and assign up to population; add_new helpers assign one unworked tile in one reach pass.
//  Stable-food helpers keep upkeep (2*pop) with start_food as the city-center budget, then greedily chase secondary.
//  The city center tile is never selected; City::add_* folds its free yields and local boosters.
//  Tile pick uses bucket-select over primary yield keys 0..4 and 5+.
//  clear and count_worked scan only the work reach disk around a city center.
//
//================================================================================================================================

class CityTileManager {
public:
    static void bind_cities (CityArray* cities);
    static TotalTileYield maximize_food (u16 player, u16 city_idx);
    static TotalTileYield maximize_production (u16 player, u16 city_idx);
    static TotalTileYield maximize_commerce (u16 player, u16 city_idx);
    static TotalTileYield add_new_food_tile (u16 player, u16 city_idx);
    static TotalTileYield add_new_production_tile (u16 player, u16 city_idx);
    static TotalTileYield add_new_commerce_tile (u16 player, u16 city_idx);
    static TotalTileYield stable_food_max_production (u16 player, u16 city_idx, u16 start_food);
    static TotalTileYield stable_food_max_commerce (u16 player, u16 city_idx, u16 start_food);
    static TotalTileYield stable_food_max_combined (u16 player, u16 city_idx, u16 start_food);
    static TotalTileYield gather_yields (u16 player, u16 city_idx);
    static void clear (u16 cx, u16 cy, u16 city_idx);
    static u32 count_worked (u16 cx, u16 cy, u16 city_idx);
    static CircArea work_area ();

private:
    static const u16 m_reach_r; // Work radius passed to CircularTileAreas
    static const u16 m_cand_max; // Max reachable tiles at m_reach_r
    static CityArray* m_cities; // Active match city pool; null until bind_cities

    static TotalTileYield assign_sorted (u16 player, u16 city_idx, u8 sort_food, u8 sort_production, u8 sort_commerce);
    static TotalTileYield assign_add_one (u16 player, u16 city_idx, u8 sort_food, u8 sort_production, u8 sort_commerce);
    static TotalTileYield assign_stable_food (u16 player, u16 city_idx, u16 start_food, u8 sec_mode);
};

#endif // CITY_TILE_MANAGER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
