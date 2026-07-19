//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_STATE_H
#define GAME_STATE_H

#define SETTLER_MISSION_SLOTS 20

#include "game_array_simple.h"
#include "bit_array.h"
#include "map_bit_overlay.h"
#include "map_bit_array_overlay.h"

#include "fort_add_vector.h"
#include "mine_add_vector.h"
#include "monastery_add_vector.h"
#include "outpost_add_vector.h"
#include "plantation_add_vector.h"
#include "shipyard_add_vector.h"
#include "trade_post_add_vector.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "civ_relations.h"
#include "city_array.h"
#include "sector_network.h"
#include "sector_network_router.h"

#include "game_primitives.h"

class RuntimeStatics;

//================================================================================================================================
//=> - PlayerState -
//================================================================================================================================
//
//  Per-player runtime slice inside GameState. One entry per seat in the match.
//
//================================================================================================================================

class PlayerState {
public:
    PlayerState () {
        for (u16 i = 0; i < static_cast<u16>(SETTLER_MISSION_SLOTS); ++i) {
            m_settler_idx[i] = U16_KEY_NULL;
        }
    }

    u16* m_small_wonder_city = nullptr; // Built small wonder city index per catalog row; U16_KEY_NULL if none
    MapBitOverlay* m_explored_overlay = nullptr; // Fog/explored bit grid; same size as map
    BitArrayCL* m_techs_researched = nullptr; // Researched-tech bitset for this seat
    u32 m_commerce = 0; // Accumulated commerce treasury for this seat
    u32 m_research = 0; // Accumulated research beakers for this seat
    u16 m_ai_controlled = UINT16_MAX; // Nonzero if seat is AI-controlled
    u16 m_is_active = UINT16_MAX; // Nonzero if seat is still in the game
    u16 m_civ_index = UINT16_MAX; // Civ roster index for this seat

    u16 m_target_settlements = 0; // Desired settler count; 0 off; STM sets SETTLER_MISSION_SLOTS with sites / 2 with none
    u16 m_last_turn_settler_count = 0; // Settlers tallied last unit pass (SettlerTurnMng::handle)
    u16 m_settler_idx[SETTLER_MISSION_SLOTS]; // Settler mission slots; length SETTLER_MISSION_SLOTS
    u16 m_scout_1_idx = U16_KEY_NULL; // Scout unit slot 1
    u16 m_scout_2_idx = U16_KEY_NULL; // Scout unit slot 2
    u16 m_scout_3_idx = U16_KEY_NULL; // Scout unit slot 3
    u16 m_scout_4_idx = U16_KEY_NULL; // Scout unit slot 4

    u16 m_free_unit_support = 0; // Max number of units before upkeep is required
};

//================================================================================================================================
//=> - GameState -
//================================================================================================================================
//
//  Owns all mutable data for one match: map, players, improvements, units, and overlays. 
//  Built or restored by GameSetup (new game or load file).
//  Advanced each turn by GameLoop; AI and sim systems read from here.
//
//================================================================================================================================

class GameState {
public:
    void clear (); // Release map, players, overlays; reset counters

    // Transitional: stacks/unstack not fully wired; see UnitMovementMng
    bool spawn (u16 x, u16 y, u16 player_idx, const u16* typ_idxs, u16 typ_n);

    u32 m_current_turn = 0; // Turn counter; incremented by GameLoop::step
    u32 m_turn_limit = 1000; // External driver stops calling step when m_current_turn reaches this
    u16 m_player_n = 0; // Seat count in m_player_states; size of m_player_states array
    u16 m_players_remaining = 0; // Seats not eliminated
    PlayerState* m_player_states = nullptr; // Array of length m_player_n
    bool m_age_of_exploration = true; // If true, fog/explore rules apply; else, all see all 
    const RuntimeStatics* m_statics = nullptr; // Process-wide static data; not owned by GameState
    CivRelations m_civ_relations; // Civ-vs-civ relation matrix for ally checks
    u16* m_wonder_city = nullptr; // Built world wonder city index per catalog row; U16_KEY_NULL if none
    u16 m_wonder_count = 0; // Length of m_wonder_city
    u16 m_small_wonder_count = 0; // Length of each player's m_small_wonder_city
    UnitAddVector m_units; // Unit pool; tile stacks linked via UnitAddStruct::m_next_unit
    CityArray m_cities; // City pool; map tiles link via m_add_idx when m_add_typ is city
    GameArraySimple m_map; // World grid; terrain, rivers, tile handles
    SectorNetwork m_sector_net; // Land/water sector lattice for general pathing
    SectorNetworkRouter m_sector_rt; // Hop router over m_sector_net land links

    // Adds on map, owned by tile owner via GameTileSimple::m_civ_owner
    FortAddVector m_adds_fort; 
    MineAddVector m_adds_mine; 
    MonasteryAddVector m_adds_monastery; 
    OutpostAddVector m_adds_outpost; 
    PlantationAddVector m_adds_plantation;
    ShipyardAddVector m_adds_shipyard;
    TradePostAddVector m_adds_trade_post; 
};

#endif // GAME_STATE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
