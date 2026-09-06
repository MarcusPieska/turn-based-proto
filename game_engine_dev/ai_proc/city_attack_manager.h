//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_ATTACK_MANAGER_H
#define CITY_ATTACK_MANAGER_H

#include "game_primitives.h"
#include "unit_add_vector_key.h"

class GameState;

//================================================================================================================================
//=> - CityAssault -
//================================================================================================================================
//
//  Ok: garrison cleared and occupy half moved onto the city.
//  Stall: canAttack units remain but spent this assault's MP with foes still on the tile.
//  Fail: hard stop (no fight left, destroy/split/occupy error).
//
//================================================================================================================================

enum class CityAssault : u8 {
    Ok = 0,
    Stall = 1,
    Fail = 2
};

//================================================================================================================================
//=> - CityBarrage -
//================================================================================================================================
//
//  Damage totals from a barrage pass: sum of all shots, and the last unit's shot only.
//
//================================================================================================================================

struct CityBarrage {
    u32 m_tot; // Sum of resolve_barrage HP removed this pass
    u32 m_last; // HP removed by the final barraging unit only
};

//================================================================================================================================
//=> - CityAttackManager -
//================================================================================================================================
//
//  Army vs city fight: refill_mp, barrage, melee, and assault (two barrage passes then melee).
//
//================================================================================================================================

struct CityAttackResult {
    CityAssault m_r;
    UnitAddKey m_stay;
    UnitAddKey m_occupy;
    u32 m_br_tot;
    u32 m_br_last;
};

class CityAttackManager {
public:
    static void refill_mp (GameState& s, UnitAddKey army_hd);

    static bool barrage (
        GameState& s,
        UnitAddKey army_hd,
        u16 city_x,
        u16 city_y,
        CityBarrage* out_dmg);

    static CityAssault melee (
        GameState& s,
        UnitAddKey* army_hd,
        u16 city_x,
        u16 city_y,
        UnitAddKey* out_stay,
        UnitAddKey* out_occupy);

    static CityAttackResult assault (
        GameState& s,
        UnitAddKey* army_hd,
        u16 city_x,
        u16 city_y);

private:
    CityAttackManager () = delete;
};

#endif // CITY_ATTACK_MANAGER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
