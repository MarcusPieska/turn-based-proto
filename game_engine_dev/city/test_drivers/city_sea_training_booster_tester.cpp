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

    const u8 def_lv = static_cast<u8>(env.m_st->config().get_default_unit_level());

    if (!env.finish_unit_build(galley)) {
        std::printf("ERROR: galley spawn failed\n");
        return 1;
    }
    const UnitAddStruct* plain = env.find_spawned_unit(galley, 5, 5);
    const bool plain_ok = plain != nullptr && plain->m_level == def_lv;
    city_booster_note(plain_ok, "Galley without Great Lighthouse stays at DEFAULT_UNIT_LEVEL");
    if (plain != nullptr) {
        std::printf("  spawned Galley level=%u (expect default=%u)\n",
            static_cast<unsigned>(plain->m_level), static_cast<unsigned>(def_lv));
    }

    env.m_wonder_city[lighthouse] = env.m_city_idx;
    if (!env.finish_unit_build(galley)) {
        std::printf("ERROR: boosted galley spawn failed\n");
        return 1;
    }
    const UnitAddStruct* boosted = env.find_spawned_unit(galley, 5, 5);
    const u8 expect_boost = static_cast<u8>(def_lv + 1u);
    const bool boost_ok = boosted != nullptr && boosted->m_level == expect_boost;
    city_booster_note(boost_ok, "Galley with Great Lighthouse gets +1 level index");
    if (boosted != nullptr) {
        std::printf("  spawned Galley level=%u (expect %u)\n",
            static_cast<unsigned>(boosted->m_level), static_cast<unsigned>(expect_boost));
    }

    std::printf("done\n");
    return (plain_ok && boost_ok) ? 0 : 1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
