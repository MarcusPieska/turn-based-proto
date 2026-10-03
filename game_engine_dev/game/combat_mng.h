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
//  on units. Default mk02: raw win odds from pwr^g, normal noise on odds (mk02 cpp dials for sigma
//  and +/- cap), then damage split 2p / 2(1-p) of each side's current HP; exact 50% nudges to 51%
//  for the attacker. resolve_win_prob samples 1000 fights on copies (attacker wins 0..1000).
//  resolve_barrage: city defense absorb then mock-fights with 20% of mock defender HP lost.
//  mk02 probe: noise_draw/noise_run for noise shape+speed; odds_raw for closed-form p from powers.
//
//================================================================================================================================

class CombatMng {
public:
    CombatMng () = delete;

    static void set_dials (u16 pred, u16 dmg_spread, u32 seed); // pred/dmg_spread 0..100; seed RNG
    static bool setup (const RuntimeStatics& st); // Bind unit statics; requires TileAttrTables ready
    static void clear (); // Drop statics pointer; not ready
    static bool ready (); // True after successful setup

    static void resolve_attack (UnitAddStruct& atk, UnitAddStruct& def, const GameState& st, u16 x, u16 y);
    static u32 resolve_barrage (UnitAddStruct& atk, GameState& st, u16 x, u16 y);
    static u16 resolve_win_prob (const UnitAddStruct& atk, const UnitAddStruct& def, const GameState& st, u16 x, u16 y);

    static double noise_sig (); // mk02: normal sigma in percentage points
    static double noise_cap (); // mk02: abs clip on noise in percentage points
    static double noise_draw (); // mk02: one clipped N(0,sig) sample in percentage points
    static void noise_run (u32 n, double* mean, double* sd, double* us); // mk02: n draws; mean/sd/us out
    static double odds_raw (u32 ap, u32 dp); // mk02: closed-form attacker damage-split share; no noise
    static double odds_split (const UnitAddStruct& atk, const UnitAddStruct& def, const GameState& st, u16 x, u16 y);
private:
    static const u16 k_prob_n = 1000u; // Sample count for resolve_win_prob

    static const RuntimeStatics* m_st; // Bound statics; not owned
    static bool m_ready; // True after successful setup
    static u16 m_pred; // Predictability 0..100; win odds use strength^(1+pred/12)
    static u16 m_dmg_spread; // Legacy dial; mk02 noise uses cpp k_noise_sig / k_noise_cap
    static u32 m_seed; // LCG RNG state

    static u16 atk_mod (const GameState& st, u16 x, u16 y);
    static u16 def_mod (const GameState& st, u16 x, u16 y);
    static i16 city_def_pct (const GameState& st, u16 x, u16 y);
    static u32 pwr (u16 base, i32 pct_mod, u8 level, u8 health, u8 size);
    static u16 lvl_pct (u8 level);
    static u16 rnd ();
    static void apply_odds (UnitAddStruct& atk, UnitAddStruct& def, u32 ap, u32 dp);
    static void resolve_atk_city (UnitAddStruct& atk, UnitAddStruct& def, const GameState& st, u16 x, u16 y);
};

#endif // COMBAT_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
