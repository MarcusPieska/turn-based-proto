//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "unit_upgrades.h"

#include "assert_log.h"
#include "game_state.h"
#include "player_ledger.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_role_enum.h"
#include "unit_static_data.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 pos_delta (u16 from, u16 to) {
    return to > from ? static_cast<u32>(to - from) : 0u;
}

static bool calc_cost (u16 from_typ, u16 to_typ, const UnitStaticData& units, u16 prod_rate, u16 stat_rate, u32* out) {
    GAME_EXPECT_RET(out != nullptr, false, "UnitUpgrades calc_cost null out");
    if (!UnitUpgrades::can_upgrade(from_typ, to_typ, units)) {
        return false;
    }
    const UnitStaticDataStruct& from = units.get_item(UnitStaticDataKey::from_raw(from_typ));
    const UnitStaticDataStruct& to = units.get_item(UnitStaticDataKey::from_raw(to_typ));
    const u32 prod = (to.cost - from.cost) * static_cast<u32>(prod_rate);
    const u32 stats = (pos_delta(from.attack, to.attack)
        + pos_delta(from.defense, to.defense)
        + pos_delta(from.mvt_pts, to.mvt_pts)
        + pos_delta(from.sight, to.sight)) * static_cast<u32>(stat_rate);
    *out = prod + stats;
    return true;
}

//================================================================================================================================
//=> - UnitUpgrades -
//================================================================================================================================

bool UnitUpgrades::can_upgrade (u16 from_typ, u16 to_typ, const UnitStaticData& units) {
    const u16 n = units.get_item_count();
    if (from_typ == to_typ || from_typ >= n || to_typ >= n) {
        return false;
    }
    const UnitStaticDataStruct& from = units.get_item(UnitStaticDataKey::from_raw(from_typ));
    const UnitStaticDataStruct& to = units.get_item(UnitStaticDataKey::from_raw(to_typ));
    if (from.domain != to.domain) {
        return false;
    }
    const u16 none = static_cast<u16>(UnitRole::NONE);
    if ((from.role == none || to.role == none) && from.type != to.type) {
        return false;
    }
    if (to.cost < from.cost) {
        return false;
    }
    return to.attack > from.attack || to.defense > from.defense || to.mvt_pts > from.mvt_pts || to.sight > from.sight;
}

bool UnitUpgrades::can_afford (u16 player, u16 from_typ, u16 to_typ, const UnitStaticData& units) {
    u16 prod_rate = 0;
    u16 stat_rate = 0;
    if (!PlayerLedger::upgrade_rates(&prod_rate, &stat_rate)) {
        return false;
    }
    u32 cost = 0;
    if (!calc_cost(from_typ, to_typ, units, prod_rate, stat_rate, &cost)) {
        return false;
    }
    return PlayerLedger::commerce(player) >= cost;
}

bool UnitUpgrades::upgrade (u16 player, UnitAddStruct& unit, u16 to_typ, const UnitStaticData& units) {
    const u16 from_typ = static_cast<u16>(unit.m_unit_typ_idx);
    u16 prod_rate = 0;
    u16 stat_rate = 0;
    if (!PlayerLedger::upgrade_rates(&prod_rate, &stat_rate)) {
        return false;
    }
    u32 cost = 0;
    if (!calc_cost(from_typ, to_typ, units, prod_rate, stat_rate, &cost)) {
        return false;
    }
    if (!PlayerLedger::spend_commerce(player, cost)) {
        return false;
    }
    unit.m_unit_typ_idx = to_typ;
    return true;
}

u16 UnitUpgrades::best_upgrade (u16 from_typ, const UnitStaticData& units) {
    (void)from_typ;
    (void)units;
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
