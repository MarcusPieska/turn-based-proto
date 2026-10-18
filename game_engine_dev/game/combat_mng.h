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
//  Stateless combat resolver. Fight strength is a sum of point channels each scaled by a settings
//  damage factor (stat/level/size/role/terrain/city). mk02: odds from pwr^g, normal noise on the
//  win share, then HP split 2p / 2(1-p). resolve_win_prob samples 1000 fights. resolve_barrage:
//  city absorb then mock-fights. Probe helpers: noise_draw/noise_run, odds_raw, odds_split.
//
//================================================================================================================================

class CombatMng {
public:
    CombatMng () = delete;

    static void set_dials (u16 pred, u16 dmg_spread, u32 seed);
    static bool setup (const RuntimeStatics& st);
    static void clear ();
    static bool ready ();

    static void resolve_attack (UnitAddStruct& atk, UnitAddStruct& def, const GameState& st, u16 x, u16 y);
    static u32 resolve_barrage (UnitAddStruct& atk, GameState& st, u16 x, u16 y);
    static u16 resolve_win_prob (const UnitAddStruct& atk, const UnitAddStruct& def, const GameState& st, u16 x, u16 y);

    static double noise_sig ();
    static double noise_cap ();
    static double noise_draw ();
    static void noise_run (u32 n, double* mean, double* sd, double* us);
    static double odds_raw (u32 ap, u32 dp);
    static double odds_split (const UnitAddStruct& atk, const UnitAddStruct& def, const GameState& st, u16 x, u16 y);
private:
    static const u16 k_prob_n = 1000u;

    static const RuntimeStatics* m_st;
    static bool m_ready;
    static u16 m_pred;
    static u16 m_dmg_spread;
    static u32 m_seed;

    static i32 atk_mod (const GameState& st, u16 x, u16 y);
    static i32 def_mod (const GameState& st, u16 x, u16 y);
    static i16 city_def_pts (const GameState& st, u16 x, u16 y);
    static u32 pwr (i32 pts, u8 health);
    static u16 rnd ();
    static void apply_odds (UnitAddStruct& atk, UnitAddStruct& def, u32 ap, u32 dp);
    static void resolve_atk_city (UnitAddStruct& atk, UnitAddStruct& def, const GameState& st, u16 x, u16 y);
};

#endif // COMBAT_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
