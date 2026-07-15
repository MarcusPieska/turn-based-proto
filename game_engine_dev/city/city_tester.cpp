//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "bit_array.h"
#include "city.h"
#include "city_array.h"
#include "game_state.h"
#include "player_ledger.h"
#include "runtime_static_loader.h"

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

typedef const char* cstr;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

void note_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        std::printf("*** TEST FAILED: %s\n", msg);
    }
}

void summarize_test_results () {
    if (print_level > 0) {
        std::printf("--------------------------------\n");
        std::printf(" Test count: %d\n", test_count);
        std::printf(" Test pass: %d\n", test_pass);
        std::printf(" Test fail: %d\n", test_count - test_pass);
        std::printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

static bool ensure_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../data_io/runtime_static_loader_lib.so", "../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

struct CityTestEnv {
    CityArray m_array;
    BitArrayCL* m_techs = nullptr;
    BitArrayCL* m_civ = nullptr;
    u16* m_wonder_city = nullptr;
    GameState m_state = {};
    PlayerState m_seat = {};

    ~CityTestEnv () {
        delete m_techs;
        delete m_civ;
        delete[] m_wonder_city;
        delete[] m_seat.m_small_wonder_city;
        PlayerLedger::bind_state(nullptr);
        City::bind_wonder_cities(nullptr);
        City::bind_player_states(nullptr, 0);
    }

    bool bind () {
        if (!ensure_statics()) {
            return false;
        }
        const RuntimeStatics& st = *g_rt_statics;
        if (!m_array.bind_statics(st)) {
            return false;
        }
        m_techs = new BitArrayCL(st.tech().get_item_count());
        m_civ = new BitArrayCL(st.civ().get_item_count());
        const u16 wonder_n = st.wonder().get_item_count();
        const u16 sw_n = st.small_wonder().get_item_count();
        if (wonder_n > 0) {
            m_wonder_city = new u16[wonder_n];
            for (u16 i = 0; i < wonder_n; ++i) {
                m_wonder_city[i] = U16_KEY_NULL;
            }
        }
        m_seat = {};
        if (sw_n > 0) {
            m_seat.m_small_wonder_city = new u16[sw_n];
            for (u16 i = 0; i < sw_n; ++i) {
                m_seat.m_small_wonder_city[i] = U16_KEY_NULL;
            }
        }
        City::bind_wonder_cities(m_wonder_city);
        m_state.m_player_n = 1;
        m_state.m_player_states = &m_seat;
        m_state.m_small_wonder_count = sw_n;
        City::bind_player_states(m_state.m_player_states, m_state.m_player_n);
        PlayerLedger::bind_state(&m_state);
        return true;
    }
};

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_city_init () {
    CityTestEnv env;
    if (!env.bind()) {
        note_result(false, "City test env bind");
        summarize_test_results();
        return;
    }
    City city;
    city.init(0, 10, 20);
    bool ok = city.get_owner() == 0
        && city.get_x() == 10
        && city.get_y() == 20
        && city.get_current_food_store() == 0
        && city.get_current_shields_store() == 0
        && city.get_current_population() == 1;
    note_result(ok, "City init fields");
    summarize_test_results();
}

void test_city_add_food () {
    City city;
    city.init(0, 0, 0);
    city.add_food(25);
    note_result(city.get_current_food_store() == 5 && city.get_current_population() == 2, "City add_food growth");
    summarize_test_results();
}

void test_city_add_shields () {
    City city;
    city.add_shields(20);
    city.add_shields(15);
    note_result(city.get_current_shields_store() == 35, "City add_shields");
    summarize_test_results();
}

void test_city_get_buildable_buildings () {
    CityTestEnv env;
    if (!env.bind()) {
        note_result(false, "City test env bind");
        summarize_test_results();
        return;
    }
    City city;
    city.init(0, 0, 0);
    BitArrayCL* r = city.get_buildable_buildings(0, env.m_techs, env.m_civ);
    note_result(r != nullptr, "City get_buildable_buildings scratch");
    summarize_test_results();
}

void test_city_accumulate_commerce_mode () {
    City city;
    city.accumulate_commerce();
    city.add_shields(10);
    note_result(city.get_current_shields_store() == 10, "City accumulate_commerce mode");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_city_init();
    test_city_add_food();
    test_city_add_shields();
    test_city_get_buildable_buildings();
    test_city_accumulate_commerce_mode();

    std::printf("=======================================================\n");
    std::printf(" TESTING CITY: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
