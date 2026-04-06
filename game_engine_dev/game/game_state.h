//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "wonder_vector.h"

#include "fort_add_vector.h"
#include "mine_add_vector.h"
#include "monastery_add_vector.h"
#include "outpost_add_vector.h"
#include "plantation_add_vector.h"
#include "shipyard_add_vector.h"
#include "trade_post_add_vector.h"
#include "unit_add_vector.h"

#include "game_primitives.h"

//================================================================================================================================
//=> - TileArray class -
//================================================================================================================================

class GameState {
public:

    // Static storage of some global game state
    BuiltWonders m_built_wonders;

    // Dynamic storage of map adds, i.e. items not in the base game tiles, for all players.
    
    FortAddVector m_adds_fort;
    MineAddVector m_adds_mine;
    MonasteryAddVector m_adds_monastery;
    OutpostAddVector m_adds_outpost;
    PlantationAddVector m_adds_plantation;
    ShipyardAddVector m_adds_shipyard;
    TradePostAddVector m_adds_trade_post;
    UnitAddVector m_adds_unit;
};

#endif // GAME_STATE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
