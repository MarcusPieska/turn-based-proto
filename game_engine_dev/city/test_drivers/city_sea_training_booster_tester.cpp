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

    std::printf("=== city sea training booster tester ===\n");

    const u16 galley = env.find_unit_idx("Galley");
    const u16 lighthouse = env.find_wonder_idx("Great Lighthouse");
    if (galley == U16_KEY_NULL || lighthouse == U16_KEY_NULL) {
        std::printf("ERROR: catalog lookup failed (galley=%u lighthouse=%u)\n", galley, lighthouse);
        return 1;
    }

    if (!env.finish_unit_build(galley)) {
        std::printf("ERROR: galley spawn failed\n");
        return 1;
    }
    const UnitAddStruct* plain = env.find_spawned_unit(galley, 5, 5);
    const bool plain_ok = plain != nullptr && plain->m_level == GREEN;
    city_booster_note(plain_ok, "Galley without Great Lighthouse stays GREEN");
    if (plain != nullptr) {
        std::printf("  spawned Galley level=%u (expect GREEN=1)\n", static_cast<unsigned>(plain->m_level));
    }

    env.m_wonder_city[lighthouse] = env.m_city_idx;
    if (!env.finish_unit_build(galley)) {
        std::printf("ERROR: boosted galley spawn failed\n");
        return 1;
    }
    const UnitAddStruct* boosted = env.find_spawned_unit(galley, 5, 5);
    const bool boost_ok = boosted != nullptr && boosted->m_level == DISCIPLINED;
    city_booster_note(boost_ok, "Galley with Great Lighthouse gets +1 level (DISCIPLINED)");
    if (boosted != nullptr) {
        std::printf("  spawned Galley level=%u (expect DISCIPLINED=2)\n", static_cast<unsigned>(boosted->m_level));
    }

    std::printf("done\n");
    return (plain_ok && boost_ok) ? 0 : 1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
