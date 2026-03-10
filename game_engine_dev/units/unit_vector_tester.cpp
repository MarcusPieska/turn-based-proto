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
#include "unit_data.h"
#include "unit_vector.h"

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

class UnitAssessor {
public:
    explicit UnitAssessor(BuildableUnits* buildable)
        : m_buildable(buildable) {}

    void set_buildable(u16 idx) {
        if (m_buildable) {
            m_buildable->set_buildable(idx);
        }
    }

private:
    BuildableUnits* m_buildable;
};

struct BuildableUnitsTestSetup {
    BuildableUnits* bu;
    u32             count;

    BuildableUnitsTestSetup() : bu(nullptr), count(0) {}
    ~BuildableUnitsTestSetup() { delete bu; }
};

BuildableUnitsTestSetup setup_buildable_units_test () {
    BuildableUnitsTestSetup setup;
    setup.count = static_cast<u32>(UnitData::get_unit_data_count());
    if (setup.count > 0) {
        setup.bu = new BuildableUnits();
    }
    return setup;
}

//================================================================================================================================
//=> - Static unit-data validation -
//================================================================================================================================

void verify_unit_type (const UnitTypeStats& stats, cstr test_name) {
    note_result(!stats.name.empty(), test_name, " - name not empty");
    note_result(stats.cost > 0, test_name, " - cost > 0");
    note_result(stats.movement_speed > 0, test_name, " - movement_speed > 0");
    (void)stats.attack;
    (void)stats.defense;
}

void test_unit_data_basic () {
    u32 count = static_cast<u32>(UnitData::get_unit_data_count());
    const UnitTypeStats* units = UnitData::get_unit_data_array();

    if (count == 0 || !units) {
        note_result(false, "UnitData basic: no units found");
        summarize_test_results();
        return;
    }

    note_result(true, "UnitData basic: units present");

    u32 checked = 0;
    for (u32 i = 0; i < count && checked < 10; ++i) {
        str name = "UnitData basic idx " + std::to_string(i);
        verify_unit_type(units[i], name.c_str());
        ++checked;
    }

    summarize_test_results();
}

//================================================================================================================================
//=> - BuildableUnits tests -
//================================================================================================================================

void test_buildable_units_initial_state () {
    BuildableUnitsTestSetup setup = setup_buildable_units_test();
    if (setup.count == 0 || !setup.bu) {
        note_result(false, "BuildableUnits initial: no units found");
        summarize_test_results();
        return;
    }

    bool all_locked = true;
    for (u32 i = 0; i < setup.count && i < 64; ++i) { 
        bool can = setup.bu->can_build(static_cast<u16>(i));
        if (can) {
            all_locked = false;
            break;
        }
    }

    note_result(all_locked, "BuildableUnits initial: all units not buildable");
    summarize_test_results();
}

void test_buildable_units_single_set () {
    BuildableUnitsTestSetup setup = setup_buildable_units_test();
    if (setup.count == 0 || !setup.bu) {
        note_result(false, "BuildableUnits single set: no units found");
        summarize_test_results();
        return;
    }
    UnitAssessor assessor(setup.bu);
    u16 idx = 0;
    assessor.set_buildable(idx);
    bool can = setup.bu->can_build(idx);
    note_result(can, "BuildableUnits single set: idx 0 becomes buildable");
    if (setup.count > 1) {
        bool other_can = setup.bu->can_build(static_cast<u16>(setup.count - 1));
        note_result(!other_can, "BuildableUnits single set: last idx still not buildable");
    }

    summarize_test_results();
}

void test_buildable_units_multiple_indices () {
    BuildableUnitsTestSetup setup = setup_buildable_units_test();
    if (setup.count < 3 || !setup.bu) {
        note_result(false, "BuildableUnits multiple indices: need at least 3 units");
        summarize_test_results();
        return;
    }

    UnitAssessor assessor(setup.bu);
    u16 idx_a = 0;
    u16 idx_b = static_cast<u16>(setup.count / 2);
    u16 idx_c = static_cast<u16>(setup.count - 1);

    assessor.set_buildable(idx_a);
    assessor.set_buildable(idx_b);
    assessor.set_buildable(idx_c);

    note_result(setup.bu->can_build(idx_a), "BuildableUnits multiple: idx A buildable");
    note_result(setup.bu->can_build(idx_b), "BuildableUnits multiple: idx B buildable");
    note_result(setup.bu->can_build(idx_c), "BuildableUnits multiple: idx C buildable");

    summarize_test_results();
}

void test_buildable_units_bounds () {
    BuildableUnitsTestSetup setup = setup_buildable_units_test();
    if (setup.count == 0 || !setup.bu) {
        note_result(false, "BuildableUnits bounds: no units found");
        summarize_test_results();
        return;
    }
    UnitAssessor assessor(setup.bu);
    assessor.set_buildable(0);
    note_result(setup.bu->can_build(0), "BuildableUnits bounds: idx 0 buildable");
    if (setup.count > 1) {
        u16 last = static_cast<u16>(setup.count - 1);
        assessor.set_buildable(last);
        note_result(setup.bu->can_build(last), "BuildableUnits bounds: last idx buildable");
    }
    u16 oob = static_cast<u16>(setup.count);
    assessor.set_buildable(oob);
    bool can_oob = setup.bu->can_build(oob);
    note_result(!can_oob, "BuildableUnits bounds: out-of-bounds idx not buildable");

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
    UnitData::load_static_data("../game_config.units");

    test_unit_data_basic();
    test_buildable_units_initial_state();
    test_buildable_units_single_set();
    test_buildable_units_multiple_indices();
    test_buildable_units_bounds();

    std::printf("=======================================================\n");
    std::printf(" TESTING UNITS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
