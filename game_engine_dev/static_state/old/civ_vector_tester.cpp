//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <sstream>

#include "bit_array.h"
#include "city_flags.h"
#include "tech_data.h"
#include "resource_data.h"
#include "building_data.h"
#include "civ_trait_data.h"
#include "civ_data.h"
#include "civ_vector.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;
typedef std::string str;
typedef uint32_t u32;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

//================================================================================================================================
//=> - Helper functions -
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
        if (print_level > 0) {
            total_test_fails++;
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void note_result (bool cond, cstr msg1, cstr msg2) {
    str msg = str(msg1) + str(msg2);
    note_result(cond, msg.c_str());
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
    test_pass  = 0;
}

//================================================================================================================================
//=> - Test helper types -
//================================================================================================================================

class GameStarter {
public:
    explicit GameStarter(CivVector* civs)
        : m_civs(civs) {}

    void add_to_game(u16 idx) {
        if (m_civs) {
            m_civs->add_to_game(idx);
        }
    }

private:
    CivVector* m_civs;
};

struct CivVectorTestSetup {
    CivVector* cv;
    u32        count;

    CivVectorTestSetup() : cv(nullptr), count(0) {}
    ~CivVectorTestSetup() { delete cv; }
};

CivVectorTestSetup setup_civ_vector_test () {
    CivVectorTestSetup setup;
    setup.count = static_cast<u32>(CivData::get_civ_data_count());
    if (setup.count > 0) {
        setup.cv = new CivVector();
    }
    return setup;
}

//================================================================================================================================
//=> - Static civ-data validation -
//================================================================================================================================

void verify_civ_stats (const CivStats& stats, cstr test_name) {
    note_result(!stats.name.empty(), test_name, " - name not empty");
}

void test_civ_data_basic () {
    u32 count = static_cast<u32>(CivData::get_civ_data_count());
    const CivStats* civs = CivData::get_civ_data_array();

    if (count == 0 || !civs) {
        note_result(false, "CivData basic: no civs found");
        summarize_test_results();
        return;
    }

    note_result(true, "CivData basic: civs present");

    u32 checked = 0;
    for (u32 i = 0; i < count && checked < 10; ++i) {
        str name = "CivData basic idx " + std::to_string(i);
        verify_civ_stats(civs[i], name.c_str());
        ++checked;
    }

    summarize_test_results();
}

void test_civ_none_placeholders () {
    u16 civ_count = CivData::get_civ_data_count();
    const CivStats* civs = CivData::get_civ_data_array();
    u16 trait_count = CivTraitData::get_civ_trait_count();
    const CivTraitStats* traits = CivTraitData::get_civ_trait_data_array();

    bool civ_ok = civ_count > 0 && civs && civs[0].name == "CIV_NONE";
    note_result(civ_ok, "CivData none placeholder: civ[0] is CIV_NONE");

    bool traits_clear = true;
    if (civ_ok) {
        for (u16 t = 0; t < MAX_TRAITS_PER_CIV; ++t) {
            if (civs[0].trait_indices.indices[t] != 0) {
                traits_clear = false;
                break;
            }
        }
    } else {
        traits_clear = false;
    }
    note_result(traits_clear, "CivData none placeholder: civ[0] traits all zero");

    bool trait_ok = trait_count > 0 && traits && traits[0].name == "CIV_TRAIT_NONE";
    note_result(trait_ok, "CivTraitData none placeholder: trait[0] is CIV_TRAIT_NONE");

    summarize_test_results();
}

//================================================================================================================================
//=> - CivVector tests -
//================================================================================================================================

void test_civ_vector_initial_state () {
    CivVectorTestSetup setup = setup_civ_vector_test();
    if (setup.count == 0 || !setup.cv) {
        note_result(false, "CivVector initial: no civs found");
        summarize_test_results();
        return;
    }

    bool all_not_in_game = true;
    for (u32 i = 0; i < setup.count && i < 64; ++i) { 
        bool in_game = setup.cv->is_in_game(static_cast<u16>(i));
        if (in_game) {
            all_not_in_game = false;
            break;
        }
    }

    note_result(all_not_in_game, "CivVector initial: all civs not in game");
    summarize_test_results();
}

void test_civ_vector_single_set () {
    CivVectorTestSetup setup = setup_civ_vector_test();
    if (setup.count == 0 || !setup.cv) {
        note_result(false, "CivVector single set: no civs found");
        summarize_test_results();
        return;
    }
    GameStarter starter(setup.cv);
    u16 idx = 0;
    starter.add_to_game(idx);
    bool in_game = setup.cv->is_in_game(idx);
    note_result(in_game, "CivVector single set: idx 0 becomes in game");
    if (setup.count > 1) {
        bool other_in_game = setup.cv->is_in_game(static_cast<u16>(setup.count - 1));
        note_result(!other_in_game, "CivVector single set: last idx still not in game");
    }

    summarize_test_results();
}

void test_civ_vector_multiple_indices () {
    CivVectorTestSetup setup = setup_civ_vector_test();
    if (setup.count < 3 || !setup.cv) {
        note_result(false, "CivVector multiple indices: need at least 3 civs");
        summarize_test_results();
        return;
    }

    GameStarter starter(setup.cv);
    u16 idx_a = 0;
    u16 idx_b = static_cast<u16>(setup.count / 2);
    u16 idx_c = static_cast<u16>(setup.count - 1);

    starter.add_to_game(idx_a);
    starter.add_to_game(idx_b);
    starter.add_to_game(idx_c);

    note_result(setup.cv->is_in_game(idx_a), "CivVector multiple: idx A in game");
    note_result(setup.cv->is_in_game(idx_b), "CivVector multiple: idx B in game");
    note_result(setup.cv->is_in_game(idx_c), "CivVector multiple: idx C in game");

    summarize_test_results();
}

void test_civ_vector_bounds () {
    CivVectorTestSetup setup = setup_civ_vector_test();
    if (setup.count == 0 || !setup.cv) {
        note_result(false, "CivVector bounds: no civs found");
        summarize_test_results();
        return;
    }
    GameStarter starter(setup.cv);
    starter.add_to_game(0);
    note_result(setup.cv->is_in_game(0), "CivVector bounds: idx 0 in game");
    if (setup.count > 1) {
        u16 last = static_cast<u16>(setup.count - 1);
        starter.add_to_game(last);
        note_result(setup.cv->is_in_game(last), "CivVector bounds: last idx in game");
    }
    u16 oob = static_cast<u16>(setup.count);
    starter.add_to_game(oob);
    bool in_game_oob = setup.cv->is_in_game(oob);
    note_result(!in_game_oob, "CivVector bounds: out-of-bounds idx not in game");

    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    CityFlagData::load_static_data("../game_config.city_flags");
    TechData::load_static_data("../game_config.techs");
    ResourceData::load_static_data("../game_config.resources");
    BuildingData::load_static_data("../game_config.buildings");
    CivTraitData::load_static_data("../game_config.civ_traits");
    CivData::load_static_data("../game_config.civs");

    test_civ_data_basic();
    test_civ_none_placeholders();
    test_civ_vector_initial_state();
    test_civ_vector_single_set();
    test_civ_vector_multiple_indices();
    test_civ_vector_bounds();

    std::printf("=======================================================\n");
    std::printf(" TESTING CIVS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
