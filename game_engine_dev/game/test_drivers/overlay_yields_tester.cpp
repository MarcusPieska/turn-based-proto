//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "map_overlay_static_key.h"
#include "overlay_yields.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "tile_usage.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";

int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void note_result (bool cond, cstr msg) {
    total_tests_run++;
    if (cond) {
        if (print_level > 1) {
            std::printf("*** TEST PASSED: %s\n", msg);
        }
        return;
    }
    total_test_fails++;
    if (print_level > 0) {
        std::printf("*** TEST FAILED: %s\n", msg);
    }
}

static cstr ov_nm (const RuntimeStatics& st, u16 ov) {
    if (ov >= st.map_overlay().get_item_count()) {
        return "?";
    }
    cstr nm = st.map_overlay().get_name(MapOverlayStaticDataKey::from_raw(ov));
    return nm != nullptr ? nm : "?";
}

static cstr intent_nm (TileAssignIntent intent) {
    if (intent == TILE_ASSIGN_FOOD) {
        return "FOOD";
    }
    if (intent == TILE_ASSIGN_PROD) {
        return "PROD";
    }
    return "?";
}

static void print_tots (const RuntimeStatics& st) {
    std::printf(" fully developed totals (baseline plains terr + plains climate, no river):\n");
    const u16 n = OverlayYields::ov_n();
    for (u16 ov = 0; ov < n; ++ov) {
        const OvYldTot t = OverlayYields::tot(ov);
        std::printf("  %-12s  food=%d  prod=%d  comm=%d  imps=%u%s\n",
            ov_nm(st, ov),
            static_cast<int>(t.m_food),
            static_cast<int>(t.m_prod),
            static_cast<int>(t.m_comm),
            static_cast<unsigned>(t.m_imp_n),
            OverlayYields::is_res(ov) ? "  [resource]" : "");
    }
}

static void print_rank (const RuntimeStatics& st, TileAssignIntent intent) {
    u16 n = 0;
    const u16* ids = OverlayYields::rank(intent, &n);
    std::printf(" %s rank (best first):\n", intent_nm(intent));
    if (ids == nullptr || n == 0u) {
        std::printf("  (empty)\n");
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        const u16 ov = ids[i];
        const OvYldTot t = OverlayYields::tot(ov);
        std::printf("  %u. %-12s  food=%d  prod=%d  comm=%d\n",
            static_cast<unsigned>(i + 1u),
            ov_nm(st, ov),
            static_cast<int>(t.m_food),
            static_cast<int>(t.m_prod),
            static_cast<int>(t.m_comm));
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "load runtime statics");
    if (total_test_fails > 0) {
        return 1;
    }
    const RuntimeStatics& st = loader.statics();
    note_result(TileAttrTables::setup(st), "TileAttrTables::setup");
    note_result(OverlayYields::setup(st), "OverlayYields::setup");
    if (total_test_fails > 0) {
        OverlayYields::clear();
        TileAttrTables::clear();
        return 1;
    }
    note_result(OverlayYields::ov_n() == st.map_overlay().get_item_count(), "ov_n matches catalog");
    u16 food_n = 0;
    u16 prod_n = 0;
    const u16* food_rk = OverlayYields::rank(TILE_ASSIGN_FOOD, &food_n);
    const u16* prod_rk = OverlayYields::rank(TILE_ASSIGN_PROD, &prod_n);
    note_result(food_rk != nullptr && food_n > 0u, "food rank non-empty");
    note_result(prod_rk != nullptr && prod_n > 0u, "prod rank non-empty");
    note_result(food_n == prod_n, "food/prod rank same candidate count");
    for (u16 i = 0; i < food_n; ++i) {
        note_result(!OverlayYields::is_res(food_rk[i]), "food rank excludes resource overlays");
    }
    for (u16 i = 0; i < prod_n; ++i) {
        note_result(!OverlayYields::is_res(prod_rk[i]), "prod rank excludes resource overlays");
    }

    std::printf("=======================================================\n");
    std::printf(" OVERLAY YIELDS\n");
    std::printf("=======================================================\n");
    print_tots(st);
    print_rank(st, TILE_ASSIGN_FOOD);
    print_rank(st, TILE_ASSIGN_PROD);
    std::printf(" note: resource overlays preempt ranks per-tile via get_res short-circuit\n");
    std::printf("=======================================================\n");
    std::printf(" OVERLAY YIELDS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    OverlayYields::clear();
    TileAttrTables::clear();
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
