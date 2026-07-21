//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "city_tile_manager.h"
#include "circular_tile_areas.h"
#include "city.h"
#include "city_array.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "tile_working.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-tile-manager-stable-food-test";
static const u32 G_SEED = 43u;
static const u16 G_POP_MAX = 15;
static const u16 G_BF_POP_MAX = 7;
static const u16 G_VIZ_POP = 10;
static const u64 G_BF_COMBO_CAP = 20000000ull;
static const u16 G_REACH_R = 4;
static const u16 G_EDGE_PAD = 4;
static const u16 G_CAND_MAX = 45;
static const u16 G_IMG = 201;
static const u16 G_IMG_R = 100;
static const u8 G_OV_A = 110;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];

//================================================================================================================================
//=> - Cand -
//================================================================================================================================

struct Cand {
    u16 m_x;
    u16 m_y;
    TileYield m_yld;
};

struct SpotCand {
    u16 m_x;
    u16 m_y;
    u32 m_score;
    u16 m_reach_n;
    u16 m_hill_n;
    u16 m_mnt_n;
};

enum SecMode : u8 {
    SEC_PROD = 0,
    SEC_COM = 1,
    SEC_BOTH = 2
};

//================================================================================================================================
//=> - Helpers -
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
    return true;
}

static bool ensure_out_dir () {
    return mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static bool is_city_terr (u8 terr) {
    return terr == TERR_PLAINS[0] || terr == TERR_HILLS[0];
}

static bool is_city_clim (u8 clim) {
    return clim == CLIMATE_GRASSLAND || clim == CLIMATE_PLAINS || clim == CLIMATE_BLACK_SOIL;
}

static bool in_reach (u16 cx, u16 cy, u16 x, u16 y) {
    const i32 dx = static_cast<i32>(x) - static_cast<i32>(cx);
    const i32 dy = static_cast<i32>(y) - static_cast<i32>(cy);
    return dx * dx + dy * dy < static_cast<i32>(G_REACH_R) * static_cast<i32>(G_REACH_R);
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
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

static void map_rgb (const GameArraySimple& map, u16 x, u16 y, u8* r, u8* g, u8* b) {
    terr_rgb(map.get_terrain(x, y), r, g, b);
    if (map.get_river(x, y) != 0) {
        *r = 0;
        *g = 180;
        *b = 255;
    }
}

static u8 blend_u8 (u8 base, u8 ov, u8 a) {
    const u16 ia = static_cast<u16>(255u - a);
    return static_cast<u8>((static_cast<u16>(base) * ia + static_cast<u16>(ov) * a) / 255u);
}

static bool wr_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
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

static u32 score_spot (u16 cx, u16 cy) {
    const CircArea area = CircularTileAreas::get(G_REACH_R);
    u32 score = 0;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (!TileYields::in_bounds(ux, uy)) {
            continue;
        }
        const TileYield yld = TileYields::get(ux, uy);
        score += static_cast<u32>(yld.m_food) + static_cast<u32>(yld.m_production) + static_cast<u32>(yld.m_commerce);
    }
    return score;
}

static bool reach_mix (const GameArraySimple& map, u16 cx, u16 cy, u16* reach_n, u16* hill_n, u16* mnt_n) {
    const CircArea area = CircularTileAreas::get(G_REACH_R);
    u16 rn = 0;
    u16 hn = 0;
    u16 mn = 0;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= map.width() || uy >= map.height()) {
            continue;
        }
        rn = static_cast<u16>(rn + 1u);
        const u8 terr = map.get_terrain(ux, uy);
        if (terr == TERR_HILLS[0]) {
            hn = static_cast<u16>(hn + 1u);
        } else if (terr == TERR_MOUNTAINS[0]) {
            mn = static_cast<u16>(mn + 1u);
        }
    }
    *reach_n = rn;
    *hill_n = hn;
    *mnt_n = mn;
    if (rn == 0) {
        return false;
    }
    const u32 hill_pct = (static_cast<u32>(hn) * 100u) / static_cast<u32>(rn);
    const u32 mnt_pct = (static_cast<u32>(mn) * 100u) / static_cast<u32>(rn);
    return mnt_pct >= 10u && hill_pct >= 40u && hill_pct <= 60u;
}

static bool cmp_spot (const SpotCand& a, const SpotCand& b) {
    return a.m_score > b.m_score;
}

static bool pick_spot (const GameArraySimple& map, SpotCand* out) {
    std::vector<SpotCand> spots;
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = G_EDGE_PAD; y + G_EDGE_PAD < h; ++y) {
        for (u16 x = G_EDGE_PAD; x + G_EDGE_PAD < w; ++x) {
            if (!is_city_terr(map.get_terrain(x, y)) || !is_city_clim(map.get_climate(x, y))) {
                continue;
            }
            u16 rn = 0;
            u16 hn = 0;
            u16 mn = 0;
            if (!reach_mix(map, x, y, &rn, &hn, &mn)) {
                continue;
            }
            spots.push_back({x, y, score_spot(x, y), rn, hn, mn});
        }
    }
    if (spots.empty()) {
        return false;
    }
    std::sort(spots.begin(), spots.end(), cmp_spot);
    *out = spots[0];
    return true;
}

static u16 gather_cands (u16 cx, u16 cy, Cand* out, u16 out_max) {
    const CircArea area = CircularTileAreas::get(G_REACH_R);
    u16 n = 0;
    for (u16 i = 0; i < area.m_lim && n < out_max; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (!TileYields::in_bounds(ux, uy)) {
            continue;
        }
        if (ux == cx && uy == cy) {
            continue;
        }
        const u16 wk = TileWorking::get_worker(ux, uy);
        if (wk != U16_KEY_NULL) {
            continue;
        }
        out[n].m_x = ux;
        out[n].m_y = uy;
        out[n].m_yld = TileYields::get(ux, uy);
        n = static_cast<u16>(n + 1u);
    }
    return n;
}

static u32 sec_score (const TotalTileYield& y, SecMode mode) {
    if (mode == SEC_PROD) {
        return y.m_production;
    }
    if (mode == SEC_COM) {
        return y.m_commerce;
    }
    return y.m_production + y.m_commerce;
}

static cstr mode_name (SecMode mode) {
    if (mode == SEC_PROD) {
        return "stable_food_max_production";
    }
    if (mode == SEC_COM) {
        return "stable_food_max_commerce";
    }
    return "stable_food_max_combined";
}

static cstr mode_file (SecMode mode) {
    if (mode == SEC_PROD) {
        return "production";
    }
    if (mode == SEC_COM) {
        return "commerce";
    }
    return "combined";
}

static u64 binom (u16 n, u16 k) {
    if (k > n) {
        return 0;
    }
    if (k > n - k) {
        k = static_cast<u16>(n - k);
    }
    u64 r = 1;
    for (u16 i = 1; i <= k; ++i) {
        r = r * static_cast<u64>(n - k + i) / static_cast<u64>(i);
    }
    return r;
}

struct BfBest {
    TotalTileYield m_yld;
    u8 m_ok;
    u8 m_have;
    u64 m_seen;
};

static bool bf_better (const TotalTileYield& y, u8 food_ok, const BfBest& best, SecMode mode) {
    if (best.m_have == 0) {
        return true;
    }
    if (food_ok != best.m_ok) {
        return food_ok > best.m_ok;
    }
    if (food_ok != 0) {
        const u32 s = sec_score(y, mode);
        const u32 bs = sec_score(best.m_yld, mode);
        if (s != bs) {
            return s > bs;
        }
        return y.m_food > best.m_yld.m_food;
    }
    if (y.m_food != best.m_yld.m_food) {
        return y.m_food > best.m_yld.m_food;
    }
    return sec_score(y, mode) > sec_score(best.m_yld, mode);
}

static void bf_rec (
    const Cand* cands,
    u16 n,
    u16 m,
    u16 start_food,
    u16 min_food,
    SecMode mode,
    u16 idx,
    u16 left,
    TotalTileYield cur,
    BfBest* best)
{
    (void)m;
    if (best->m_seen >= G_BF_COMBO_CAP) {
        return;
    }
    if (left == 0) {
        best->m_seen = best->m_seen + 1ull;
        const u8 food_ok = (start_food + cur.m_food >= min_food) ? 1u : 0u;
        if (bf_better(cur, food_ok, *best, mode)) {
            best->m_yld = cur;
            best->m_ok = food_ok;
            best->m_have = 1;
        }
        return;
    }
    if (idx >= n || static_cast<u16>(n - idx) < left) {
        return;
    }
    {
        TotalTileYield take = cur;
        take.m_food += cands[idx].m_yld.m_food;
        take.m_production += cands[idx].m_yld.m_production;
        take.m_commerce += cands[idx].m_yld.m_commerce;
        bf_rec(cands, n, m, start_food, min_food, mode, static_cast<u16>(idx + 1u),
            static_cast<u16>(left - 1u), take, best);
    }
    bf_rec(cands, n, m, start_food, min_food, mode, static_cast<u16>(idx + 1u), left, cur, best);
}

static BfBest brute_force (const Cand* cands, u16 n, u16 pop, u16 start_food, SecMode mode) {
    BfBest best = {};
    if (pop == 0 || n == 0) {
        return best;
    }
    u16 m = pop;
    if (m > n) {
        m = n;
    }
    const u16 min_food = static_cast<u16>(pop * 2u);
    TotalTileYield cur = {};
    bf_rec(cands, n, m, start_food, min_food, mode, 0, m, cur, &best);
    return best;
}

static TotalTileYield run_stable (SecMode mode, u16 player, u16 city_idx, u16 start_food) {
    if (mode == SEC_PROD) {
        return CityTileManager::stable_food_max_production(player, city_idx, start_food);
    }
    if (mode == SEC_COM) {
        return CityTileManager::stable_food_max_commerce(player, city_idx, start_food);
    }
    return CityTileManager::stable_food_max_combined(player, city_idx, start_food);
}

static void print_yld (cstr tag, u16 pop, u16 avail, u16 start_food, const TotalTileYield& y, u32 worked) {
    const u32 food_tot = start_food + y.m_food;
    const u16 need = static_cast<u16>(pop * 2u);
    std::printf("  %s pop=%u avail=%u start_food=%u need=%u food=%u food_tot=%u production=%u commerce=%u worked=%u stable=%s\n",
        tag,
        (unsigned)pop,
        (unsigned)avail,
        (unsigned)start_food,
        (unsigned)need,
        (unsigned)y.m_food,
        (unsigned)food_tot,
        (unsigned)y.m_production,
        (unsigned)y.m_commerce,
        (unsigned)worked,
        food_tot >= need ? "yes" : "no");
}

static void wr_overview_map (const GameArraySimple& map, u16 cx, u16 cy, cstr path) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
            map_rgb(map, x, y, &p[0], &p[1], &p[2]);
            if (in_reach(cx, cy, x, y)) {
                p[0] = blend_u8(p[0], 255, G_OV_A);
                p[1] = blend_u8(p[1], 220, G_OV_A);
                p[2] = blend_u8(p[2], 40, G_OV_A);
            }
        }
    }
    for (i32 dy = -2; dy <= 2; ++dy) {
        for (i32 dx = -2; dx <= 2; ++dx) {
            const i32 x = static_cast<i32>(cx) + dx;
            const i32 y = static_cast<i32>(cy) + dy;
            if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                continue;
            }
            u8* p = &rgb[(static_cast<size_t>(y) * w + static_cast<size_t>(x)) * 3u];
            p[0] = 255;
            p[1] = 0;
            p[2] = 255;
        }
    }
    wr_ppm(path, rgb.data(), w, h);
}

static void wr_sel_img (const GameArraySimple& map, u16 cx, u16 cy, u16 city_idx, cstr path) {
    u8 rgb[G_IMG * G_IMG * 3];
    for (u16 py = 0; py < G_IMG; ++py) {
        for (u16 px = 0; px < G_IMG; ++px) {
            const i32 mx = static_cast<i32>(cx) + static_cast<i32>(px) - static_cast<i32>(G_IMG_R);
            const i32 my = static_cast<i32>(cy) + static_cast<i32>(py) - static_cast<i32>(G_IMG_R);
            u8* p = &rgb[(static_cast<u32>(py) * G_IMG + static_cast<u32>(px)) * 3u];
            if (mx < 0 || my < 0 || mx >= static_cast<i32>(map.width()) || my >= static_cast<i32>(map.height())) {
                p[0] = 255;
                p[1] = 255;
                p[2] = 255;
                continue;
            }
            const u16 ux = static_cast<u16>(mx);
            const u16 uy = static_cast<u16>(my);
            map_rgb(map, ux, uy, &p[0], &p[1], &p[2]);
            if (in_reach(cx, cy, ux, uy)) {
                p[0] = blend_u8(p[0], 255, G_OV_A);
                p[1] = blend_u8(p[1], 220, G_OV_A);
                p[2] = blend_u8(p[2], 40, G_OV_A);
            }
            if (ux == cx && uy == cy) {
                p[0] = 255;
                p[1] = 0;
                p[2] = 255;
            } else if (in_reach(cx, cy, ux, uy) && TileWorking::get_worker(ux, uy) == city_idx) {
                p[0] = 255;
                p[1] = 0;
                p[2] = 0;
            }
        }
    }
    wr_ppm(path, rgb, G_IMG, G_IMG);
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("fail build paths\n");
        return 1;
    }
    if (!ensure_out_dir()) {
        std::printf("fail mkdir %s\n", G_OUT_DIR);
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("fail load runtime statics\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    if (!TileYields::setup(st)) {
        std::printf("fail setup tile yields\n");
        return 1;
    }
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, nullptr)) {
        std::printf("fail load map\n");
        return 1;
    }
    CityArray cities;
    if (!cities.bind_statics(st)) {
        std::printf("fail bind city array\n");
        return 1;
    }
    TileYields::bind_map(&map);
    TileWorking::bind_map(&map);
    CityTileManager::bind_cities(&cities);

    SpotCand spot = {};
    if (!pick_spot(map, &spot)) {
        std::printf("fail pick spot with >=10%% mountains and 40-60%% hills in reach\n");
        return 1;
    }
    const u16 spot_x = spot.m_x;
    const u16 spot_y = spot.m_y;
    const u32 hill_pct = (static_cast<u32>(spot.m_hill_n) * 100u) / static_cast<u32>(spot.m_reach_n);
    const u32 mnt_pct = (static_cast<u32>(spot.m_mnt_n) * 100u) / static_cast<u32>(spot.m_reach_n);
    const u16 player = 0;
    const u16 city_idx = cities.get_next_new_city_idx();
    City* city = cities.get_city(city_idx);
    if (city == nullptr) {
        std::printf("fail get city\n");
        return 1;
    }
    city->init(player, spot_x, spot_y);
    CityTileManager::clear(spot_x, spot_y, city_idx);

    Cand cands[G_CAND_MAX];
    const u16 avail = gather_cands(spot_x, spot_y, cands, G_CAND_MAX);
    const u16 start_food = TileYields::get(spot_x, spot_y).m_food;
    std::printf("spot=(%u,%u) reach=%u hills=%u(%u%%) mnt=%u(%u%%) avail=%u start_food=%u\n",
        (unsigned)spot_x, (unsigned)spot_y,
        (unsigned)spot.m_reach_n,
        (unsigned)spot.m_hill_n, (unsigned)hill_pct,
        (unsigned)spot.m_mnt_n, (unsigned)mnt_pct,
        (unsigned)avail, (unsigned)start_food);
    std::printf("out=%s\n", G_OUT_DIR);

    char path[512];
    std::snprintf(path, sizeof(path), "%s/overview_reach.ppm", G_OUT_DIR);
    wr_overview_map(map, spot_x, spot_y, path);
    std::printf("wrote %s\n", path);
    std::snprintf(path, sizeof(path), "%s/city_reach_close.ppm", G_OUT_DIR);
    wr_sel_img(map, spot_x, spot_y, city_idx, path);
    std::printf("wrote %s\n", path);

    const SecMode modes[3] = {SEC_PROD, SEC_COM, SEC_BOTH};
    for (u16 mi = 0; mi < 3; ++mi) {
        const SecMode mode = modes[mi];
        std::printf("\n== %s ==\n", mode_name(mode));
        for (u16 pop = 1; pop <= G_POP_MAX; ++pop) {
            city->set_population(pop);
            CityTileManager::clear(spot_x, spot_y, city_idx);
            const auto t0 = std::chrono::high_resolution_clock::now();
            const TotalTileYield y = run_stable(mode, player, city_idx, start_food);
            const auto t1 = std::chrono::high_resolution_clock::now();
            const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
            const u32 worked = CityTileManager::count_worked(spot_x, spot_y, city_idx);
            std::printf("  greedy us=%.2f", us);
            print_yld("", pop, avail, start_food, y, worked);
            if (pop == G_VIZ_POP) {
                std::snprintf(path, sizeof(path), "%s/%s_pop%u.ppm", G_OUT_DIR, mode_file(mode), (unsigned)pop);
                wr_sel_img(map, spot_x, spot_y, city_idx, path);
                std::printf("  wrote %s\n", path);
            }
        }
    }

    std::printf("\n== brute force N choose M (cap=%llu) ==\n", (unsigned long long)G_BF_COMBO_CAP);
    for (u16 pop = 1; pop <= G_BF_POP_MAX; ++pop) {
        const u16 m = (pop <= avail) ? pop : avail;
        const u64 est = binom(avail, m);
        std::printf("\npop=%u N=%u M=%u est_combos=%llu\n",
            (unsigned)pop, (unsigned)avail, (unsigned)m, (unsigned long long)est);
        if (est == 0 || est > G_BF_COMBO_CAP) {
            std::printf("  skip brute (over cap or empty)\n");
            continue;
        }
        city->set_population(pop);
        for (u16 mi = 0; mi < 3; ++mi) {
            const SecMode mode = modes[mi];
            CityTileManager::clear(spot_x, spot_y, city_idx);
            const TotalTileYield gy = run_stable(mode, player, city_idx, start_food);
            const auto t0 = std::chrono::high_resolution_clock::now();
            const BfBest bf = brute_force(cands, avail, pop, start_food, mode);
            const auto t1 = std::chrono::high_resolution_clock::now();
            const f64 ms = std::chrono::duration<f64, std::milli>(t1 - t0).count();
            const u32 g_sec = sec_score(gy, mode);
            const u32 b_sec = sec_score(bf.m_yld, mode);
            const u8 match = (gy.m_food == bf.m_yld.m_food
                && gy.m_production == bf.m_yld.m_production
                && gy.m_commerce == bf.m_yld.m_commerce) ? 1u : 0u;
            const u8 sec_ok = (g_sec == b_sec) ? 1u : 0u;
            std::printf("  %s bf_ms=%.2f seen=%llu greedy_sec=%u bf_sec=%u match_yld=%u match_sec=%u\n",
                mode_name(mode),
                ms,
                (unsigned long long)bf.m_seen,
                (unsigned)g_sec,
                (unsigned)b_sec,
                (unsigned)match,
                (unsigned)sec_ok);
            std::printf("    greedy food=%u production=%u commerce=%u\n",
                (unsigned)gy.m_food, (unsigned)gy.m_production, (unsigned)gy.m_commerce);
            std::printf("    brute  food=%u production=%u commerce=%u food_ok=%u\n",
                (unsigned)bf.m_yld.m_food, (unsigned)bf.m_yld.m_production, (unsigned)bf.m_yld.m_commerce,
                (unsigned)bf.m_ok);
        }
    }

    TileYields::bind_map(nullptr);
    TileWorking::bind_map(nullptr);
    CityTileManager::bind_cities(nullptr);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
