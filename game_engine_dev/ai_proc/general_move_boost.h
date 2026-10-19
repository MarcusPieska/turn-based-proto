//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERAL_MOVE_BOOST_H
#define GENERAL_MOVE_BOOST_H

#include "game_primitives.h"
#include "unit_add_vector_key.h"

class GameState;

//================================================================================================================================
//=> - GeneralMoveBoost -
//================================================================================================================================
//
//  After a normal MP assign, adds one PATH_MP_TURN to a unit if its group is commanded by a general
//  listed in PlayerState::m_generals. Call on_unit_refilled after single-unit refill; on_grp_refilled
//  after a full group-chain refill (delegates to ArmyGeneral::boost_mvt_pts). Unit-loop general
//  MP is applied by ArmyGeneralTurnHandler::handle instead of on_unit_refilled.
//
//================================================================================================================================

class GeneralMoveBoost {
public:
    GeneralMoveBoost () = delete;

    static void on_unit_refilled (GameState& s, u16 unit_idx);
    static void on_grp_refilled (GameState& s, UnitAddKey head);

private:
    static bool unit_has_general (const GameState& s, u16 seat, u16 unit_idx);
    static bool chain_has_general (const GameState& s, u16 seat, UnitAddKey head);
};

#endif // GENERAL_MOVE_BOOST_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
