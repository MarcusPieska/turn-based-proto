//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ATTACK_CITY_H
#define ATTACK_CITY_H

#include "game_primitives.h"
#include "unit_add_vector_key.h"

class GameState;

//================================================================================================================================
//=> - AttackCity -
//================================================================================================================================
//
//  Assaults a city tile from an adjacent army group: offensive units with >= mov_pt_per_turn MP strike
//  (highest attack vs highest-defense defender), each strike costs mov_pt_per_turn on that unit only.
//  Stops when no eligible offensive units remain. On cleared garrison, splits the army half-by-type
//  (forcing the last attacker into the occupy half) and moves the occupy group onto the city.
//
//================================================================================================================================

class AttackCity {
public:
    static bool assault (
        GameState& s,
        UnitAddKey army_hd,
        u16 city_x,
        u16 city_y,
        UnitAddKey* out_stay,
        UnitAddKey* out_occupy);

private:
    AttackCity () = delete;
};

#endif // ATTACK_CITY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
