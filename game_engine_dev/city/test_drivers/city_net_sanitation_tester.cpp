//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>

#include "city_booster_test_shared.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () { 
    CityBoosterTestEnv env;
    if (!env.bind()) {
        return 1;
    }

    City* city = env.city();
    if (city == nullptr) {
        return 1;
    }

    std::printf("=== city net sanitation tester ===\n");
    for (u16 pop = 1; pop <= 20; ++pop) {
        city->set_population(pop);
        const auto t0 = std::chrono::high_resolution_clock::now();
        const i16 net = city->get_city_net_sanitation(env.m_city_idx);
        const auto t1 = std::chrono::high_resolution_clock::now();
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        std::printf("pop=%u net_sanitation=%d time_ns=%lld\n",
            static_cast<u32>(pop), static_cast<i32>(net), static_cast<i64>(ns));
    }
    std::printf("done\n");
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
