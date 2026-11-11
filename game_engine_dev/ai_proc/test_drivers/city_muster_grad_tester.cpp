//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_muster_grad.h"
#include "draw_map_tile_ownership.h"
#include "eval_paths.h"
#include "generate_distance_p2p.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"
#include "unit_movement_mng.h"
#include "walk_p2p.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_OUT = "/home/w/Projects/simple-map-gen/city-muster-grad";
static const char* G_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_DATA = "../../";

//================================================================================================================================
//=> - Timing -
//================================================================================================================================

static const u16 k_time_cap = 512u;

struct GradTime {
    u32 m_turn; // Save turn timed
    u64 m_ns; // CityMusterGrad bind+set_caps+build duration
};

static cstr time_unit (u8 u) {
    if (u == 0u) {
        return "s";
    }
    if (u == 1u) {
        return "ms";
    }
    if (u == 2u) {
        return "us";
    }
    return "ns";
}

static double time_div (u8 u) {
    if (u == 0u) {
        return 1000000000.0;
    }
    if (u == 1u) {
        return 1000000.0;
    }
    if (u == 2u) {
        return 1000.0;
    }
    return 1.0;
}

static u8 pick_time_unit (double ns) {
    for (u8 u = 0u; u < 4u; ++u) {
        if ((ns / time_div(u)) > 1.0) {
            return u;
        }
    }
    return 3u;
}

static void print_times (const GradTime* times, u16 n) {
    if (times == nullptr || n == 0u) {
        return;
    }
    std::printf("city_muster_grad times n=%u\n", static_cast<unsigned>(n));
    for (u16 i = 0; i < n; ++i) {
        const double ns = static_cast<double>(times[i].m_ns);
        const u8 u = pick_time_unit(ns);
        std::printf("  turn=%u  %.2f %s\n",
            static_cast<unsigned>(times[i].m_turn),
            ns / time_div(u),
            time_unit(u));
    }
}

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool mkdir_p (cstr path) {
    return path != nullptr && (::mkdir(path, 0755) == 0 || errno == EEXIST);
}

static void set_px (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i] = r;
    rgb[i + 1u] = g;
    rgb[i + 2u] = b;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0;
    *g = 0;
    *b = 0;
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_INLAND_SEA[0]) {
        *r = TERR_INLAND_SEA[1]; *g = TERR_INLAND_SEA[2]; *b = TERR_INLAND_SEA[3];
    } else if (cls == TERR_INLAND_LAKE[0]) {
        *r = TERR_INLAND_LAKE[1]; *g = TERR_INLAND_LAKE[2]; *b = TERR_INLAND_LAKE[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

static bool is_water_base (u8 terr) {
    return terr == TERR_OCEAN[0] || terr == TERR_SEA[0] || terr == TERR_COASTAL[0]
        || terr == TERR_INLAND_SEA[0] || terr == TERR_INLAND_LAKE[0];
}

static void bleach (u8* r, u8* g, u8* b) {
    const u16 gray = (static_cast<u16>(*r) + static_cast<u16>(*g) + static_cast<u16>(*b)) / 3u;
    *r = static_cast<u8>((gray + 255u * 2u) / 3u);
    *g = static_cast<u8>((gray + 255u * 2u) / 3u);
    *b = static_cast<u8>((gray + 255u * 2u) / 3u);
}

static void paint_base (u8* rgb, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            const u8 terr = map.get_terrain(x, y);
            if (is_water_base(terr)) {
                terr_rgb(terr, &r, &g, &b);
            } else {
                climate_to_rgb(map.get_climate(x, y), &r, &g, &b);
            }
            if (map.get_river(x, y) != 0u) {
                r = 40;
                g = 100;
                b = 220;
            }
            bleach(&r, &g, &b);
            set_px(rgb, w, h, x, y, r, g, b);
        }
    }
    const u8 a = 230u;
    const u8 ia = static_cast<u8>(255u - a);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_terrain(x, y) != TERR_MOUNTAINS[0]) {
                continue;
            }
            const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
            rgb[i] = static_cast<u8>((static_cast<u16>(42u) * a + static_cast<u16>(rgb[i]) * ia) / 255u);
            rgb[i + 1u] = static_cast<u8>((static_cast<u16>(24u) * a + static_cast<u16>(rgb[i + 1u]) * ia) / 255u);
            rgb[i + 2u] = static_cast<u8>((static_cast<u16>(12u) * a + static_cast<u16>(rgb[i + 2u]) * ia) / 255u);
        }
    }
}

static void put_mark (u8* rgb, u16 w, u16 h, u16 cx, u16 cy, u8 r, u8 g, u8 b) {
    const i32 x0 = static_cast<i32>(cx);
    const i32 y0 = static_cast<i32>(cy);
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 t = -1; t <= 1; ++t) {
            const i32 x = x0 + dx;
            const i32 y = y0 + t;
            if (x >= 0 && y >= 0 && x < static_cast<i32>(w) && y < static_cast<i32>(h)) {
                set_px(rgb, w, h, static_cast<u16>(x), static_cast<u16>(y), r, g, b);
            }
        }
    }
    for (i32 dy = -2; dy <= 2; ++dy) {
        for (i32 t = -1; t <= 1; ++t) {
            const i32 x = x0 + t;
            const i32 y = y0 + dy;
            if (x >= 0 && y >= 0 && x < static_cast<i32>(w) && y < static_cast<i32>(h)) {
                set_px(rgb, w, h, static_cast<u16>(x), static_cast<u16>(y), r, g, b);
            }
        }
    }
}

static bool save_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, n, fp) == n;
    std::fclose(fp);
    return ok;
}

static bool wr_own_ppm (cstr path, GameState& st) {
    const u16 w = st.m_map.width();
    const u16 h = st.m_map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    u8* seats = new u8[n];
    if (rgb == nullptr || seats == nullptr) {
        delete[] rgb;
        delete[] seats;
        return false;
    }
    paint_base(rgb, st.m_map);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            seats[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)] = st.m_map.get_civ_owner(x, y);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            DrawMapTileOwnership::blend(rgb, w, h, x, y, seats);
        }
    }
    if (st.m_player_states != nullptr) {
        for (u16 s = 0; s < st.m_player_n; ++s) {
            const u16 ci = st.m_player_states[s].m_muster_city;
            if (ci == U16_KEY_NULL) {
                continue;
            }
            const City* c = st.m_cities.get_city(ci);
            if (c == nullptr) {
                continue;
            }
            put_mark(rgb, w, h, c->get_x(), c->get_y(), 255u, 0u, 0u);
        }
    }
    const bool ok = save_ppm(path, rgb, w, h);
    delete[] seats;
    delete[] rgb;
    return ok;
}

static bool wr_grad_ppm (cstr path, GameState& st, const CityMusterGrad& cm) {
    const u16 w = st.m_map.width();
    const u16 h = st.m_map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    u16* smax = (st.m_player_n > 0u) ? new u16[st.m_player_n] : nullptr;
    if (rgb == nullptr || (st.m_player_n > 0u && smax == nullptr)) {
        delete[] rgb;
        delete[] smax;
        return false;
    }
    paint_base(rgb, st.m_map);
    for (u16 s = 0; s < st.m_player_n; ++s) {
        smax[s] = 0u;
    }
    const u16* turn = cm.board().turn();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (turn[i] == GenerateDistanceP2P::k_turn_sent) {
                continue;
            }
            const u8 own = st.m_map.get_civ_owner(x, y);
            if (own == U8_KEY_NULL || static_cast<u16>(own) >= st.m_player_n) {
                continue;
            }
            if (turn[i] > smax[own]) {
                smax[own] = turn[i];
            }
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (turn[i] == GenerateDistanceP2P::k_turn_sent) {
                continue;
            }
            const u8 own = st.m_map.get_civ_owner(x, y);
            if (own == U8_KEY_NULL || static_cast<u16>(own) >= st.m_player_n) {
                continue;
            }
            u8 col[3];
            DrawMapTileOwnership::rgb(static_cast<u16>(own), 0u, 0u, col);
            const u32 span = (smax[own] == 0u) ? 1u : static_cast<u32>(smax[own]);
            const u32 keep = span - static_cast<u32>(turn[i]);
            const u32 p = i * 3u;
            rgb[p] = static_cast<u8>((static_cast<u32>(col[0]) * keep) / span);
            rgb[p + 1u] = static_cast<u8>((static_cast<u32>(col[1]) * keep) / span);
            rgb[p + 2u] = static_cast<u8>((static_cast<u32>(col[2]) * keep) / span);
        }
    }
    if (st.m_player_states != nullptr) {
        for (u16 s = 0; s < st.m_player_n; ++s) {
            const u16 ci = st.m_player_states[s].m_muster_city;
            if (ci == U16_KEY_NULL) {
                continue;
            }
            const City* c = st.m_cities.get_city(ci);
            if (c == nullptr) {
                continue;
            }
            put_mark(rgb, w, h, c->get_x(), c->get_y(), 255u, 0u, 0u);
        }
    }
    const bool ok = save_ppm(path, rgb, w, h);
    delete[] smax;
    delete[] rgb;
    return ok;
}

static bool run_turn (
    GameState& st, RuntimeStatics& rst, const EvalPaths& paths, u32 turn, GradTime* out_t, bool* out_timed) {
    if (out_timed != nullptr) {
        *out_timed = false;
    }
    char map_p[512];
    char cities_p[512];
    char players_p[512];
    if (!paths.map_path(turn, map_p, sizeof(map_p))
        || !paths.cities_path(turn, cities_p, sizeof(cities_p))
        || !paths.players_path(turn, players_p, sizeof(players_p))) {
        return false;
    }
    if (!GameIo::load_map_tiles(map_p, st.m_map)) {
        std::printf("city_muster_grad: map load fail turn=%u %s\n", turn, map_p);
        return false;
    }
    if (!GameIo::load_cities(cities_p, st.m_cities)) {
        std::printf("city_muster_grad: cities load fail turn=%u %s\n", turn, cities_p);
        return false;
    }
    if (!GameIo::load_players(players_p, st)) {
        std::printf("city_muster_grad: players load fail turn=%u %s\n", turn, players_p);
        return false;
    }
    st.m_statics = &rst;
    st.m_current_turn = turn;
    UnitMovementMng::bind_state(&st);
    const u16 w = st.m_map.width();
    const u16 h = st.m_map.height();
    WhiteboardMng::init(w, h);
    bool ok = false;
    {
        CityMusterGrad cm;
        const auto t0 = std::chrono::steady_clock::now();
        ok = cm.bind(&st) && cm.set_caps() && cm.build();
        const auto t1 = std::chrono::steady_clock::now();
        if (out_t != nullptr) {
            out_t->m_turn = turn;
            out_t->m_ns = static_cast<u64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        }
        if (out_timed != nullptr) {
            *out_timed = true;
        }
        if (!ok) {
            std::printf("city_muster_grad: build fail turn=%u\n", turn);
        } else {
            char own_p[640];
            char grad_p[640];
            if (std::snprintf(own_p, sizeof(own_p), "%s/turn_%04u_own.ppm", G_OUT, turn) <= 0
                || std::snprintf(grad_p, sizeof(grad_p), "%s/turn_%04u_grad.ppm", G_OUT, turn) <= 0) {
                ok = false;
            } else {
                ok = wr_own_ppm(own_p, st) && wr_grad_ppm(grad_p, st, cm);
                if (!ok) {
                    std::printf("city_muster_grad: write fail turn=%u\n", turn);
                }
            }
        }
    }
    WhiteboardMng::terminate();
    if (!ok) {
        return false;
    }
    std::printf("city_muster_grad turn=%u ok\n", turn);
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!mkdir_p(G_OUT)) {
        std::printf("city_muster_grad: cannot mkdir %s\n", G_OUT);
        return 1;
    }
    EvalPaths paths;
    if (!paths.load(G_PATHS) || !paths.scan_saves() || paths.save_turn_n() == 0u) {
        std::printf("city_muster_grad: no saves (paths/seed/players)\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_LIB, G_DATA)) {
        std::printf("city_muster_grad: statics load fail\n");
        return 1;
    }
    RuntimeStatics& rst = loader.statics();
    if (!TechAgeMng::setup(rst)) {
        std::printf("city_muster_grad: TechAgeMng setup fail\n");
        return 1;
    }
    if (!UnitMovementMng::setup_mvt_costs(rst)) {
        std::printf("city_muster_grad: mvt setup fail\n");
        TechAgeMng::clear();
        return 1;
    }
    GameState st;
    st.m_statics = &rst;
    if (!st.m_cities.bind_statics(rst)) {
        std::printf("city_muster_grad: cities bind fail\n");
        TechAgeMng::clear();
        return 1;
    }
    GradTime times[k_time_cap];
    u16 time_n = 0u;
    u16 ok_n = 0u;
    for (u16 i = 0; i < paths.save_turn_n(); ++i) {
        GradTime slot;
        bool timed = false;
        if (run_turn(st, rst, paths, paths.save_turn_at(i), &slot, &timed)) {
            ok_n = static_cast<u16>(ok_n + 1u);
        }
        if (timed && time_n < k_time_cap) {
            times[time_n] = slot;
            time_n = static_cast<u16>(time_n + 1u);
        }
    }
    TechAgeMng::clear();
    print_times(times, time_n);
    std::printf("city_muster_grad done ok=%u / %u -> %s\n",
        static_cast<unsigned>(ok_n),
        static_cast<unsigned>(paths.save_turn_n()),
        G_OUT);
    return (ok_n > 0u) ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
