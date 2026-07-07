//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_STATE_H
#define GAME_STATE_H

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
    u16 m_ai_controlled = UINT16_MAX; // Nonzero if seat is AI-controlled
    u16 m_is_active = UINT16_MAX; // Nonzero if seat is still in the game
    u16 m_civ_index = UINT16_MAX; // Civ roster index for this seat
    MapBitOverlay* m_explored_overlay = nullptr; // Fog/explored bit grid; same size as map
    BitArrayCL* m_techs_researched = nullptr; // Researched-tech bitset for this seat
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

    u32 m_current_turn = 0; // Turn counter; incremented by GameLoop
    u16 m_player_n = 0; // Seat count in m_player_states; size of m_player_states array
    u16 m_players_remaining = 0; // Seats not eliminated
    PlayerState* m_player_states = nullptr; // Array of length m_player_n
    bool m_age_of_exploration = true; // If true, fog/explore rules apply; else, all see all 
    const RuntimeStatics* m_statics = nullptr; // Process-wide static data; not owned by GameState
    CivRelations m_civ_relations; // Civ-vs-civ relation matrix for ally checks
    MapBitArrayOverlay* m_tile_ownership_array = nullptr; // Owner id per tile; sized to map
    BitArrayCL* m_built_wonders = nullptr; // World wonders built; shared across seats
    UnitAddVector m_units; // Unit pool; tile stacks linked via UnitAddStruct::m_next_unit
    GameArraySimple m_map; // World grid; terrain, rivers, tile handles

    // Adds on map, owned by tile owner, per m_tile_ownership_array
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
