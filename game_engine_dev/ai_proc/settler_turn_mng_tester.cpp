//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "city.h"
#include "city_border.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "sense_settling_pts_opt.h"
#include "settler_turn_mng.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/settler-turn-mng";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/settler-turn-mng/game_loop.trace";
static const u32 G_SEED = 42u;
static const u16 G_PLAYERS = 31;
static const u32 G_TURN_CAP = 50u;
static const u32 G_TURN_CAP_EXT = 1000u;
static const u16 G_CLAIM_CULT = 25u;
static u32 g_ppm_every = 10u;
static bool g_time_only = false;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_res[320];

static const u8 k_own_pal[][3] = {
    {220, 40, 40},
    {40, 90, 220},
    {40, 170, 70},
    {220, 110, 30},
    {190, 40, 170},
    {30, 170, 170},
    {150, 70, 30},
    {100, 40, 180},
};
static const u16 k_own_pal_n = static_cast<u16>(sizeof(k_own_pal) / sizeof(k_own_pal[0]));

//================================================================================================================================
//=> - Hot-path timers -
//================================================================================================================================

struct HotTimer {
    const char* name;
    u64 n;
    u64 ns;
};

static HotTimer g_tm_begin = {"SettlerTurnMng::begin", 0, 0};

static void tm_add (HotTimer* t, u64 ns) {
    t->n = t->n + 1u;
    t->ns = t->ns + ns;
}

static void tm_report (const HotTimer& t) {
    const double total_ms = static_cast<double>(t.ns) / 1.0e6;
    const double avg_us = (t.n == 0) ? 0.0 : (static_cast<double>(t.ns) / static_cast<double>(t.n)) / 1.0e3;
    std::printf("  %-32s  calls=%llu  total=%.3f ms  avg=%.3f us\n",
        t.name,
        static_cast<unsigned long long>(t.n),
        total_ms,
        avg_us);
}

//================================================================================================================================
//=> - Setup helpers -
//================================================================================================================================

static bool build_paths () {
    char dir[256];
    if (std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool is_settler_typ (const RuntimeStatics& st, u16 typ_idx) {
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    const u16 ut = st.unit().get_item(uk).type;
    const UnitTypeStaticDataKey tk = UnitTypeStaticDataKey::from_raw(ut);
    return std::strcmp(st.unit_type().get_name(tk), "LAND_SETTLER") == 0;
}

static u16 count_cities (const GameState& state) {
    u16 n = 0;
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c != nullptr && c->get_owner() != U16_KEY_NULL) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

static u16 count_settlers (const GameState& state) {
    if (state.m_statics == nullptr) {
        return 0;
    }
    u16 n = 0;
    const u32 scan_n = static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        if (is_settler_typ(*state.m_statics, u->m_unit_typ_idx)) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

static void claim_city_borders (GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        CityBorder::claim_expand(c->get_x(), c->get_y(), 0, G_CLAIM_CULT, static_cast<u8>(c->get_owner()));
    }
}

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static void set_px (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void shade_own (u8* rgb, u16 w, u16 h, u16 x, u16 y, u16 seat) {
    if (x >= w || y >= h) {
        return;
    }
    const u8* c = k_own_pal[seat % k_own_pal_n];
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = static_cast<u8>((static_cast<u16>(rgb[i + 0]) + static_cast<u16>(c[0])) / 2u);
    rgb[i + 1] = static_cast<u8>((static_cast<u16>(rgb[i + 1]) + static_cast<u16>(c[1])) / 2u);
    rgb[i + 2] = static_cast<u8>((static_cast<u16>(rgb[i + 2]) + static_cast<u16>(c[2])) / 2u);
}

static void desat_px (u8* rgb, u16 w, u16 h, u16 x, u16 y) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    const u16 g = static_cast<u16>((static_cast<u16>(rgb[i + 0]) + static_cast<u16>(rgb[i + 1]) + static_cast<u16>(rgb[i + 2])) / 3u);
    rgb[i + 0] = static_cast<u8>((static_cast<u16>(rgb[i + 0]) + g * 7u) / 8u);
    rgb[i + 1] = static_cast<u8>((static_cast<u16>(rgb[i + 1]) + g * 7u) / 8u);
    rgb[i + 2] = static_cast<u8>((static_cast<u16>(rgb[i + 2]) + g * 7u) / 8u);
}

static void paint_black_mark (u8* rgb, u16 w, u16 h, u16 x, u16 y) {
    set_px(rgb, w, h, x, y, 0, 0, 0);
    if (x > 0) {
        set_px(rgb, w, h, static_cast<u16>(x - 1u), y, 0, 0, 0);
    }
    if (static_cast<u32>(x) + 1u < static_cast<u32>(w)) {
        set_px(rgb, w, h, static_cast<u16>(x + 1u), y, 0, 0, 0);
    }
    if (y > 0) {
        set_px(rgb, w, h, x, static_cast<u16>(y - 1u), 0, 0, 0);
    }
    if (static_cast<u32>(y) + 1u < static_cast<u32>(h)) {
        set_px(rgb, w, h, x, static_cast<u16>(y + 1u), 0, 0, 0);
    }
}

static bool save_turn_ppm (const GameState& state, u32 turn) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(state.m_map.get_climate(x, y), &r, &g, &b);
            if (state.m_map.get_river(x, y) != 0) {
                r = 40;
                g = 100;
                b = 220;
            }
            if (state.m_map.get_terrain(x, y) == TERR_MOUNTAINS[0]) {
                r = 120;
                g = 72;
                b = 40;
            }
            set_px(rgb, w, h, x, y, r, g, b);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (state.m_map.get_settler_blocked(x, y) != 0) {
                desat_px(rgb, w, h, x, y);
            }
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 own = state.m_map.get_civ_owner(x, y);
            if (own != U8_KEY_NULL) {
                shade_own(rgb, w, h, x, y, static_cast<u16>(own));
            }
        }
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        paint_black_mark(rgb, w, h, c->get_x(), c->get_y());
    }
    for (u16 p = 0; p < state.m_player_n; ++p) {
        for (u16 s = 0; s < static_cast<u16>(SETTLER_MISSION_SLOTS); ++s) {
            u16 tx = U16_KEY_NULL;
            u16 ty = U16_KEY_NULL;
            if (!SettlerTurnMng::tgt_xy(p, s, &tx, &ty)) {
                continue;
            }
            set_px(rgb, w, h, tx, ty, 255, 255, 0);
        }
    }
    char path[384];
    if (std::snprintf(path, sizeof(path), "%s/turn_%04u.ppm", G_OUT_DIR, turn) <= 0) {
        delete[] rgb;
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    bool extend = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "extend") == 0) {
            extend = true;
        }
        if (std::strcmp(argv[i], "time_only") == 0) {
            g_time_only = true;
        }
    }
    const u32 turn_cap = extend ? G_TURN_CAP_EXT : G_TURN_CAP;
    if (!build_paths()) {
        std::printf("path build failed\n");
        return 1;
    }
    if (!ensure_out_dir()) {
        std::printf("out dir failed\n");
        return 1;
    }
    GameSetup setup;
    GameState state;
    MapPpmPaths paths = {};
    paths.m_terr = g_terr;
    paths.m_clim = g_clim;
    paths.m_riv = g_riv;
    paths.m_ov = nullptr;
    paths.m_res = g_res;
    if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
        std::printf("setup_new_game failed\n");
        return 1;
    }
    WhiteboardMng::init(state.m_map.width(), state.m_map.height());
    const auto tb0 = std::chrono::steady_clock::now();
    const bool began = SettlerTurnMng::begin(state);
    const auto tb1 = std::chrono::steady_clock::now();
    tm_add(&g_tm_begin, static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(tb1 - tb0).count()));
    if (!began) {
        std::printf("SettlerTurnMng::begin failed\n");
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }
    claim_city_borders(state);
    for (u16 p = 0; p < state.m_player_n; ++p) {
        state.m_player_states[p].m_target_settlements = SETTLER_MISSION_SLOTS;
    }
    state.m_turn_limit = turn_cap;
    state.m_current_turn = 0;
    const u16 cities0 = count_cities(state);
    std::printf("*** start players=%u cities=%u turn_cap=%u extend=%d ppm_every=%u time_only=%d\n",
        state.m_player_n, cities0, turn_cap, extend ? 1 : 0, g_ppm_every, g_time_only ? 1 : 0);
    if (!g_time_only) {
        if (!save_turn_ppm(state, 0)) {
            std::printf("save turn 0 failed\n");
            SettlerTurnMng::clear();
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
    }
    GameLoop loop;
    if (!loop.begin(&state, G_TRACE)) {
        std::printf("GameLoop::begin failed\n");
        SettlerTurnMng::clear();
        state.clear();
        return 1;
    }
    const auto t_loop0 = std::chrono::steady_clock::now();
    while (state.m_current_turn < turn_cap) {
        if (!loop.step()) {
            std::printf("\nGameLoop::step failed at turn %u\n", state.m_current_turn);
            SettlerTurnMng::clear();
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
        if (g_time_only) {
            std::printf("\rturn %u / %u", state.m_current_turn, turn_cap);
            std::fflush(stdout);
        } else if (g_ppm_every != 0 && (state.m_current_turn % g_ppm_every) == 0) {
            if (!save_turn_ppm(state, state.m_current_turn)) {
                std::printf("save turn %u failed\n", state.m_current_turn);
                SettlerTurnMng::clear();
                state.clear();
                return 1;
            }
        }
    }
    const auto t_loop1 = std::chrono::steady_clock::now();
    if (g_time_only) {
        std::printf("\n");
    }
    if (!save_turn_ppm(state, state.m_current_turn)) {
        std::printf("save final turn failed\n");
        SettlerTurnMng::clear();
        state.clear();
        return 1;
    }
    const u16 cities1 = count_cities(state);
    const u16 settlers = count_settlers(state);
    const bool ok = state.m_current_turn == turn_cap && cities1 > cities0;
    const double loop_ms = static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t_loop1 - t_loop0).count()) / 1.0e6;
    const double avg_ms = (turn_cap == 0) ? 0.0 : loop_ms / static_cast<double>(turn_cap);
    std::printf("=======================================================\n");
    std::printf(" SETTLER TURN MNG: %s after %u turns (players=%u cities %u -> %u settlers=%u)\n",
        ok ? "PASS" : "FAIL", state.m_current_turn, state.m_player_n, cities0, cities1, settlers);
    std::printf(" loop: %.3f ms total  %.3f ms/turn\n", loop_ms, avg_ms);
    std::printf(" maps: %s/turn_XXXX.ppm\n", G_OUT_DIR);
    std::printf(" hot-path timings:\n");
    tm_report(g_tm_begin);
    std::printf("=======================================================\n");
    SettlerTurnMng::clear();
    WhiteboardMng::terminate();
    state.clear();
    setup.release_map_gen();
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
