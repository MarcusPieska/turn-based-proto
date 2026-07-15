//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "mvt_cost_static_key.h"
#include "unit_movement_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* k_red = "\033[31m";
static const char* k_rst = "\033[0m";

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
    if (id == OVERLAY_NONE) {
        return "OVERLAY_NONE";
    }
    if (id == OV_SWAMP[0]) {
        return "OVERLAY_SWAMPS";
    }
    if (id == OV_FOREST[0]) {
        return "OVERLAY_FORESTS";
    }
    if (id == OV_JUNGLE[0]) {
        return "OVERLAY_JUNGLES";
    }
    return "OVERLAY_OTHER";
}

static bool print_map_rows (const RuntimeStatics& st, u16* out_mapped) {
    std::printf("--- mvt_cost config -> game class mapping ---\n");
    u16 mapped = 0u;
    const u16 n = st.mvt_cost().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = st.mvt_cost().get_name(MvtCostStaticDataKey::from_raw(i));
        const u16 cost = st.mvt_cost().get_item(MvtCostStaticDataKey::from_raw(i)).cost;
        u8 gid = 0u;
        u8 kind = 0u;
        if (nm == nullptr || !UnitMovementMng::map_mvt_cost_name(nm, &gid, &kind)) {
            std::printf("%sUNMAPPED: %s (cost=%u)%s\n",
                k_red, (nm != nullptr) ? nm : "(null)",
                static_cast<unsigned>(cost), k_rst);
            continue;
        }
        mapped = static_cast<u16>(mapped + 1u);
        if (kind == 0u) {
            std::printf("  %s -> %s id=%u cost=%u\n", nm, terr_lbl(gid), gid, cost);
        } else if (kind == 1u) {
            std::printf("  %s -> %s id=%u cost=%u\n", nm, clim_lbl(gid), gid, cost);
        } else if (kind == 2u) {
            std::printf("  %s -> %s id=%u cost=%u\n", nm, ov_lbl(gid), gid, cost);
        } else if (kind == 3u) {
            std::printf("  %s -> river transport cost=%u\n", nm, cost);
        } else if (kind == 4u) {
            std::printf("  %s -> road transport cost=%u\n", nm, cost);
        }
    }
    *out_mapped = mapped;
    std::printf("  mapped %u / %u mvt_cost rows\n", mapped, n);
    return mapped == n;
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
    std::printf("  id=%u(%s) cost=%u\n", OVERLAY_NONE, ov_lbl(OVERLAY_NONE), UnitMovementMng::mvt_cost_ov(OVERLAY_NONE));
    std::printf("  id=%u(%s) cost=%u\n", OV_SWAMP[0], ov_lbl(OV_SWAMP[0]), UnitMovementMng::mvt_cost_ov(OV_SWAMP[0]));
    std::printf("  id=%u(%s) cost=%u\n", OV_FOREST[0], ov_lbl(OV_FOREST[0]), UnitMovementMng::mvt_cost_ov(OV_FOREST[0]));
    std::printf("  id=%u(%s) cost=%u\n", OV_JUNGLE[0], ov_lbl(OV_JUNGLE[0]), UnitMovementMng::mvt_cost_ov(OV_JUNGLE[0]));
    std::printf("--- transport cost ---\n");
    std::printf("  river cost=%u\n", t.m_riv);
    std::printf("  road cost=%u\n", t.m_road);
}

//================================================================================================================================
//=> - Test -
//================================================================================================================================

void test_mvt_setup () {
    bool ok = load_statics();
    note_result(ok, "load runtime statics");
    if (!ok) {
        return;
    }
    u16 mapped = 0u;
    ok = print_map_rows(*g_rt_statics, &mapped);
    note_result(ok, "mvt_cost mapped count matches item count");
    ok = UnitMovementMng::setup_mvt_costs(*g_rt_statics);
    note_result(ok, "setup_mvt_costs");
    note_result(UnitMovementMng::mvt_mapped_count() == UnitMovementMng::mvt_cost_count(),
        "setup mapped count matches item count");
    note_result(UnitMovementMng::mvt_ready(), "mvt_ready");
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
//=> - Main -
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
//=> - End -
//================================================================================================================================
