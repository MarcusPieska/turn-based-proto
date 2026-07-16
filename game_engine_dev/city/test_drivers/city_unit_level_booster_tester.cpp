//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_booster_test_shared.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    CityBoosterTestEnv env;
    if (!env.bind()) {
        return 1;
    }

    std::printf("=== city unit level booster tester ===\n");

    const u16 warrior = env.find_unit_idx("Warrior");
    const u16 barracks = env.find_bld_idx("Barracks");
    if (warrior == U16_KEY_NULL || barracks == U16_KEY_NULL) {
        std::printf("ERROR: catalog lookup failed (warrior=%u barracks=%u)\n", warrior, barracks);
        return 1;
    }

    BoosterRegisterResult huge = {};
    huge.m_unit = 20;
    const u8 capped = apply_unit_level_boost(GREEN, huge);
    city_booster_note(capped == ELITE, "apply_unit_level_boost caps at ELITE (6)");
    std::printf("  base=GREEN(1) boost=+20 -> level=%u (expect ELITE=6)\n", static_cast<unsigned>(capped));

    if (!env.finish_unit_build(warrior)) {
        std::printf("ERROR: warrior spawn failed\n");
        return 1;
    }
    const UnitAddStruct* plain = env.find_spawned_unit(warrior, 5, 5);
    const bool plain_ok = plain != nullptr && plain->m_level == GREEN;
    city_booster_note(plain_ok, "Warrior without Barracks stays GREEN");
    if (plain != nullptr) {
        std::printf("  spawned Warrior level=%u (expect GREEN=1)\n", static_cast<unsigned>(plain->m_level));
    }

    env.m_array.set_building_flag(env.m_city_idx, barracks);
    if (!env.finish_unit_build(warrior)) {
        std::printf("ERROR: barracks warrior spawn failed\n");
        return 1;
    }
    const UnitAddStruct* boosted = env.find_spawned_unit(warrior, 5, 5);
    const bool boost_ok = boosted != nullptr && boosted->m_level == DISCIPLINED;
    city_booster_note(boost_ok, "Warrior with Barracks gets +1 level (DISCIPLINED)");
    if (boosted != nullptr) {
        std::printf("  spawned Warrior level=%u (expect DISCIPLINED=2)\n", static_cast<unsigned>(boosted->m_level));
    }

    std::printf("done\n");
    return (plain_ok && boost_ok && capped == ELITE) ? 0 : 1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
