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

    std::printf("=== city commerce booster tester ===\n");

    const u16 market = env.find_bld_idx("Marketplace");
    const u16 bank = env.find_bld_idx("Bank");
    if (market == U16_KEY_NULL || bank == U16_KEY_NULL) {
        std::printf("ERROR: catalog lookup failed (market=%u bank=%u)\n", market, bank);
        return 1;
    }

    City* city = env.city();
    if (city == nullptr) {
        return 1;
    }

    city->accumulate_commerce();
    city->add_production(env.m_city_idx, 100);
    if (!city->finish_if_ready(env.m_city_idx)) {
        std::printf("ERROR: baseline commerce finish failed\n");
        return 1;
    }
    const u32 base_commerce = env.m_seat.m_commerce;
    const bool base_ok = base_commerce == 50;
    city_booster_note(base_ok, "100 shields -> 50 commerce with no boosters");
    std::printf("  treasury=%u (expect 50)\n", base_commerce);

    env.m_seat.m_commerce = 0;
    env.m_array.set_building_flag(env.m_city_idx, market);
    city->accumulate_commerce();
    city->add_production(env.m_city_idx, 100);
    if (!city->finish_if_ready(env.m_city_idx)) {
        std::printf("ERROR: marketplace commerce finish failed\n");
        return 1;
    }
    const u32 market_commerce = env.m_seat.m_commerce;
    const bool market_ok = market_commerce == 75;
    city_booster_note(market_ok, "Marketplace +50%% on 50 base -> 75 commerce");
    std::printf("  treasury=%u (expect 75)\n", market_commerce);

    env.m_seat.m_commerce = 0;
    env.m_array.set_building_flag(env.m_city_idx, bank);
    city->accumulate_commerce();
    city->add_production(env.m_city_idx, 100);
    if (!city->finish_if_ready(env.m_city_idx)) {
        std::printf("ERROR: stacked commerce finish failed\n");
        return 1;
    }
    const u32 stack_commerce = env.m_seat.m_commerce;
    const bool stack_ok = stack_commerce == 125;
    city_booster_note(stack_ok, "Marketplace+Bank +150%% on 50 base -> 125 commerce");
    std::printf("  treasury=%u (expect 125)\n", stack_commerce);

    std::printf("done\n");
    return (base_ok && market_ok && stack_ok) ? 0 : 1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
