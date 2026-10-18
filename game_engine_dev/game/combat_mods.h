//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef COMBAT_MODS_H
#define COMBAT_MODS_H

#include "game_primitives.h"

class RuntimeStatics;

//================================================================================================================================
//=> - CombatBoost -
//================================================================================================================================

struct CombatBoost {
    i16 m_atk; // Attacker role points vs defender role
    i16 m_def; // Defender role points vs attacker role (+ city if on_city)
};

//================================================================================================================================
//=> - CombatMods -
//================================================================================================================================
//
//  Role-vs-role combat point table from unit_role statics. CITY_DEFENSE applies to the defender only.
//
//================================================================================================================================

class CombatMods {
public:
    CombatMods ();
    ~CombatMods ();

    void clear ();
    bool setup (const RuntimeStatics& st);
    CombatBoost get (u16 atk, u16 def, bool city) const;
    u16 role_n () const;

private:
    CombatMods (const CombatMods& o) = delete;
    CombatMods& operator= (const CombatMods& o) = delete;

    i16* m_vs; // Flat n*n; m_vs[a*n+d] = role a points vs role d
    i16* m_city; // Per-role city-defense points for the defender
    u16 m_n; // Role count
};

#endif // COMBAT_MODS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
