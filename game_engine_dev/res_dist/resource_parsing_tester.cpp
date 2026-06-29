//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "resource_io.h"
#include "resource_parsing.h"
#include "resource_placement.h"
#include "p1_gen_climate.h"
#include "p1_tester_chain15.h"
#include "p1_tester_util.h"

typedef const char* cstr;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;
static u8 g_dot_small = 0;

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
        total_test_fails++;
        if (print_level > 0) {
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void summarize_test_results () {
    if (print_level > 0) {
        std::printf("--------------------------------\n");
        std::printf(" Test count: %d\n", test_count);
        std::printf(" Test pass: %d\n", test_pass);
        std::printf(" Test fail: %d\n", test_count - test_pass);
        std::printf("--------------------------------\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

static const ResEntry* find_entry (const ResCatalog& cat, cstr name) {
    for (u16 i = 0; i < cat.m_n; ++i) {
        if (std::strcmp(cat.m_items[i].m_name, name) == 0) {
            return &cat.m_items[i];
        }
    }
    return nullptr;
}

static ResPlcVizPrm viz_prm () {
    return g_dot_small != 0 ? res_plc_viz_prm_small() : res_plc_viz_prm_def();
}

static bool build_ctx (
    const P1_RunPrm& prm,
    const P1_Adj_LandAltitudePrm& lap,
    ResPlcMapCtx* ctx,
    P1_TesterChain15Rslt* chain,
    u8** overlay) 
{
    if (ctx == nullptr || chain == nullptr || overlay == nullptr) {
        return false;
    }
    double sec_i = 0.0;
    if (!p1_build_ensure_input(prm, lap, 22u, chain, &sec_i)) {
        return false;
    }
    P1_Gen_Climate clim_gen(prm);
    if (!clim_gen.generate(chain->m_terrain, chain->m_w, chain->m_h, chain->m_river)
        || !clim_gen.is_valid()) {
        return false;
    }
    *overlay = ResPlcOverlay::build_stub(chain->m_w, chain->m_h, chain->m_terrain,
        clim_gen.result().m_ov.data(), chain->m_river);
    if (*overlay == nullptr) {
        return false;
    }
    ctx->m_w = chain->m_w;
    ctx->m_h = chain->m_h;
    ctx->m_terrain = chain->m_terrain;
    ctx->m_climate = clim_gen.result().m_ov.data();
    ctx->m_river = chain->m_river;
    ctx->m_overlay = *overlay;
    return true;
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_res_parse_catalog () {
    ResIoPath paths("../");
    ResIoFile file;
    ResParser parser;
    note_result(file.load(paths.resources_path()), "load file");
    note_result(parser.parse_file(file), "parse catalog");
    const ResCatalog& cat = parser.catalog();
    note_result(cat.m_n == 60u, "catalog count");
    note_result(find_entry(cat, "Iron") != nullptr, "Iron found");
    note_result(find_entry(cat, "Furs") != nullptr, "Furs found");
    parser.release();
}

void test_res_parse_fields () {
    ResIoPath paths("../");
    ResIoFile file;
    ResParser parser;
    file.load(paths.resources_path());
    parser.parse_file(file);
    const ResCatalog& cat = parser.catalog();
    const ResEntry* city = find_entry(cat, "City");
    const ResEntry* iron = find_entry(cat, "Iron");
    const ResEntry* furs = find_entry(cat, "Furs");
    note_result(city != nullptr && city->m_has_plc == 0, "City has no placement");
    note_result(iron != nullptr && iron->m_has_plc == 1, "Iron has placement");
    note_result(iron != nullptr && iron->m_plc.m_res_wt == 4u, "Iron resource weight");
    note_result(iron != nullptr && iron->m_plc.m_quad_n == 2u, "Iron quad count");
    note_result(furs != nullptr && furs->m_plc.m_quad_n == 2u, "Furs quad count");
    note_result(iron != nullptr && iron->m_food == 0u && iron->m_shields == 2u, "Iron yields");
    note_result(std::strcmp(iron->m_tech, "Iron Working") == 0, "Iron tech");
    parser.release();
}

static i32 run_parse_viz (const P1_RunPrm& prm, const P1_Adj_LandAltitudePrm& lap) {
    ResIoPath paths("../");
    ResIoFile file;
    ResParser parser;
    if (!file.load(paths.resources_path()) || !parser.parse_file(file)) {
        return -1;
    }
    const ResCatalog& cat = parser.catalog();
    P1_TesterChain15Rslt chain = {};
    ResPlcMapCtx ctx = {};
    u8* overlay = nullptr;
    if (!build_ctx(prm, lap, &ctx, &chain, &overlay)) {
        parser.release();
        return -1;
    }
    const u32 npx = (u32)chain.m_w * (u32)chain.m_h;
    u8* marks = new u8[npx];
    if (marks == nullptr) {
        delete[] overlay;
        p1_free_chain15(&chain);
        parser.release();
        return -1;
    }
    const ResPlcVizPrm vprm = viz_prm();
    u32 img_n = 0;
    char fname[96];
    char out_path[384];
    for (u16 ri = 0; ri < cat.m_n; ++ri) {
        const ResEntry& entry = cat.m_items[ri];
        if (entry.m_has_plc == 0) {
            continue;
        }
        const u32 hit = ResPlcMatch::mark_all_rules(ctx, entry, marks, npx);
        std::snprintf(fname, sizeof(fname), "Possible_%s.ppm", entry.m_name);
        if (!ResPlcViz::make_out_path(prm.m_seed, fname, out_path, sizeof(out_path))) {
            delete[] marks;
            delete[] overlay;
            p1_free_chain15(&chain);
            parser.release();
            return -1;
        }
        if (!ResPlcViz::save_pair_img(out_path, ctx, marks, npx, vprm)) {
            delete[] marks;
            delete[] overlay;
            p1_free_chain15(&chain);
            parser.release();
            return -1;
        }
        std::printf("saved: %s (hits=%u)\n", out_path, hit);
        ++img_n;
    }
    std::printf("resource_parsing images: %u (seed=%u, %ux%u pair)\n",
        img_n, prm.m_seed, (unsigned)(chain.m_w * 2u), (unsigned)chain.m_h);
    delete[] marks;
    delete[] overlay;
    p1_free_chain15(&chain);
    parser.release();
    return (i32)img_n;
}

void test_suite_res_parsing () {
    test_res_parse_catalog();
    summarize_test_results();
    test_res_parse_fields();
    summarize_test_results();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    if (argc > 2) {
        g_dot_small = (u8)std::atoi(argv[2]);
    }
    test_suite_res_parsing();
    P1_RunPrm prm = p1_run_prm_def();
    P1_Adj_LandAltitudePrm lap = p1_tester_land_altitude_prm();
    if (argc >= 4) {
        prm.m_seed = (u32)std::strtoul(argv[3], nullptr, 10);
        if (argc >= 5) {
            prm.m_w = (u16)std::strtoul(argv[4], nullptr, 10);
        }
        if (argc >= 6) {
            prm.m_h = (u16)std::strtoul(argv[5], nullptr, 10);
        }
    } else {
        prm.m_seed = p1_read_seed_file();
    }
    const clock_t t0 = clock();
    const i32 img_n = run_parse_viz(prm, lap);
    const clock_t t1 = clock();
    if (img_n < 0) {
        std::printf("resource_parsing viz failed\n");
        return total_test_fails > 0 ? total_test_fails : -1;
    }
    const double sec = (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
    std::printf("=======================================================\n");
    std::printf(" TESTING RESOURCE PARSING: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf(" parsing viz: %d images in %.3f s (dot=%s)\n",
        img_n, sec, g_dot_small != 0 ? "1px" : "large");
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
