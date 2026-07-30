//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_TURN_HANDLER_CTX_H
#define CITY_TURN_HANDLER_CTX_H

#include "game_primitives.h"

class City;
class GameState;

//================================================================================================================================
//=> - CityTurnHandlerCtx -
//================================================================================================================================
//
//  Context passed from CityTurnHandler into trait-specific city AI implementers.
//
//================================================================================================================================

struct CityTurnHandlerCtx {
    GameState* m_state; // Active match state
    City* m_city; // City being processed this turn
    u16 m_city_idx; // Index in the city array
    u16 m_player; // Owning player seat
    u16 m_sanitation_boost; // Sanitation capacity before population debit
    i16 m_net_sanitation; // Net sanitation after population debit
    i16 m_pop_change; // Population delta from add_food this turn
};

#endif // CITY_TURN_HANDLER_CTX_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
