//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "game_state.h"
#include "map_bit_array_overlay.h"
#include "map_bit_overlay.h"
#include "map_terrain_validate.h"
#include "runtime_static_loader.h"
#include "unit_movement_mng.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-42/terrain.ppm";
static const char* G_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-42/climate.ppm";
static const char* G_RIV = "/home/w/Projects/simple-map-gen/p1-seed-42/rivers.ppm";
static const char* G_RES = "/home/w/Projects/simple-map-gen/p1-seed-42/overlay.ppm";
static const char* G_VIS_SCOUT_OUT = "/home/w/Projects/simple-map-gen/unit_movement_mng_land_scout_cost_visits.ppm";
static const char* G_VIS_SPEAR_OUT = "/home/w/Projects/simple-map-gen/unit_movement_mng_land_spearman_cost_visits.ppm";
static const char* G_VIS_EXPL_OUT = "/home/w/Projects/simple-map-gen/unit_movement_mng_land_explorer_cost_visits.ppm";
static const char* G_VIS_WATER_OUT = "/home/w/Projects/simple-map-gen/unit_movement_mng_water_cost_visits.ppm";

static const u8 k_vis_bpv = 16u;
static const u32 k_stk_cap = 1024u * 1024u;
static const u16 k_scout_typ = 0u;
static const u16 k_spearman_typ = 7u;
static const u16 k_explorer_typ = 11u;
static const u16 k_galley_typ = 10u;
static const u16 k_terr_dim = 16u;
static const u16 k_clim_dim = 8u;
static const u16 k_ov_dim = 8u;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

//================================================================================================================================
//=> - Table cells -
//================================================================================================================================

struct CostCell {
    i16 m_cost;
    u32 m_n;
};

struct RivRivCell {
    i16 m_cost;
    u32 m_n;
};

static CostCell g_tbl[k_terr_dim][k_clim_dim][k_ov_dim];
static RivRivCell g_riv_riv;
static u32 g_terr_vis[k_terr_dim];

struct FnTiming {
    u64 m_ns;
    u32 m_n;
};

static FnTiming g_t_place;
static FnTiming g_t_can_step;
static FnTiming g_t_apply_step;

static u32 g_tile_stk[k_stk_cap];
static u32 g_tile_stk_n = 0u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 tile_idx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static void tile_xy (u16 w, u32 idx, u16* x, u16* y) {
    *x = static_cast<u16>(idx % static_cast<u32>(w));
    *y = static_cast<u16>(idx / static_cast<u32>(w));
}

static void stk_reset () {
    g_tile_stk_n = 0u;
}

static void stk_push (u16 w, u16 x, u16 y) {
    if (g_tile_stk_n >= k_stk_cap) {
        return;
    }
    g_tile_stk[g_tile_stk_n] = tile_idx(w, x, y);
    g_tile_stk_n = static_cast<u32>(g_tile_stk_n + 1u);
}

static bool stk_pop (u16 w, u16* x, u16* y) {
    if (g_tile_stk_n == 0u) {
        return false;
    }
    g_tile_stk_n = static_cast<u32>(g_tile_stk_n - 1u);
    tile_xy(w, g_tile_stk[g_tile_stk_n], x, y);
    return true;
}

static bool stk_top (u16 w, u16* x, u16* y) {
    if (g_tile_stk_n == 0u) {
        return false;
    }
    tile_xy(w, g_tile_stk[g_tile_stk_n - 1u], x, y);
    return true;
}

//================================================================================================================================
//=> - Timing -
//================================================================================================================================

static void t_add (FnTiming& ft, u64 ns) {
    ft.m_ns = static_cast<u64>(ft.m_ns + ns);
    ft.m_n = static_cast<u32>(ft.m_n + 1u);
}

static void t_reset () {
    g_t_place.m_ns = 0u;
    g_t_place.m_n = 0u;
    g_t_can_step.m_ns = 0u;
    g_t_can_step.m_n = 0u;
    g_t_apply_step.m_ns = 0u;
    g_t_apply_step.m_n = 0u;
}

static u64 t_elapsed_ns (const std::chrono::high_resolution_clock::time_point& t0,
    const std::chrono::high_resolution_clock::time_point& t1) {
    return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
}

static bool can_step_timed (const GameState& s, UnitAddKey key, u16 dx, u16 dy, i16* out_cost) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    const bool ok = UnitMovementMng::can_step(s, key, dx, dy, out_cost);
    const auto t1 = std::chrono::high_resolution_clock::now();
    t_add(g_t_can_step, t_elapsed_ns(t0, t1));
    return ok;
}

static bool apply_step_timed (GameState& s, UnitAddKey key, u16 dx, u16 dy) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    const bool ok = UnitMovementMng::apply_step(s, key, dx, dy);
    const auto t1 = std::chrono::high_resolution_clock::now();
    t_add(g_t_apply_step, t_elapsed_ns(t0, t1));
    return ok;
}

static bool place_timed (GameState& s, u16 x, u16 y, u16 seat, u16 typ, UnitAddKey* out) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    const bool ok = UnitMovementMng::place_on_tile(s, x, y, seat, typ, out);
    const auto t1 = std::chrono::high_resolution_clock::now();
    t_add(g_t_place, t_elapsed_ns(t0, t1));
    return ok;
}

static void print_timing_line (cstr lbl, const FnTiming& ft) {
    if (ft.m_n == 0u) {
        return;
    }
    const u64 avg_ns = ft.m_ns / static_cast<u64>(ft.m_n);
    if (avg_ns < 1000u) {
        std::printf("  %s: %llu ns (n=%u)\n",
            lbl, static_cast<unsigned long long>(avg_ns), ft.m_n);
    } else {
        std::printf("  %s: %llu us (n=%u)\n",
            lbl, static_cast<unsigned long long>(avg_ns / 1000u), ft.m_n);
    }
}

static void print_timing () {
    std::printf("--- UnitMovementMng timing (avg) ---\n");
    print_timing_line("place_on_tile", g_t_place);
    print_timing_line("can_step", g_t_can_step);
    print_timing_line("apply_step", g_t_apply_step);
}

static void refill_mp (GameState& s, UnitAddKey key) {
    UnitAddStruct* u = s.m_units.get_unit_add(key);
    if (u == nullptr || s.m_statics == nullptr) {
        return;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    if (u->m_unit_typ_idx >= typ_n) {
        return;
    }
    const u16 pts = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    u->m_mvt_points = static_cast<i16>(pts * PATH_MP_TURN);
}

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

static bool is_water (u8 terr) {
    return terr == TERR_OCEAN[0] || terr == TERR_SEA[0] || terr == TERR_COASTAL[0];
}

static u16 ov_slot (u8 ov) {
    if (ov == 0u || ov == OVERLAY_NONE) {
        return 0u;
    }
    if (ov == OV_SWAMP[0]) {
        return 1u;
    }
    if (ov == OV_FOREST[0]) {
        return 2u;
    }
    if (ov == OV_JUNGLE[0]) {
        return 3u;
    }
    return 4u;
}

static cstr ov_label (u16 slot) {
    if (slot == 0u) {
        return "NONE";
    }
    if (slot == 1u) {
        return "SWAMPS";
    }
    if (slot == 2u) {
        return "FORESTS";
    }
    if (slot == 3u) {
        return "JUNGLES";
    }
    return "OTHER";
}

static cstr terr_label (u8 terr) {
    if (terr == TERR_NONE[0]) {
        return "TERR_NONE";
    }
    if (terr == TERR_OCEAN[0]) {
        return "TERR_OCEAN";
    }
    if (terr == TERR_SEA[0]) {
        return "TERR_SEA";
    }
    if (terr == TERR_COASTAL[0]) {
        return "TERR_COASTAL";
    }
    if (terr == TERR_PLAINS[0]) {
        return "TERR_PLAINS";
    }
    if (terr == TERR_HILLS[0]) {
        return "TERR_HILLS";
    }
    if (terr == TERR_MOUNTAINS[0]) {
        return "TERR_MOUNTAINS";
    }
    if (terr == TERR_INLAND_SEA[0]) {
        return "TERR_INLAND_SEA";
    }
    if (terr == TERR_INLAND_LAKE[0]) {
        return "TERR_INLAND_LAKE";
    }
    return "TERR_OTHER";
}

static const u8 k_game_terr[] = {
    TERR_NONE[0],
    TERR_OCEAN[0],
    TERR_SEA[0],
    TERR_COASTAL[0],
    TERR_PLAINS[0],
    TERR_HILLS[0],
    TERR_MOUNTAINS[0],
    TERR_INLAND_SEA[0],
    TERR_INLAND_LAKE[0],
};

static const u16 k_game_terr_n = 9u;

static bool init_state (GameState* state) {
    if (state == nullptr || !load_statics()) {
        return false;
    }
    state->clear();
    state->m_statics = g_rt_statics;
    if (!UnitMovementMng::setup_mvt_costs(*g_rt_statics)) {
        return false;
    }
    state->m_civ_relations.reset(g_rt_statics->civ().get_item_count());
    if (!Factory_GameArraySimple::load_map_gen_data(&state->m_map, G_TERR, G_CLIM, G_RIV)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&state->m_map, G_RES)) {
        return false;
    }
    const u16 w = state->m_map.width();
    const u16 h = state->m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    PlayerState* seat = new PlayerState();
    if (seat == nullptr) {
        return false;
    }
    seat->m_ai_controlled = 0;
    seat->m_is_active = 1;
    seat->m_civ_index = 0;
    seat->m_explored_overlay = new MapBitOverlay(w, h);
    seat->m_techs_researched = nullptr;
    if (seat->m_explored_overlay == nullptr) {
        delete seat;
        return false;
    }
    state->m_player_states = seat;
    state->m_player_n = 1;
    state->m_players_remaining = 1;
    return true;
}

static void map_center (const GameState& s, u16* out_x, u16* out_y) {
    *out_x = static_cast<u16>(s.m_map.width() / 2u);
    *out_y = static_cast<u16>(s.m_map.height() / 2u);
}

static void reset_counters () {
    t_reset();
    for (u16 ti = 0; ti < k_terr_dim; ++ti) {
        for (u16 ci = 0; ci < k_clim_dim; ++ci) {
            for (u16 oi = 0; oi < k_ov_dim; ++oi) {
                g_tbl[ti][ci][oi].m_cost = 0;
                g_tbl[ti][ci][oi].m_n = 0u;
            }
        }
    }
    g_riv_riv.m_cost = 0;
    g_riv_riv.m_n = 0u;
    for (u16 ti = 0; ti < k_terr_dim; ++ti) {
        g_terr_vis[ti] = 0u;
    }
}

static bool record_land (u8 terr, u8 clim, u8 ov, i16 cost) {
    if (is_water(terr) || terr >= k_terr_dim || clim >= k_clim_dim) {
        return true;
    }
    const u16 os = ov_slot(ov);
    if (os >= k_ov_dim) {
        return true;
    }
    CostCell& c = g_tbl[terr][clim][os];
    if (c.m_n > 0u && c.m_cost != cost) {
        std::printf("INCONSISTENT land cost terr=%u clim=%u ov=%u had=%d now=%d\n",
            terr, clim, ov, static_cast<int>(c.m_cost), static_cast<int>(cost));
        return false;
    }
    if (c.m_n == 0u) {
        c.m_cost = cost;
    }
    c.m_n = static_cast<u32>(c.m_n + 1u);
    return true;
}

static bool record_water (u8 terr, u8 clim, u8 ov, i16 cost) {
    if (!is_water(terr) || terr >= k_terr_dim || clim >= k_clim_dim) {
        return true;
    }
    const u16 os = ov_slot(ov);
    if (os >= k_ov_dim) {
        return true;
    }
    CostCell& c = g_tbl[terr][clim][os];
    if (c.m_n > 0u && c.m_cost != cost) {
        std::printf("INCONSISTENT water cost terr=%u clim=%u ov=%u had=%d now=%d\n",
            terr, clim, ov, static_cast<int>(c.m_cost), static_cast<int>(cost));
        return false;
    }
    if (c.m_n == 0u) {
        c.m_cost = cost;
    }
    c.m_n = static_cast<u32>(c.m_n + 1u);
    return true;
}

static bool record_riv_riv (i16 cost) {
    if (g_riv_riv.m_n > 0u && g_riv_riv.m_cost != cost) {
        std::printf("INCONSISTENT river-to-river cost had=%d now=%d\n",
            static_cast<int>(g_riv_riv.m_cost), static_cast<int>(cost));
        return false;
    }
    if (g_riv_riv.m_n == 0u) {
        g_riv_riv.m_cost = cost;
    }
    g_riv_riv.m_n = static_cast<u32>(g_riv_riv.m_n + 1u);
    return true;
}

static bool record_step (const GameState& s, u16 fx, u16 fy, u16 tx, u16 ty, i16 cost) {
    const u8 tterr = s.m_map.get_terrain(tx, ty);
    if (tterr < k_terr_dim) {
        g_terr_vis[tterr] = static_cast<u32>(g_terr_vis[tterr] + 1u);
    }
    const bool fr = s.m_map.get_river(fx, fy) != 0u;
    const bool tr = s.m_map.get_river(tx, ty) != 0u;
    if (fr && tr) {
        return record_riv_riv(cost);
    }
    return record_land(tterr, s.m_map.get_climate(tx, ty), s.m_map.get_overlay(tx, ty), cost);
}

static bool record_water_step (const GameState& s, u16 fx, u16 fy, u16 tx, u16 ty, i16 cost) {
    const u8 tterr = s.m_map.get_terrain(tx, ty);
    if (tterr < k_terr_dim) {
        g_terr_vis[tterr] = static_cast<u32>(g_terr_vis[tterr] + 1u);
    }
    const bool fr = s.m_map.get_river(fx, fy) != 0u;
    const bool tr = s.m_map.get_river(tx, ty) != 0u;
    if (fr && tr) {
        return record_riv_riv(cost);
    }
    return record_water(tterr, s.m_map.get_climate(tx, ty), s.m_map.get_overlay(tx, ty), cost);
}

static u32 terr_vis_total () {
    u32 sum = 0u;
    for (u16 ti = 0; ti < k_terr_dim; ++ti) {
        sum = static_cast<u32>(sum + g_terr_vis[ti]);
    }
    return sum;
}

static void print_terr_vis () {
    std::printf("--- terrain visit counts (destination terrain only) ---\n");
    for (u16 i = 0; i < k_game_terr_n; ++i) {
        const u8 tid = k_game_terr[i];
        const u32 n = (tid < k_terr_dim) ? g_terr_vis[tid] : 0u;
        std::printf("  terr=%u(%s) visits=%u\n", tid, terr_label(tid), n);
    }
    std::printf("  total terrain visits: %u\n", terr_vis_total());
}

static void vis_inc (MapBitArrayOverlay& vis, u16 x, u16 y) {
    const u32 v = vis.get(x, y);
    vis.set(x, y, static_cast<u32>(v + 1u));
}

static u32 vis_cnt_tiles (const MapBitArrayOverlay& vis) {
    u32 n = 0u;
    for (u16 y = 0; y < vis.height(); ++y) {
        for (u16 x = 0; x < vis.width(); ++x) {
            if (vis.get(x, y) > 0u) {
                n = static_cast<u32>(n + 1u);
            }
        }
    }
    return n;
}

static u32 vis_max (const MapBitArrayOverlay& vis) {
    u32 mx = 0u;
    for (u16 y = 0; y < vis.height(); ++y) {
        for (u16 x = 0; x < vis.width(); ++x) {
            const u32 v = vis.get(x, y);
            if (v > mx) {
                mx = v;
            }
        }
    }
    return mx;
}

static bool pick_step (
    GameState& s,
    UnitAddKey key,
    const MapBitArrayOverlay& vis,
    u16 ux,
    u16 uy,
    u16 base,
    u16* out_x,
    u16* out_y,
    i16* out_cost) {
    bool has_new = false;
    u16 nx_new = 0;
    u16 ny_new = 0;
    i16 cost_new = 0;
    u16 dir_new = MAP_NBR8_N;
    for (u32 t = 0; t < MAP_NBR8_N; ++t) {
        const u16 d = static_cast<u16>((base + t) % MAP_NBR8_N);
        const i32 nx_i = static_cast<i32>(ux) + MAP_NBR8_DX[d];
        const i32 ny_i = static_cast<i32>(uy) + MAP_NBR8_DY[d];
        if (nx_i < 0 || ny_i < 0) {
            continue;
        }
        const u16 nx = static_cast<u16>(nx_i);
        const u16 ny = static_cast<u16>(ny_i);
        if (nx >= s.m_map.width() || ny >= s.m_map.height()) {
            continue;
        }
        if (vis.get(nx, ny) != 0u) {
            continue;
        }
        i16 cost = 0;
        if (!can_step_timed(s, key, nx, ny, &cost)) {
            continue;
        }
        if (!has_new || d < dir_new) {
            has_new = true;
            nx_new = nx;
            ny_new = ny;
            cost_new = cost;
            dir_new = d;
        }
    }
    if (has_new) {
        *out_x = nx_new;
        *out_y = ny_new;
        *out_cost = cost_new;
        return true;
    }
    return false;
}

static bool note_mp_delta (
    bool* ok,
    i16 mp_before,
    i16 mp_after,
    i16 out_cost,
    u16 fx,
    u16 fy,
    u16 tx,
    u16 ty) {
    const i16 delta = static_cast<i16>(mp_before - mp_after);
    if (delta == out_cost) {
        return true;
    }
    *ok = false;
    if (print_level > 0) {
        std::printf("MP DELTA MISMATCH (%u,%u)->(%u,%u) out_cost=%d delta=%d mp_before=%d mp_after=%d\n",
            fx, fy, tx, ty, static_cast<int>(out_cost), static_cast<int>(delta),
            static_cast<int>(mp_before), static_cast<int>(mp_after));
    }
    return false;
}

static bool backtrack_stk (
    GameState& s,
    UnitAddKey key,
    const MapBitArrayOverlay& vis,
    u16 w,
    u16* ux,
    u16* uy,
    u16 base,
    u32* move_n,
    u32* back_n,
    bool* done,
    bool* cost_delta_ok) {
    while (true) {
        UnitAddStruct* u = s.m_units.get_unit_add(key);
        if (u == nullptr || u->m_mvt_points <= 0) {
            return false;
        }
        u16 nx = 0;
        u16 ny = 0;
        i16 cost = 0;
        if (pick_step(s, key, vis, *ux, *uy, base, &nx, &ny, &cost)) {
            return true;
        }
        u16 tx = 0;
        u16 ty = 0;
        while (stk_top(w, &tx, &ty) && *ux == tx && *uy == ty) {
            (void)stk_pop(w, &tx, &ty);
        }
        if (!stk_top(w, &tx, &ty)) {
            *done = true;
            return false;
        }
        cost = 0;
        if (!can_step_timed(s, key, tx, ty, &cost)) {
            *done = true;
            return false;
        }
        const i16 mp_before = u->m_mvt_points;
        const u16 fx = *ux;
        const u16 fy = *uy;
        if (!apply_step_timed(s, key, tx, ty)) {
            return false;
        }
        u = s.m_units.get_unit_add(key);
        if (u != nullptr) {
            (void)note_mp_delta(cost_delta_ok, mp_before, u->m_mvt_points, cost, fx, fy, tx, ty);
        }
        *ux = tx;
        *uy = ty;
        *move_n = static_cast<u32>(*move_n + 1u);
        *back_n = static_cast<u32>(*back_n + 1u);
    }
}

static bool save_vis_ppm (const GameState& s, const MapBitArrayOverlay& vis, cstr path) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 mx = vis_max(vis);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            const u32 v = vis.get(x, y);
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            if (v == 0u) {
                MapTerrainValidate::rgb_from_class(s.m_map.get_terrain(x, y), &r, &g, &b);
                r = static_cast<u8>(r / 4u);
                g = static_cast<u8>(g / 4u);
                b = static_cast<u8>(b / 4u);
            } else if (mx <= 1u) {
                r = 255;
                g = 255;
                b = 0;
            } else {
                const u32 num = static_cast<u32>((v - 1u) * 255u);
                const u32 den = static_cast<u32>(mx - 1u);
                r = static_cast<u8>(num / den);
                b = static_cast<u8>(255u - (num / den));
                g = static_cast<u8>(64u + (num / den) / 2u);
            }
            rgb[i * 3u + 0] = r;
            rgb[i * 3u + 1] = g;
            rgb[i * 3u + 2] = b;
        }
    }
    FILE* fp = std::fopen(path, "wb");
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

static void print_table (cstr cost_hdr) {
    print_terr_vis();
    std::printf("--- %s (terrain x climate x overlay) ---\n", cost_hdr);
    for (u16 ti = 0; ti < k_terr_dim; ++ti) {
        for (u16 ci = 0; ci < k_clim_dim; ++ci) {
            for (u16 oi = 0; oi < k_ov_dim; ++oi) {
                const CostCell& c = g_tbl[ti][ci][oi];
                if (c.m_n == 0u) {
                    continue;
                }
                std::printf("  terr=%u(%s) clim=%u ov=%s cost=%d visits=%u\n",
                    ti, terr_label(static_cast<u8>(ti)), ci, ov_label(oi),
                    static_cast<int>(c.m_cost), c.m_n);
            }
        }
    }
    std::printf("--- river-to-river movement ---\n");
    if (g_riv_riv.m_n > 0u) {
        std::printf("  cost=%d visits=%u\n", static_cast<int>(g_riv_riv.m_cost), g_riv_riv.m_n);
    } else {
        std::printf("  (no visits)\n");
    }
}

//================================================================================================================================
//=> - Test -
//================================================================================================================================

static void run_cost_walk (
    u16 sx,
    u16 sy,
    u16 unit_typ,
    cstr place_msg,
    cstr spawn_lbl,
    cstr cost_hdr,
    cstr vis_out,
    bool (*rec_fn) (const GameState&, u16, u16, u16, u16, i16)) {
    reset_counters();
    GameState state;
    bool ok = init_state(&state);
    note_result(ok, "init state and map");
    if (!ok) {
        return;
    }
    UnitAddKey key = UnitAddKey::None();
    ok = place_timed(state, sx, sy, 0, unit_typ, &key);
    note_result(ok, place_msg);
    if (!ok) {
        state.clear();
        return;
    }
    MapBitArrayOverlay vis(state.m_map.width(), state.m_map.height(), k_vis_bpv);
    if (vis.width() == 0) {
        state.clear();
        note_result(false, "visit overlay alloc");
        return;
    }
    vis_inc(vis, sx, sy);
    stk_reset();
    stk_push(state.m_map.width(), sx, sy);
    const u16 map_w = state.m_map.width();
    u16 ux = sx;
    u16 uy = sy;
    u32 turn_n = 0u;
    u32 move_n = 0u;
    u32 xpl_n = 0u;
    u32 back_n = 0u;
    bool consistent = true;
    bool cost_delta_ok = true;
    bool done = false;
    while (!done) {
        refill_mp(state, key);
        turn_n = static_cast<u32>(turn_n + 1u);
        while (true) {
            UnitAddStruct* u = state.m_units.get_unit_add(key);
            if (u == nullptr || u->m_mvt_points <= 0) {
                break;
            }
            const u16 base = static_cast<u16>(move_n % MAP_NBR8_N);
            u16 nx = 0;
            u16 ny = 0;
            i16 cost = 0;
            if (pick_step(state, key, vis, ux, uy, base, &nx, &ny, &cost)) {
                if (!rec_fn(state, ux, uy, nx, ny, cost)) {
                    consistent = false;
                    break;
                }
                const i16 mp_before = u->m_mvt_points;
                const u16 fx = ux;
                const u16 fy = uy;
                if (!apply_step_timed(state, key, nx, ny)) {
                    consistent = false;
                    break;
                }
                u = state.m_units.get_unit_add(key);
                if (u != nullptr) {
                    (void)note_mp_delta(&cost_delta_ok, mp_before, u->m_mvt_points, cost, fx, fy, nx, ny);
                }
                vis_inc(vis, nx, ny);
                ux = nx;
                uy = ny;
                stk_push(map_w, ux, uy);
                move_n = static_cast<u32>(move_n + 1u);
                xpl_n = static_cast<u32>(xpl_n + 1u);
                continue;
            }
            if (!backtrack_stk(state, key, vis, map_w, &ux, &uy, base, &move_n, &back_n, &done, &cost_delta_ok)) {
                break;
            }
        }
        if (!consistent || done) {
            break;
        }
    }
    note_result(consistent, "cost consistent per combo");
    note_result(cost_delta_ok, "out_cost matches mp budget delta");
    note_result(move_n > 0u, "walk moved at least once");
    note_result(terr_vis_total() == xpl_n, "terrain visit total matches explore moves");
    const bool saved = save_vis_ppm(state, vis, vis_out);
    note_result(saved, "save visit map ppm");
    if (print_level > 0) {
        UnitAddStruct* fu = state.m_units.get_unit_add(key);
        const i16 end_mp = (fu != nullptr) ? fu->m_mvt_points : 0;
        std::printf("spawn at %s (%u,%u) terr=%u(%s)\n",
            spawn_lbl, sx, sy, state.m_map.get_terrain(sx, sy), terr_label(state.m_map.get_terrain(sx, sy)));
        std::printf("turns: %u  moves: %u  explore: %u  backtrack: %u  done: %u  end mp: %d\n",
            turn_n, move_n, xpl_n, back_n, done ? 1u : 0u, static_cast<int>(end_mp));
        std::printf("tiles visited: %u (max visits on one tile: %u)\n", vis_cnt_tiles(vis), vis_max(vis));
        std::printf("visit map: %s\n", vis_out);
        print_timing();
        print_table(cost_hdr);
    }
    state.clear();
}

void test_land_cost_scout () {
    u16 sx = 0;
    u16 sy = 0;
    GameState tmp;
    if (init_state(&tmp)) {
        map_center(tmp, &sx, &sy);
        tmp.clear();
    }
    sx -= 100;
    run_cost_walk(sx, sy, k_scout_typ, "place scout", "map center",
        "land cost table (scout)", G_VIS_SCOUT_OUT, record_step);
}

void test_land_cost_spearman () {
    u16 sx = 0;
    u16 sy = 0;
    GameState tmp;
    if (init_state(&tmp)) {
        map_center(tmp, &sx, &sy);
        tmp.clear();
    }
    sx -= 100;
    run_cost_walk(sx, sy, k_spearman_typ, "place spearman", "map center",
        "land cost table (spearman)", G_VIS_SPEAR_OUT, record_step);
}

void test_land_cost_explorer () {
    u16 sx = 0;
    u16 sy = 0;
    GameState tmp;
    if (init_state(&tmp)) {
        map_center(tmp, &sx, &sy);
        tmp.clear();
    }
    sx -= 100;
    run_cost_walk(sx, sy, k_explorer_typ, "place explorer", "map center", "land cost table (explorer)", G_VIS_EXPL_OUT, record_step);
}

void test_water_cost_sail () {
    run_cost_walk(0u, 0u, k_galley_typ, "place sea unit", "0,0", "water cost table", G_VIS_WATER_OUT, record_water_step);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    test_land_cost_scout();
    std::printf("=======================================================\n");
    std::printf(" LAND SCOUT WALK: FAILURES SO FAR: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    test_land_cost_spearman();
    std::printf("=======================================================\n");
    std::printf(" LAND SPEARMAN WALK: FAILURES SO FAR: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    test_land_cost_explorer();
    std::printf("=======================================================\n");
    std::printf(" LAND EXPLORER WALK: FAILURES SO FAR: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    test_water_cost_sail();
    std::printf("=======================================================\n");
    std::printf(" MOVEMENT COST TESTER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
