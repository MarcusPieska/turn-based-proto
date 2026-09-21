//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_UPGRADES_H
#define UNIT_UPGRADES_H

#include "game_primitives.h"

class UnitStaticData;
struct UnitAddStruct;

//================================================================================================================================
//=> - UnitUpgrades -
//================================================================================================================================
//
//  Pay treasury commerce to change a live unit's catalog type. can_upgrade gates legality
//  (same domain; NONE role requires same type; to.cost >= from.cost; some combat/mvt/sight
//  stat strictly higher). Cost is (to.cost - from.cost) * UPGRADE_COST_PER_PROD plus positive
//  attack/defense/mvt/sight deltas * UPGRADE_COST_PER_STAT_PT. best_upgrade is a stub.
//
//================================================================================================================================

class UnitUpgrades {
public:
    UnitUpgrades () = delete;

    static bool can_upgrade (u16 from_typ, u16 to_typ, const UnitStaticData& units);
    static bool can_afford (u16 player, u16 from_typ, u16 to_typ, const UnitStaticData& units);
    static bool upgrade (u16 player, UnitAddStruct& unit, u16 to_typ, const UnitStaticData& units);
    static u16 best_upgrade (u16 from_typ, const UnitStaticData& units);
};

#endif // UNIT_UPGRADES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
