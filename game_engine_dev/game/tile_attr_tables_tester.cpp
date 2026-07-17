//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "tile_attribute_static_key.h"

//================================================================================================================================
//=> - Globals -
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
    if (!g_rt_loader.load("../data_io/runtime_static_loader_lib.so", "../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

static bool same_row (const TileAttributeStaticDataStruct& a, const TileAttributeStaticDataStruct& b) {
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

static bool is_zero (const TileAttributeStaticDataStruct& a) {
    TileAttributeStaticDataStruct z = {};
    return same_row(a, z);
}

enum {
    k_kind_terr = 0,
    k_kind_clim = 1,
    k_kind_ov = 2,
    k_kind_road = 3,
    k_kind_riv = 4
};

struct ExpectRow {
    cstr m_name;
    u8 m_kind;
    u8 m_id;
};

static const ExpectRow k_expect[] = {
    {"TERR_NONE", k_kind_terr, TERR_NONE[0]},
    {"TERR_OCEAN", k_kind_terr, TERR_OCEAN[0]},
    {"TERR_SEA", k_kind_terr, TERR_SEA[0]},
    {"TERR_COASTAL", k_kind_terr, TERR_COASTAL[0]},
    {"TERR_PLAINS", k_kind_terr, TERR_PLAINS[0]},
    {"TERR_HILLS", k_kind_terr, TERR_HILLS[0]},
    {"TERR_MOUNTAINS", k_kind_terr, TERR_MOUNTAINS[0]},
    {"TERR_VOLCANO", k_kind_terr, TERR_VOLCANO[0]},
    {"TERR_INLAND_SEA", k_kind_terr, TERR_INLAND_SEA[0]},
    {"TERR_INLAND_LAKE", k_kind_terr, TERR_INLAND_LAKE[0]},
    {"TERR_TILE_SENTINEL", k_kind_terr, TERR_TILE_SENTINEL[0]},
    {"CLIMATE_NONE", k_kind_clim, CLIMATE_NONE},
    {"CLIMATE_PLAINS", k_kind_clim, CLIMATE_PLAINS},
    {"CLIMATE_DESERT", k_kind_clim, CLIMATE_DESERT},
    {"CLIMATE_GRASSLAND", k_kind_clim, CLIMATE_GRASSLAND},
    {"CLIMATE_BLACK_SOIL", k_kind_clim, CLIMATE_BLACK_SOIL},
    {"OV_NONE", k_kind_ov, OV_NONE[0]},
    {"OV_FORESTS", k_kind_ov, OV_FOREST[0]},
    {"OV_SWAMPS", k_kind_ov, OV_SWAMP[0]},
    {"OV_JUNGLES", k_kind_ov, OV_JUNGLE[0]},
    {"OV_GLACIER", k_kind_ov, OV_GLACIER[0]},
    {"OV_RIVERS", k_kind_riv, 0u},
    {"ROAD_NONE", k_kind_road, ROAD_NONE},
    {"ROAD_PATH", k_kind_road, ROAD_PATH},
    {"ROAD_COBBLE", k_kind_road, ROAD_COBBLE},
    {"ROAD_ASPHALT", k_kind_road, ROAD_ASPHALT},
    {"ROAD_RAIL", k_kind_road, ROAD_RAIL},
};

static const TileAttributeStaticDataStruct& slot_for (u8 kind, u8 id) {
    if (kind == k_kind_terr) {
        return TileAttrTables::terr(id);
    }
    if (kind == k_kind_clim) {
        return TileAttrTables::clim(id);
    }
    if (kind == k_kind_ov) {
        return TileAttrTables::ov(id);
    }
    if (kind == k_kind_road) {
        return TileAttrTables::road(id);
    }
    return TileAttrTables::riv();
}

static bool find_src (cstr name, TileAttributeStaticDataStruct* out) {
    const TileAttributeStaticData& src = g_rt_statics->tile_attribute();
    const u16 n = src.get_item_count();
    for (u16 i = 0; i < n; ++i) {
        const TileAttributeStaticDataKey key = TileAttributeStaticDataKey::from_raw(i);
        if (std::strcmp(src.get_name(key), name) == 0) {
            *out = src.get_item(key);
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - Main -
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
    note_result(TileAttrTables::terr_n() == 16u, "terr table size 16");
    note_result(TileAttrTables::clim_n() == 5u, "clim table size 5");
    note_result(TileAttrTables::ov_n() == 16u, "ov table size 16");
    note_result(TileAttrTables::road_n() == 5u, "road table size 5");

    note_result(TileAttrTables::terr(TERR_HILLS[0]).mvt_cost == 2000u, "TERR_HILLS mvt_cost");
    note_result(TileAttrTables::terr(TERR_HILLS[0]).food == -1, "TERR_HILLS food");
    note_result(TileAttrTables::terr(TERR_HILLS[0]).defense_mod == 100u, "TERR_HILLS defense_mod");
    note_result(TileAttrTables::terr(TERR_PLAINS[0]).attack_mod == 50u, "TERR_PLAINS attack_mod");
    note_result(TileAttrTables::clim(CLIMATE_GRASSLAND).food == 3, "CLIMATE_GRASSLAND food");
    note_result(TileAttrTables::clim(CLIMATE_BLACK_SOIL).food == 4, "CLIMATE_BLACK_SOIL food");
    note_result(TileAttrTables::clim(CLIMATE_BLACK_SOIL).commerce == 1u, "CLIMATE_BLACK_SOIL commerce");
    note_result(TileAttrTables::ov(OV_FOREST[0]).mvt_cost == 1000u, "OV_FOREST mvt_cost");
    note_result(TileAttrTables::ov(OV_FOREST[0]).food == -1, "OV_FOREST food");
    note_result(TileAttrTables::ov(OV_FOREST[0]).defense_mod == 100u, "OV_FOREST defense_mod");
    note_result(TileAttrTables::road(ROAD_RAIL).mvt_cost == 100u, "ROAD_RAIL mvt_cost");
    note_result(TileAttrTables::riv().mvt_cost == 500u, "OV_RIVERS riv mvt_cost");
    note_result(TileAttrTables::riv().defense_mod == 200u, "OV_RIVERS riv defense_mod");
    note_result(is_zero(TileAttrTables::terr(10u)), "terr gap id 10 is zero");
    note_result(is_zero(TileAttrTables::ov(10u)), "ov gap id 10 is zero");
    note_result(is_zero(TileAttrTables::terr(255u)), "terr oob returns zero");

    bool all_match = true;
    const u32 expect_n = static_cast<u32>(sizeof(k_expect) / sizeof(k_expect[0]));
    for (u32 i = 0; i < expect_n; ++i) {
        TileAttributeStaticDataStruct src = {};
        if (!find_src(k_expect[i].m_name, &src)) {
            all_match = false;
            break;
        }
        if (!same_row(slot_for(k_expect[i].m_kind, k_expect[i].m_id), src)) {
            all_match = false;
            break;
        }
    }
    note_result(all_match, "all config rows match id slots");
    note_result(g_rt_statics->tile_attribute().get_item_count() == expect_n, "statics count matches name map");

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
//=> - End of file -
//================================================================================================================================
