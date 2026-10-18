//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "game_map_defs.h"
#include "map_overlay_enum.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"

//================================================================================================================================
//= - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

//================================================================================================================================
//= - Helpers -
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

static bool load_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

static bool same_row (const TileAttrRow& a, const TileAttrRow& b) {
    return a.mvt_cost == b.mvt_cost
        && a.food == b.food
        && a.production == b.production
        && a.commerce == b.commerce
        && a.culture == b.culture
        && a.science == b.science
        && a.religion == b.religion
        && a.attack_mod == b.attack_mod
        && a.defense_mod == b.defense_mod;
}

static bool is_zero (const TileAttrRow& a) {
    TileAttrRow z = {};
    return same_row(a, z);
}

//================================================================================================================================
//= - Main -
//================================================================================================================================

int main () {
    note_result(load_statics(), "load runtime statics");
    if (g_rt_statics == nullptr) {
        std::printf("=======================================================\n");
        std::printf(" TESTING TILE ATTR TABLES: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return total_test_fails > 0 ? 1 : 0;
    }

    note_result(TileAttrTables::setup(*g_rt_statics), "setup tile attr tables");
    note_result(TileAttrTables::ready(), "tables ready");
    note_result(TileAttrTables::terr_n() == g_rt_statics->map_terrain().get_item_count(), "terr_n matches catalog");
    note_result(TileAttrTables::clim_n() == g_rt_statics->map_climate().get_item_count(), "clim_n matches catalog");
    note_result(TileAttrTables::ov_n() == g_rt_statics->map_overlay().get_item_count(), "ov_n matches catalog");
    note_result(TileAttrTables::road_n() == 6u, "road_n is six Road rows");

    note_result(TileAttrTables::terr(TERR_HILLS[0]).mvt_cost == 2000u, "TERR_HILLS mvt_cost");
    note_result(TileAttrTables::terr(TERR_HILLS[0]).food == -1, "TERR_HILLS food");
    note_result(TileAttrTables::terr(TERR_HILLS[0]).defense_mod == 1u, "TERR_HILLS defense_mod");
    note_result(TileAttrTables::terr(TERR_PLAINS[0]).attack_mod == 1u, "TERR_PLAINS attack_mod");
    note_result(TileAttrTables::clim(CLIMATE_GRASSLAND).food == 2, "CLIMATE_GRASSLAND food");
    note_result(TileAttrTables::clim(CLIMATE_BLACK_SOIL).food == 3, "CLIMATE_BLACK_SOIL food");
    note_result(TileAttrTables::clim(CLIMATE_BLACK_SOIL).commerce == 1u, "CLIMATE_BLACK_SOIL commerce");
    note_result(TileAttrTables::ov(static_cast<u8>(MapOverlay::Forest)).mvt_cost == 1000u, "Forest mvt_cost");
    note_result(TileAttrTables::ov(static_cast<u8>(MapOverlay::Forest)).food == -1, "Forest food");
    note_result(TileAttrTables::ov(static_cast<u8>(MapOverlay::Forest)).defense_mod == 1u, "Forest defense_mod");
    note_result(TileAttrTables::road(ROAD_RAIL).mvt_cost == 100u, "ROAD_RAIL mvt_cost");
    note_result(TileAttrTables::riv().mvt_cost == 500u, "River mvt_cost");
    note_result(TileAttrTables::riv().defense_mod == 2u, "River defense_mod");
    note_result(is_zero(TileAttrTables::terr(255u)), "terr oob returns zero");
    note_result(is_zero(TileAttrTables::ov(255u)), "ov oob returns zero");

    TileAttrTables::clear();
    note_result(!TileAttrTables::ready(), "clear drops ready");
    note_result(is_zero(TileAttrTables::terr(TERR_HILLS[0])), "clear zeroes terr");
    note_result(is_zero(TileAttrTables::riv()), "clear zeroes riv");

    std::printf("=======================================================\n");
    std::printf(" TESTING TILE ATTR TABLES: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//= - End of file -
//================================================================================================================================
