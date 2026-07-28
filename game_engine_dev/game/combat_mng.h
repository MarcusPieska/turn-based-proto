//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef COMBAT_MNG_H
#define COMBAT_MNG_H

#include "game_primitives.h"

class GameState;
class RuntimeStatics;
struct UnitAddStruct;

//================================================================================================================================
//=> - CombatMng -
//================================================================================================================================
//
//  Stateless combat resolver. Looks up unit static attack/defense via m_unit_typ_idx, folds tile
//  mods, role CombatMods, and city DEFENSE boosters when the fight tile is a city. Writes m_health
//  on both units. resolve_win_prob samples 1000 fights on copies and returns attacker wins in 0..1000.
//
//================================================================================================================================

class CombatMng {
public:
    CombatMng () = delete;

    static void set_dials (u16 pred, u16 dmg_spread, u32 seed); // pred/dmg_spread 0..100; seed RNG
    static bool setup (const RuntimeStatics& st); // Bind unit statics; requires TileAttrTables ready
    static void clear (); // Drop statics pointer; not ready
    static bool ready (); // True after successful setup

    static void resolve (UnitAddStruct& atk, UnitAddStruct& def, const GameState& st, u16 x, u16 y);
    static u16 resolve_win_prob (const UnitAddStruct& atk, const UnitAddStruct& def, const GameState& st, u16 x, u16 y);

private:
    static const u16 k_prob_n = 1000u; // Sample count for resolve_win_prob

    static const RuntimeStatics* m_st; // Bound statics; not owned
    static bool m_ready; // True after setup
    static u16 m_pred; // Predictability 0..100; win odds use strength^(1+pred/15)
    static u16 m_dmg_spread; // Winner damage variance 0..100
    static u32 m_seed; // LCG RNG state

    static u16 atk_mod (const GameState& st, u16 x, u16 y);
    static u16 def_mod (const GameState& st, u16 x, u16 y);
    static i16 city_def_pct (const GameState& st, u16 x, u16 y);
    static u32 pwr (u16 base, i32 pct_mod, u8 level, u8 health);
    static u16 lvl_pct (u8 level);
    static u16 rnd (); 
};

#endif // COMBAT_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
