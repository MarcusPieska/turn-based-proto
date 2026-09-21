//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "game_state.h"
#include "player_ledger.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "unit_upgrades.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;
static u32 g_fail = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void note (bool ok, cstr msg) {
    if (ok) {
        std::printf("PASS: %s\n", msg);
    } else {
        std::printf("FAIL: %s\n", msg);
        g_fail = g_fail + 1u;
    }
}

static bool load_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return g_rt_statics != nullptr;
}

static u16 find_unit (cstr nm) {
    const UnitStaticData& units = g_rt_statics->unit();
    const u16 n = units.get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(units.get_name(UnitStaticDataKey::from_raw(i)), nm) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!load_statics()) {
        std::printf("ERROR: failed to load runtime statics\n");
        return 1;
    }

    note(g_rt_statics->config().get_upgrade_cost_per_prod() == 20u, "UPGRADE_COST_PER_PROD defaults to 20");
    note(g_rt_statics->config().get_upgrade_cost_per_stat_pt() == 2u, "UPGRADE_COST_PER_STAT_PT defaults to 2");

    const u16 warrior = find_unit("Warrior");
    const u16 swordsman = find_unit("Swordsman");
    const u16 spearman = find_unit("Spearman");
    note(warrior != U16_KEY_NULL && swordsman != U16_KEY_NULL && spearman != U16_KEY_NULL, "catalog has Warrior/Swordsman/Spearman");
    if (warrior == U16_KEY_NULL || swordsman == U16_KEY_NULL || spearman == U16_KEY_NULL) {
        return 1;
    }

    PlayerState seat;
    seat.m_commerce = 0;
    seat.m_commerce_from_turn = 999u;
    PlayerLedger::bind_seats(&seat, 1u, g_rt_statics);

    const UnitStaticData& units = g_rt_statics->unit();
    const UnitStaticDataStruct& wu = units.get_item(UnitStaticDataKey::from_raw(warrior));
    const UnitStaticDataStruct& su = units.get_item(UnitStaticDataKey::from_raw(swordsman));
    const u16 prod_rate = g_rt_statics->config().get_upgrade_cost_per_prod();
    const u16 stat_rate = g_rt_statics->config().get_upgrade_cost_per_stat_pt();
    const u32 prod = (su.cost - wu.cost) * static_cast<u32>(prod_rate);
    u32 stat_pts = 0u;
    if (su.attack > wu.attack) {
        stat_pts += static_cast<u32>(su.attack - wu.attack);
    }
    if (su.defense > wu.defense) {
        stat_pts += static_cast<u32>(su.defense - wu.defense);
    }
    if (su.mvt_pts > wu.mvt_pts) {
        stat_pts += static_cast<u32>(su.mvt_pts - wu.mvt_pts);
    }
    if (su.sight > wu.sight) {
        stat_pts += static_cast<u32>(su.sight - wu.sight);
    }
    const u32 expect_cost = prod + stat_pts * static_cast<u32>(stat_rate);

    note(UnitUpgrades::can_upgrade(warrior, swordsman, units), "can_upgrade Warrior->Swordsman");
    note(UnitUpgrades::can_upgrade(warrior, spearman, units), "can_upgrade Warrior->Spearman (same domain, non-NONE roles)");
    const u16 settler = find_unit("Settler");
    const u16 galley = find_unit("Galley");
    note(settler != U16_KEY_NULL && galley != U16_KEY_NULL, "catalog has Settler/Galley");
    if (settler != U16_KEY_NULL) {
        note(!UnitUpgrades::can_upgrade(warrior, settler, units), "rejects Warrior->Settler (NONE role type mismatch)");
    }
    if (galley != U16_KEY_NULL) {
        note(!UnitUpgrades::can_upgrade(warrior, galley, units), "rejects Warrior->Galley (domain mismatch)");
    }

    note(!UnitUpgrades::can_afford(0, warrior, swordsman, units), "cannot afford Warrior->Swordsman with empty treasury");

    seat.m_commerce = expect_cost;
    note(UnitUpgrades::can_afford(0, warrior, swordsman, units), "can afford Warrior->Swordsman at exact cost");

    UnitAddStruct unit = {};
    unit.m_unit_typ_idx = warrior;
    unit.m_player_idx = 0;
    note(UnitUpgrades::upgrade(0, unit, swordsman, units), "upgrade Warrior->Swordsman succeeds");
    note(static_cast<u16>(unit.m_unit_typ_idx) == swordsman, "unit typ flipped to Swordsman");
    note(seat.m_commerce == 0u, "treasury spent exact upgrade cost");
    note(seat.m_commerce_from_turn == 999u, "turn commerce buffer untouched");

    if (settler != U16_KEY_NULL) {
        seat.m_commerce = expect_cost;
        unit.m_unit_typ_idx = warrior;
        note(!UnitUpgrades::upgrade(0, unit, settler, units), "upgrade rejects NONE-role type mismatch");
        note(static_cast<u16>(unit.m_unit_typ_idx) == warrior, "unit typ unchanged on failed upgrade");
        note(seat.m_commerce == expect_cost, "treasury unchanged on failed upgrade");
    }
    PlayerLedger::bind_seats(nullptr, 0u, nullptr);

    std::printf("unit_upgrades_tester failures=%u\n", g_fail);
    return g_fail == 0u ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
