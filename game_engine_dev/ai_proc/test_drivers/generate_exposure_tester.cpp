//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "city.h"
#include "city_array.h"
#include "game_primitives.h"
#include "game_state.h"
#include "generate_exposure.h"
#include "test_hlp_city_spawning.h"
#include "test_hlp_city_spawning_validate.h"
#include "test_hlp_civ_spawning.h"
#include "test_hlp_map_load.h"
#include "test_hlp_tile_ownership.h"
#include "test_hlp_tile_ownership_validate.h"
#include "test_hlp_unit_spawning.h"
#include "test_hlp_unit_validate.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const cstr G_IN = "/home/w/Projects/simple-map-gen/p1-seed-43/";
static const cstr G_OUT = "/home/w/Projects/simple-map-gen/generate-exposure";
static const cstr G_LIB = "../../data_io/runtime_static_loader_lib.so";
static const cstr G_DATA = "../../";

static const u16 k_lat = 10u;
static const u16 k_jit_pct = 30u;
static const u32 k_seed = 43u;
static const u8 k_exp_lim = 10u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool ensure_out_dir () {
    return mkdir(G_OUT, 0755) == 0 || errno == EEXIST;
}

static void put_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i] = r;
    rgb[i + 1u] = g;
    rgb[i + 2u] = b;
}

static void draw_plus (u8* rgb, u16 w, u16 h, u16 cx, u16 cy, u8 r, u8 g, u8 b) {
    const i32 x0 = static_cast<i32>(cx);
    const i32 y0 = static_cast<i32>(cy);
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 t = -1; t <= 1; ++t) {
            put_px(rgb, w, h, x0 + dx, y0 + t, r, g, b);
        }
    }
    for (i32 dy = -2; dy <= 2; ++dy) {
        for (i32 t = -1; t <= 1; ++t) {
            put_px(rgb, w, h, x0 + t, y0 + dy, r, g, b);
        }
    }
}

static bool save_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static u8 find_dmax (const Whiteboard_1B& board) {
    u8 mx = 0;
    const u32 n = static_cast<u32>(board.w()) * static_cast<u32>(board.h());
    const u8* p = board.get_iter_ptr();
    for (u32 i = 0; i < n; ++i) {
        if (p[i] == GenerateExposure::k_none) {
            continue;
        }
        if (p[i] > mx) {
            mx = p[i];
        }
    }
    return mx;
}

static bool save_exposure_ppm (
    const GameState& s,
    const Whiteboard_1B& board,
    u16 pa,
    u16 pb,
    cstr path) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    const u8 dmax = find_dmax(board);
    const u32 span = dmax == 0u ? 1u : static_cast<u32>(dmax);
    u8* rgb = new u8[static_cast<size_t>(w) * static_cast<size_t>(h) * 3u];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 own = s.m_map.get_civ_owner(x, y);
            const u32 i = tidx(w, x, y) * 3u;
            if (own == static_cast<u8>(pb)) {
                rgb[i] = 50;
                rgb[i + 1u] = 90;
                rgb[i + 2u] = 220;
                continue;
            }
            if (own != static_cast<u8>(pa)) {
                rgb[i] = 36;
                rgb[i + 1u] = 36;
                rgb[i + 2u] = 40;
                continue;
            }
            const u8 d = board.rd(x, y);
            if (d == GenerateExposure::k_none) {
                rgb[i] = 70;
                rgb[i + 1u] = 70;
                rgb[i + 2u] = 70;
                continue;
            }
            const u8 v = static_cast<u8>(40u + (215u * (span - static_cast<u32>(d))) / span);
            rgb[i] = v;
            rgb[i + 1u] = static_cast<u8>(v / 3u);
            rgb[i + 2u] = 20;
        }
    }
    const u16 cn = s.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        if (c->get_owner() == pa) {
            const u8 d = board.rd(cx, cy);
            if (d < k_exp_lim) {
                draw_plus(rgb, w, h, cx, cy, 255, 255, 40);
            } else {
                draw_plus(rgb, w, h, cx, cy, 20, 20, 20);
            }
        } else if (c->get_owner() == pb) {
            draw_plus(rgb, w, h, cx, cy, 40, 40, 255);
        }
    }
    const bool ok = save_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!ensure_out_dir()) {
        std::printf("*** FAILED mkdir %s\n", G_OUT);
        return 1;
    }
    GameState state;
    if (!TestHlpMapLoad::load(state, G_IN, G_LIB, G_DATA)) {
        std::printf("*** FAILED map load\n");
        return 1;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    WhiteboardMng::init(w, h);
    Whiteboard_2B ov_wb("generate_exposure_tester", "ov", 0u);
    if (!ov_wb.ok()) {
        std::printf("*** FAILED ov whiteboard\n");
        WhiteboardMng::terminate();
        return 1;
    }
    u16 seed_x[TestHlpTileOwnership::k_seed_cap];
    u16 seed_y[TestHlpTileOwnership::k_seed_cap];
    u16 sec_n = 0;
    if (!TestHlpTileOwnership::mock_sectors(state, ov_wb.get_iter_ptr(), &sec_n, seed_x, seed_y)) {
        std::printf("*** FAILED mock_sectors\n");
        WhiteboardMng::terminate();
        return 1;
    }
    u16 pa = 0;
    u16 pb = 0;
    u32 bord_n = 0;
    if (!TestHlpTileOwnership::find_wide_border(ov_wb.get_iter_ptr(), w, h, sec_n, &pa, &pb, &bord_n)) {
        std::printf("*** FAILED find_wide_border\n");
        WhiteboardMng::terminate();
        return 1;
    }
    TestHlpTileOwnershipValidate::print_summary(sec_n, pa, pb, bord_n);
    if (!TestHlpCivSpawning::init_seats(state, sec_n, pa, pb)) {
        std::printf("*** FAILED civ init_seats\n");
        WhiteboardMng::terminate();
        return 1;
    }
    TestHlpTileOwnership::apply_owners(state, ov_wb.get_iter_ptr());
    const u16 city_n = TestHlpCitySpawning::spawn_lattice(
        state, ov_wb.get_iter_ptr(), pa, pb, k_lat, k_jit_pct, k_seed);
    if (city_n == 0u) {
        std::printf("*** FAILED spawn_lattice\n");
        WhiteboardMng::terminate();
        return 1;
    }
    TestHlpCitySpawningValidate::print_summary(state, pa, pb);
    if (!TestHlpUnitSpawning::spawn_at_cities(state, pa, "Spearman", 2u)
        || !TestHlpUnitSpawning::spawn_at_cities(state, pa, "Swordsman", 3u)
        || !TestHlpUnitSpawning::spawn_at_cities(state, pa, "Catapult", 2u)) {
        std::printf("*** FAILED spawn attacker units\n");
        WhiteboardMng::terminate();
        return 1;
    }
    if (!TestHlpUnitSpawning::spawn_at_cities(state, pb, "Spearman", 2u)) {
        std::printf("*** FAILED spawn defender units\n");
        WhiteboardMng::terminate();
        return 1;
    }
    TestHlpUnitValidate::print_counts(state);

    Whiteboard_1B board("generate_exposure_tester", "exp", 0u);
    if (!board.ok()) {
        std::printf("*** FAILED exposure whiteboard\n");
        WhiteboardMng::terminate();
        return 1;
    }
    const auto t0 = std::chrono::steady_clock::now();
    const bool gen_ok = GenerateExposure::generate(state, pa, pb, board);
    const auto t1 = std::chrono::steady_clock::now();
    const double gen_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    if (!gen_ok) {
        std::printf("*** FAILED GenerateExposure (%.2f us)\n", gen_us);
        WhiteboardMng::terminate();
        return 1;
    }
    const u8 dmax = find_dmax(board);
    u32 reached = 0;
    u32 seeds = 0;
    const u32 tn = static_cast<u32>(w) * static_cast<u32>(h);
    const u8* p = board.get_iter_ptr();
    for (u32 i = 0; i < tn; ++i) {
        if (p[i] == GenerateExposure::k_none) {
            continue;
        }
        reached++;
        if (p[i] == 0u) {
            seeds++;
        }
    }
    u16 exp_n = 0;
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != pa) {
            continue;
        }
        if (board.rd(c->get_x(), c->get_y()) < k_exp_lim) {
            exp_n++;
        }
    }
    char path[512];
    if (std::snprintf(path, sizeof(path), "%s/exposure.ppm", G_OUT) <= 0) {
        WhiteboardMng::terminate();
        return 1;
    }
    if (!save_exposure_ppm(state, board, pa, pb, path)) {
        std::printf("*** FAILED save %s\n", path);
        WhiteboardMng::terminate();
        return 1;
    }
    std::printf("GenerateExposure gen_us=%.2f seeds=%u reached=%u dmax=%u\n", gen_us, seeds, reached, (u32)dmax);
    std::printf("exposed_cities=%u (lim=%u) view=%u enemy=%u\n", (u32)exp_n, (u32)k_exp_lim, (u32)pa, (u32)pb);
    std::printf("out=%s\n", path);
    std::printf("*** PASSED generate_exposure\n");
    WhiteboardMng::terminate();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
