//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "game_map_defs.h"
#include "map_overlay_enum.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "unit_movement_mng.h"

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
    if (!g_rt_loader.load("../data_io/runtime_static_loader_lib.so", "../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

static cstr terr_lbl (u8 id) {
    if (id == TERR_NONE[0]) {
        return "TERR_NONE";
    }
    if (id == TERR_OCEAN[0]) {
        return "TERR_OCEAN";
    }
    if (id == TERR_SEA[0]) {
        return "TERR_SEA";
    }
    if (id == TERR_COASTAL[0]) {
        return "TERR_COASTAL";
    }
    if (id == TERR_PLAINS[0]) {
        return "TERR_PLAINS";
    }
    if (id == TERR_HILLS[0]) {
        return "TERR_HILLS";
    }
    if (id == TERR_MOUNTAINS[0]) {
        return "TERR_MOUNTAINS";
    }
    if (id == TERR_INLAND_SEA[0]) {
        return "TERR_INLAND_SEA";
    }
    if (id == TERR_INLAND_LAKE[0]) {
        return "TERR_INLAND_LAKE";
    }
    return "TERR_OTHER";
}

static cstr clim_lbl (u8 id) {
    if (id == CLIMATE_NONE) {
        return "CLIMATE_NONE";
    }
    if (id == CLIMATE_GRASSLAND) {
        return "CLIMATE_GRASSLAND";
    }
    if (id == CLIMATE_PLAINS) {
        return "CLIMATE_PLAINS";
    }
    if (id == CLIMATE_DESERT) {
        return "CLIMATE_DESERT";
    }
    if (id == CLIMATE_BLACK_SOIL) {
        return "CLIMATE_BLACK_SOIL";
    }
    return "CLIMATE_OTHER";
}

static cstr ov_lbl (u8 id) {
    if (id == static_cast<u8>(MapOverlay::None)) {
        return "None";
    }
    if (id == static_cast<u8>(MapOverlay::Swamp)) {
        return "Swamp";
    }
    if (id == static_cast<u8>(MapOverlay::Forest)) {
        return "Forest";
    }
    if (id == static_cast<u8>(MapOverlay::Jungle)) {
        return "Jungle";
    }
    if (id == static_cast<u8>(MapOverlay::Glacier)) {
        return "Glacier";
    }
    return "OV_OTHER";
}

static void print_tbl (const UnitMovementMngMvtTbl& t) {
    static const u8 k_terr[] = {
        TERR_NONE[0], TERR_OCEAN[0], TERR_SEA[0], TERR_COASTAL[0],
        TERR_PLAINS[0], TERR_HILLS[0], TERR_MOUNTAINS[0],
        TERR_INLAND_SEA[0], TERR_INLAND_LAKE[0],
    };
    std::printf("--- heap tables: terr_n=%u clim_n=%u ov_n=%u bytes=%u ---\n",
        t.m_terr_n, t.m_clim_n, t.m_ov_n, t.m_bytes);
    std::printf("--- terrain cost table ---\n");
    for (u16 i = 0; i < 9u; ++i) {
        const u8 id = k_terr[i];
        std::printf("  id=%u(%s) cost=%u\n", id, terr_lbl(id), UnitMovementMng::mvt_cost_terr(id));
    }
    std::printf("--- climate cost table ---\n");
    for (u8 id = 0u; id <= CLIMATE_BLACK_SOIL; ++id) {
        std::printf("  id=%u(%s) cost=%u\n", id, clim_lbl(id), UnitMovementMng::mvt_cost_clim(id));
    }
    std::printf("--- overlay cost table ---\n");
    std::printf("  id=%u(%s) cost=%u\n", static_cast<unsigned>(MapOverlay::None), ov_lbl(static_cast<u8>(MapOverlay::None)),
        UnitMovementMng::mvt_cost_ov(static_cast<u8>(MapOverlay::None)));
    std::printf("  id=%u(%s) cost=%u\n", static_cast<unsigned>(MapOverlay::Swamp), ov_lbl(static_cast<u8>(MapOverlay::Swamp)),
        UnitMovementMng::mvt_cost_ov(static_cast<u8>(MapOverlay::Swamp)));
    std::printf("  id=%u(%s) cost=%u\n", static_cast<unsigned>(MapOverlay::Forest), ov_lbl(static_cast<u8>(MapOverlay::Forest)),
        UnitMovementMng::mvt_cost_ov(static_cast<u8>(MapOverlay::Forest)));
    std::printf("  id=%u(%s) cost=%u\n", static_cast<unsigned>(MapOverlay::Jungle), ov_lbl(static_cast<u8>(MapOverlay::Jungle)),
        UnitMovementMng::mvt_cost_ov(static_cast<u8>(MapOverlay::Jungle)));
    std::printf("--- transport cost ---\n");
    std::printf("  river cost=%u\n", t.m_riv);
    std::printf("  road cost=%u\n", t.m_road);
}

//================================================================================================================================
//= - Test -
//================================================================================================================================

void test_mvt_setup () {
    bool ok = load_statics();
    note_result(ok, "load runtime statics");
    if (!ok) {
        return;
    }
    ok = UnitMovementMng::setup_mvt_costs(*g_rt_statics);
    note_result(ok, "setup_mvt_costs");
    note_result(UnitMovementMng::mvt_mapped_count() == UnitMovementMng::mvt_cost_count(),
        "setup mapped count matches item count");
    note_result(UnitMovementMng::mvt_ready(), "mvt_ready");
    note_result(UnitMovementMng::mvt_cost_count() == static_cast<u16>(
        TileAttrTables::terr_n() + TileAttrTables::clim_n() + TileAttrTables::ov_n()
        + TileAttrTables::road_n() + 1u), "mvt_cost_count is catalog sum");
    if (print_level > 0) {
        std::printf("  setup reports mapped %u / %u\n",
            UnitMovementMng::mvt_mapped_count(), UnitMovementMng::mvt_cost_count());
    }
    UnitMovementMngMvtTbl tbl;
    UnitMovementMng::mvt_tables(&tbl);
    if (print_level > 0) {
        print_tbl(tbl);
    }
}

//================================================================================================================================
//= - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    test_mvt_setup();
    std::printf("=======================================================\n");
    std::printf(" MVT SETUP TESTER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//= - End -
//================================================================================================================================
