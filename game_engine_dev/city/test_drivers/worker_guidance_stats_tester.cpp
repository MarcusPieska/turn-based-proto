//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "bit_array.h"
#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "city_tile_manager.h"
#include "circular_tile_areas.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_working.h"
#include "tile_yields.h"
#include "tile_usage.h"
#include "tile_work_assessor.h"
#include "worker_guidance.h"
#include "worker_job_static_key.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u32 G_CITY_N = 2500u;
static const u16 G_POP_LO = 5u;
static const u16 G_POP_HI = 20u;
static const u16 G_CLAIM_CULT = 150u;
static const u16 G_EDGE_PAD = 6u;
static const u16 G_MIN_REACH = 12u;
static const u16 G_MIN_SEP = 7u;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
static char g_out_dir[320];

struct StatRow {
    std::string m_job;
    std::string m_geo;
    std::string m_geo_clr;
    std::string m_intent;
    u8 m_clim;
    u8 m_terr;
    u32 m_n;
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool build_paths () {
    if (std::snprintf(g_out_dir, sizeof(g_out_dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", g_out_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", g_out_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", g_out_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", g_out_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", g_out_dir) <= 0) {
        return false;
    }
    return true;
}

static bool land_terr (u8 t) {
    return t == TERR_PLAINS[0] || t == TERR_HILLS[0] || t == TERR_MOUNTAINS[0];
}

static cstr clim_nm (u8 id) {
    if (id == CLIMATE_NONE) {
        return "none";
    }
    if (id == CLIMATE_PLAINS) {
        return "plains";
    }
    if (id == CLIMATE_DESERT) {
        return "desert";
    }
    if (id == CLIMATE_GRASSLAND) {
        return "grassland";
    }
    if (id == CLIMATE_BLACK_SOIL) {
        return "black_soil";
    }
    return "unknown_clim";
}

static cstr terr_nm (u8 id) {
    if (id == TERR_PLAINS[0]) {
        return "plains";
    }
    if (id == TERR_HILLS[0]) {
        return "hills";
    }
    if (id == TERR_MOUNTAINS[0]) {
        return "mountains";
    }
    if (id == TERR_VOLCANO[0]) {
        return "volcano";
    }
    if (id == TERR_COASTAL[0]) {
        return "coastal";
    }
    return "unknown_terr";
}

static cstr ov_nm (u8 id) {
    if (id == OV_NONE[0]) {
        return nullptr;
    }
    if (id == OV_FOREST[0]) {
        return "forest";
    }
    if (id == OV_JUNGLE[0]) {
        return "jungle";
    }
    if (id == OV_SWAMP[0]) {
        return "swamp";
    }
    if (id == OV_GLACIER[0]) {
        return "glacier";
    }
    return "unknown_ov";
}

static cstr intent_nm (u8 u) {
    if (u == TILE_ASSIGN_FOOD) {
        return "food";
    }
    if (u == TILE_ASSIGN_PROD) {
        return "prod";
    }
    return "unknown";
}

static bool is_exc_nm (cstr nm) {
    if (nm == nullptr) {
        return true;
    }
    if (std::strcmp(nm, "none") == 0) {
        return true;
    }
    if (std::strncmp(nm, "unknown", 7) == 0) {
        return true;
    }
    return false;
}

static void force_red_if_exc (cstr nm, u8* r, u8* g, u8* b) {
    if (is_exc_nm(nm)) {
        *r = 255;
        *g = 0;
        *b = 0;
    }
}

static const char* K_CLR_RST = "\033[0m";
static const u8 K_RIV_R = 40u;
static const u8 K_RIV_G = 100u;
static const u8 K_RIV_B = 220u;
static const u8 K_MTN_R = 120u;
static const u8 K_MTN_G = 72u;
static const u8 K_MTN_B = 40u;
static const u8 K_IMP_R = 220u;
static const u8 K_IMP_G = 30u;
static const u8 K_IMP_B = 30u;

static void ansi_rgb (char* buf, size_t cap, u8 r, u8 g, u8 b) {
    std::snprintf(buf, cap, "\033[38;2;%u;%u;%um", (unsigned)r, (unsigned)g, (unsigned)b);
}

static void terr_rgb (u8 terr, u8* r, u8* g, u8* b) {
    static const u8* const k_rows[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS, TERR_MOUNTAINS, TERR_VOLCANO,
        TERR_INLAND_SEA, TERR_INLAND_LAKE, TERR_TILE_SENTINEL
    };
    for (u32 i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == terr) {
            *r = k_rows[i][1];
            *g = k_rows[i][2];
            *b = k_rows[i][3];
            return;
        }
    }
    *r = 180;
    *g = 180;
    *b = 180;
}

static void ov_rgb (u8 ov, u8* r, u8* g, u8* b) {
    static const u8* const k_rows[] = {OV_NONE, OV_FOREST, OV_SWAMP, OV_JUNGLE, OV_GLACIER};
    for (u32 i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == ov) {
            *r = k_rows[i][1];
            *g = k_rows[i][2];
            *b = k_rows[i][3];
            return;
        }
    }
    *r = 180;
    *g = 180;
    *b = 180;
}

static void geo_parts (
    const GameArraySimple& map,
    u16 x,
    u16 y,
    char* plain,
    size_t pcap,
    char* colored,
    size_t ccap,
    u8* out_clim,
    u8* out_terr)
{
    const u8 clim_id = map.get_climate(x, y);
    const u8 terr_id = map.get_terrain(x, y);
    const u8 ov_id = map.get_overlay(x, y);
    *out_clim = clim_id;
    *out_terr = terr_id;
    const cstr clim = clim_nm(clim_id);
    const cstr terr = terr_nm(terr_id);
    const cstr ov = ov_nm(ov_id);
    const u8 has_riv = map.get_river(x, y) != 0 ? 1u : 0u;
    u8 cr = 0;
    u8 cg = 0;
    u8 cb = 0;
    u8 tr = 0;
    u8 tg = 0;
    u8 tb = 0;
    climate_to_rgb(clim_id, &cr, &cg, &cb);
    terr_rgb(terr_id, &tr, &tg, &tb);
    force_red_if_exc(clim, &cr, &cg, &cb);
    force_red_if_exc(terr, &tr, &tg, &tb);
    char c_clim[32];
    char c_terr[32];
    char c_ov[32];
    char c_riv[32];
    ansi_rgb(c_clim, sizeof(c_clim), cr, cg, cb);
    ansi_rgb(c_terr, sizeof(c_terr), tr, tg, tb);
    if (ov != nullptr) {
        u8 orr = 0;
        u8 og = 0;
        u8 ob = 0;
        ov_rgb(ov_id, &orr, &og, &ob);
        force_red_if_exc(ov, &orr, &og, &ob);
        ansi_rgb(c_ov, sizeof(c_ov), orr, og, ob);
        if (has_riv != 0) {
            ansi_rgb(c_riv, sizeof(c_riv), K_RIV_R, K_RIV_G, K_RIV_B);
            std::snprintf(plain, pcap, "%s %s %s with river", clim, terr, ov);
            std::snprintf(colored, ccap, "%s%s%s %s%s%s %s%s%s %swith river%s",
                c_clim, clim, K_CLR_RST, c_terr, terr, K_CLR_RST, c_ov, ov, K_CLR_RST, c_riv, K_CLR_RST);
        } else {
            std::snprintf(plain, pcap, "%s %s %s", clim, terr, ov);
            std::snprintf(colored, ccap, "%s%s%s %s%s%s %s%s%s",
                c_clim, clim, K_CLR_RST, c_terr, terr, K_CLR_RST, c_ov, ov, K_CLR_RST);
        }
    } else if (has_riv != 0) {
        ansi_rgb(c_riv, sizeof(c_riv), K_RIV_R, K_RIV_G, K_RIV_B);
        std::snprintf(plain, pcap, "%s %s with river", clim, terr);
        std::snprintf(colored, ccap, "%s%s%s %s%s%s %swith river%s",
            c_clim, clim, K_CLR_RST, c_terr, terr, K_CLR_RST, c_riv, K_CLR_RST);
    } else {
        std::snprintf(plain, pcap, "%s %s", clim, terr);
        std::snprintf(colored, ccap, "%s%s%s %s%s%s",
            c_clim, clim, K_CLR_RST, c_terr, terr, K_CLR_RST);
    }
}

static u16 count_reach_land (const GameArraySimple& map, u16 cx, u16 cy) {
    const CircArea area = CircularTileAreas::get(4);
    u16 n = 0;
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
        if (ux == cx && uy == cy) {
            continue;
        }
        if (land_terr(map.get_terrain(ux, uy))) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

static bool pick_locs (const GameArraySimple& map, std::mt19937* rng, u16* ox, u16* oy, u32 n) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 got = 0;
    u32 tries = 0;
    while (got < n && tries < 8000000u) {
        ++tries;
        const u16 x = static_cast<u16>((*rng)() % static_cast<u32>(w - 2u * G_EDGE_PAD)) + G_EDGE_PAD;
        const u16 y = static_cast<u16>((*rng)() % static_cast<u32>(h - 2u * G_EDGE_PAD)) + G_EDGE_PAD;
        if (!land_terr(map.get_terrain(x, y))) {
            continue;
        }
        if (count_reach_land(map, x, y) < G_MIN_REACH) {
            continue;
        }
        u8 bad = 0;
        for (u32 i = 0; i < got; ++i) {
            const i32 dx = static_cast<i32>(ox[i]) - static_cast<i32>(x);
            const i32 dy = static_cast<i32>(oy[i]) - static_cast<i32>(y);
            const i32 adx = dx < 0 ? -dx : dx;
            const i32 ady = dy < 0 ? -dy : dy;
            const i32 cheb = adx > ady ? adx : ady;
            if (cheb < static_cast<i32>(G_MIN_SEP)) {
                bad = 1;
                break;
            }
        }
        if (bad != 0) {
            continue;
        }
        ox[got] = x;
        oy[got] = y;
        got = got + 1u;
    }
    return got == n;
}

static void bump_stat (std::vector<StatRow>& rows, cstr job, cstr geo, cstr geo_clr, cstr intent, u8 clim, u8 terr) {
    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].m_job == job && rows[i].m_geo == geo && rows[i].m_intent == intent) {
            rows[i].m_n = rows[i].m_n + 1u;
            return;
        }
    }
    StatRow r;
    r.m_job = job;
    r.m_geo = geo;
    r.m_geo_clr = geo_clr;
    r.m_intent = intent;
    r.m_clim = clim;
    r.m_terr = terr;
    r.m_n = 1u;
    rows.push_back(r);
}

static void mark_job (std::vector<std::vector<u8>>& marks, u16 job, u16 w, u16 x, u16 y) {
    if (job >= marks.size() || marks[job].empty()) {
        return;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    if (i < marks[job].size()) {
        marks[job][i] = 1u;
    }
}

static void ensure_marks (std::vector<std::vector<u8>>& marks, u16 job_n, u32 tile_n) {
    marks.assign(job_n, std::vector<u8>());
    for (u16 j = 0; j < job_n; ++j) {
        marks[j].assign(tile_n, 0u);
    }
}

static void job_file_nm (cstr job, char* out, size_t cap) {
    size_t o = 0;
    for (size_t i = 0; job[i] != '\0' && o + 1u < cap; ++i) {
        const char c = job[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            out[o++] = c;
        } else {
            out[o++] = '_';
        }
    }
    out[o] = '\0';
}

static bool write_imp_ppm (cstr path, const GameArraySimple& map, const std::vector<u8>& mark) {
    FILE* f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(map.get_climate(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0u) {
                r = K_RIV_R;
                g = K_RIV_G;
                b = K_RIV_B;
            }
            if (map.get_terrain(x, y) == TERR_MOUNTAINS[0]) {
                r = K_MTN_R;
                g = K_MTN_G;
                b = K_MTN_B;
            }
            const u32 ti = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (ti < mark.size() && mark[ti] != 0u) {
                r = K_IMP_R;
                g = K_IMP_G;
                b = K_IMP_B;
            }
            const u32 i = ti * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    const size_t nbytes = rgb.size();
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, f) == nbytes;
    std::fclose(f);
    return ok;
}

static void write_imp_maps (
    const GameArraySimple& map,
    const RuntimeStatics& st,
    const std::vector<std::vector<u8>>& marks,
    cstr file_pfx)
{
    const u16 jn = st.worker_job().get_item_count();
    for (u16 j = 0; j < jn && j < marks.size(); ++j) {
        u32 n = 0;
        for (size_t i = 0; i < marks[j].size(); ++i) {
            n = n + (marks[j][i] != 0u ? 1u : 0u);
        }
        if (n == 0u) {
            continue;
        }
        cstr jnm = st.worker_job().get_name(WorkerJobStaticDataKey::from_raw(j));
        if (jnm == nullptr) {
            jnm = "unknown_job";
        }
        char safe[96];
        job_file_nm(jnm, safe, sizeof(safe));
        char path[384];
        if (std::snprintf(path, sizeof(path), "%s/%s_%s.ppm", g_out_dir, file_pfx, safe) <= 0) {
            continue;
        }
        if (write_imp_ppm(path, map, marks[j])) {
            std::printf("  map %s (%u tiles)\n", path, (unsigned)n);
        } else {
            std::printf("  fail write %s\n", path);
        }
    }
}

static bool setup_city (GameArraySimple& map, CityArray& cities, u16 player, u16 x, u16 y, u16 pop, u16 city_idx) {
    City* city = cities.get_city(city_idx);
    if (city == nullptr) {
        return false;
    }
    city->init(player, x, y);
    city->set_population(pop);
    if (!map.set_tile_add(x, y, city_idx, BUILD_ADD_CITY)) {
        return false;
    }
    CityBorder::claim_expand(x, y, 0, G_CLAIM_CULT, static_cast<u8>(player));
    CityTileManager::clear(x, y, city_idx);
    return true;
}

static bool try_apply_job (
    GameArraySimple& map,
    const RuntimeStatics& st,
    u16 ux,
    u16 uy,
    TileAssignIntent intent,
    u16 job,
    std::vector<StatRow>& rows,
    std::vector<std::vector<u8>>& marks,
    u16 mw,
    u32* job_n,
    u64* guide_ns,
    u32* guide_n)
{
    if (job == U16_KEY_NULL) {
        return false;
    }
    char geo[96];
    char geo_clr[256];
    u8 clim = 0;
    u8 terr = 0;
    geo_parts(map, ux, uy, geo, sizeof(geo), geo_clr, sizeof(geo_clr), &clim, &terr);
    cstr jnm = st.worker_job().get_name(WorkerJobStaticDataKey::from_raw(job));
    if (jnm == nullptr) {
        jnm = "unknown_job";
    }
    const auto a0 = std::chrono::steady_clock::now();
    const bool ok = WorkerGuidance::apply_job(ux, uy, job);
    const auto a1 = std::chrono::steady_clock::now();
    *guide_ns = *guide_ns + static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(a1 - a0).count());
    *guide_n = *guide_n + 1u;
    if (!ok) {
        return false;
    }
    bump_stat(rows, jnm, geo, geo_clr, intent_nm(static_cast<u8>(intent)), clim, terr);
    mark_job(marks, job, mw, ux, uy);
    *job_n = *job_n + 1u;
    return true;
}

static bool try_apply_next (
    GameArraySimple& map,
    const RuntimeStatics& st,
    u16 ux,
    u16 uy,
    TileAssignIntent intent,
    std::vector<StatRow>& rows,
    std::vector<std::vector<u8>>& marks,
    u16 mw,
    u32* job_n,
    u64* guide_ns,
    u32* guide_n)
{
    const auto t0 = std::chrono::steady_clock::now();
    const u16 job = WorkerGuidance::next_job(ux, uy, intent);
    const auto t1 = std::chrono::steady_clock::now();
    *guide_ns = *guide_ns + static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    *guide_n = *guide_n + 1u;
    return try_apply_job(map, st, ux, uy, intent, job, rows, marks, mw, job_n, guide_ns, guide_n);
}

struct OptPick {
    u16 m_x;
    u16 m_y;
    TileAssignIntent m_intent;
    u16 m_job;
};

static bool scan_best_worked (GameArraySimple& map, u16 cx, u16 cy, u16 city_idx, OptPick* out) {
    const CircArea area = CircularTileAreas::get(4);
    u8 have_food = 0;
    u8 have_prod = 0;
    u16 best_food = 0;
    u16 best_prod = 0;
    OptPick food_pick = {};
    OptPick prod_pick = {};
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
        if (map.get_city_worker(ux, uy) != city_idx) {
            continue;
        }
        const TileAssignIntent intent = static_cast<TileAssignIntent>(map.get_tile_usage(ux, uy));
        const u16 job = WorkerGuidance::next_job(ux, uy, intent);
        if (job == U16_KEY_NULL) {
            continue;
        }
        const TileYield yld = TileYields::get(ux, uy);
        if (intent == TILE_ASSIGN_FOOD) {
            const u16 score = TileYields::food_no_imp(ux, uy);
            if (have_food == 0 || score > best_food) {
                best_food = score;
                food_pick.m_x = ux;
                food_pick.m_y = uy;
                food_pick.m_intent = intent;
                food_pick.m_job = job;
                have_food = 1;
            }
        } else {
            const u16 score = yld.m_production;
            if (have_prod == 0 || score > best_prod) {
                best_prod = score;
                prod_pick.m_x = ux;
                prod_pick.m_y = uy;
                prod_pick.m_intent = intent;
                prod_pick.m_job = job;
                have_prod = 1;
            }
        }
    }
    if (have_food != 0) {
        *out = food_pick;
        return true;
    }
    if (have_prod != 0) {
        *out = prod_pick;
        return true;
    }
    return false;
}

static void apply_city_tiles (
    GameArraySimple& map,
    const RuntimeStatics& st,
    u16 cx,
    u16 cy,
    u16 city_idx,
    u16 player,
    u16 start_food,
    u16 pop,
    bool do_update,
    bool do_opt,
    std::vector<StatRow>& rows,
    std::vector<std::vector<u8>>& marks,
    u32* job_n,
    u64* guide_ns,
    u32* guide_n,
    u64* assign_ns,
    u32* assign_n,
    u64* scan_ns,
    u32* scan_n)
{
    const CircArea area = CircularTileAreas::get(4);
    const u16 mw = map.width();
    if (do_opt) {
        for (;;) {
            if (do_update) {
                const auto a0 = std::chrono::steady_clock::now();
                CityTileManager::stable_food_max_production(player, city_idx, start_food, pop);
                const auto a1 = std::chrono::steady_clock::now();
                *assign_ns = *assign_ns + static_cast<u64>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(a1 - a0).count());
                *assign_n = *assign_n + 1u;
            }
            OptPick pick = {};
            const auto s0 = std::chrono::steady_clock::now();
            const bool found = scan_best_worked(map, cx, cy, city_idx, &pick);
            const auto s1 = std::chrono::steady_clock::now();
            *scan_ns = *scan_ns + static_cast<u64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(s1 - s0).count());
            *scan_n = *scan_n + 1u;
            if (!found) {
                break;
            }
            if (!try_apply_job(
                    map, st, pick.m_x, pick.m_y, pick.m_intent, pick.m_job,
                    rows, marks, mw, job_n, guide_ns, guide_n)) {
                break;
            }
        }
        return;
    }
    if (!do_update) {
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
            if (map.get_city_worker(ux, uy) != city_idx) {
                continue;
            }
            const TileAssignIntent intent = static_cast<TileAssignIntent>(map.get_tile_usage(ux, uy));
            for (;;) {
                if (!try_apply_next(map, st, ux, uy, intent, rows, marks, mw, job_n, guide_ns, guide_n)) {
                    break;
                }
            }
        }
        return;
    }
    for (;;) {
        const auto a0 = std::chrono::steady_clock::now();
        CityTileManager::stable_food_max_production(player, city_idx, start_food, pop);
        const auto a1 = std::chrono::steady_clock::now();
        *assign_ns = *assign_ns + static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(a1 - a0).count());
        *assign_n = *assign_n + 1u;
        u8 applied = 0;
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
            if (map.get_city_worker(ux, uy) != city_idx) {
                continue;
            }
            const TileAssignIntent intent = static_cast<TileAssignIntent>(map.get_tile_usage(ux, uy));
            if (try_apply_next(map, st, ux, uy, intent, rows, marks, mw, job_n, guide_ns, guide_n)) {
                applied = 1;
                break;
            }
        }
        if (applied == 0) {
            break;
        }
    }
}

static bool cli_has_flag (int argc, char** argv, cstr flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) {
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main (int argc, char** argv) {
    const bool do_update = cli_has_flag(argc, argv, "--update");
    const bool do_opt = cli_has_flag(argc, argv, "--opt");
    if (!build_paths()) {
        std::printf("fail build paths\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("fail load runtime statics\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    BitArrayCL tech(st.tech().get_item_count());
    for (u32 i = 0; i < tech.get_count(); ++i) {
        tech.set_bit(i);
    }
    TileYieldCtx yctx = {};
    yctx.m_tech = &tech;
    if (!TileYields::setup(st)) {
        std::printf("fail setup tile yields\n");
        return 1;
    }
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov)) {
        std::printf("fail load map\n");
        return 1;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&map, g_res)) {
        std::printf("fail load resources\n");
        return 1;
    }
    TileYields::bind_map(&map);
    TileYields::bind_ctx(&yctx);
    TileWorking::bind_map(&map);
    CityBorder::bind_map(&map);
    WorkerGuidance::bind_statics(&st);
    WorkerGuidance::bind_map(&map);
    TileWorkCtx wctx = {};
    wctx.m_tech = &tech;
    wctx.m_resource = nullptr;
    TileWorkAssessor::bind_ctx(&wctx);
    CityArray cities;
    if (!cities.bind_statics(st)) {
        std::printf("fail bind city array\n");
        return 1;
    }
    CityTileManager::bind_cities(&cities);

    std::mt19937 rng(90210u);
    std::uniform_int_distribution<u16> pop_dist(G_POP_LO, G_POP_HI);
    std::vector<u16> loc_x(G_CITY_N);
    std::vector<u16> loc_y(G_CITY_N);
    if (!pick_locs(map, &rng, loc_x.data(), loc_y.data(), G_CITY_N)) {
        std::printf("fail pick %u city sites\n", (unsigned)G_CITY_N);
        return 1;
    }

    const u16 player = 0;
    std::vector<StatRow> rows;
    std::vector<std::vector<u8>> marks;
    ensure_marks(marks, st.worker_job().get_item_count(), map.tile_n());
    u32 job_n = 0;
    u32 worked_n = 0;
    u32 city_ok = 0;
    u64 guide_ns = 0;
    u32 guide_n = 0;
    u64 assign_ns = 0;
    u32 assign_n = 0;
    u64 scan_ns = 0;
    u32 scan_n = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (u32 i = 0; i < G_CITY_N; ++i) {
        const u16 city_idx = cities.get_next_new_city_idx();
        if (cities.get_city(city_idx) == nullptr) {
            std::printf("fail alloc city %u\n", (unsigned)i);
            return 1;
        }
        const u16 pop = pop_dist(rng);
        if (!setup_city(map, cities, player, loc_x[i], loc_y[i], pop, city_idx)) {
            continue;
        }
        const u16 start_food = TileYields::get(loc_x[i], loc_y[i]).m_food;
        {
            const auto a0 = std::chrono::steady_clock::now();
            CityTileManager::stable_food_max_production(player, city_idx, start_food, pop);
            const auto a1 = std::chrono::steady_clock::now();
            assign_ns = assign_ns + static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(a1 - a0).count());
            assign_n = assign_n + 1u;
        }
        apply_city_tiles(
            map, st, loc_x[i], loc_y[i], city_idx, player, start_food, pop, do_update, do_opt,
            rows, marks, &job_n, &guide_ns, &guide_n, &assign_ns, &assign_n, &scan_ns, &scan_n);
        worked_n += CityTileManager::count_worked(loc_x[i], loc_y[i], city_idx);
        city_ok = city_ok + 1u;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double guide_avg_ns = guide_n == 0 ? 0.0 : static_cast<double>(guide_ns) / static_cast<double>(guide_n);
    const double guide_tot_us = static_cast<double>(guide_ns) / 1.0e3;
    const double assign_avg_ns = assign_n == 0 ? 0.0 : static_cast<double>(assign_ns) / static_cast<double>(assign_n);
    const double assign_tot_us = static_cast<double>(assign_ns) / 1.0e3;
    const double scan_avg_ns = scan_n == 0 ? 0.0 : static_cast<double>(scan_ns) / static_cast<double>(scan_n);
    const double scan_tot_us = static_cast<double>(scan_ns) / 1.0e3;

    std::sort(rows.begin(), rows.end(), [](const StatRow& a, const StatRow& b) {
        if (a.m_job != b.m_job) {
            return a.m_job < b.m_job;
        }
        if (a.m_clim != b.m_clim) {
            return a.m_clim < b.m_clim;
        }
        if (a.m_terr != b.m_terr) {
            return a.m_terr < b.m_terr;
        }
        if (a.m_geo != b.m_geo) {
            return a.m_geo < b.m_geo;
        }
        if (a.m_intent != b.m_intent) {
            return a.m_intent < b.m_intent;
        }
        return a.m_n > b.m_n;
    });

    std::printf("worker_guidance stats: cities=%u/%u worked_tiles=%u jobs=%u wall_ms=%.3f update=%s opt=%s\n",
        (unsigned)city_ok, (unsigned)G_CITY_N, (unsigned)worked_n, (unsigned)job_n, ms,
        do_update ? "on" : "off", do_opt ? "on" : "off");
    std::printf("guidance timing: calls=%u avg_ns=%.2f total_us=%.2f\n",
        (unsigned)guide_n, guide_avg_ns, guide_tot_us);
    std::printf("stable_food_max_production timing: calls=%u avg_ns=%.2f total_us=%.2f\n",
        (unsigned)assign_n, assign_avg_ns, assign_tot_us);
    std::printf("scan_best_worked timing: calls=%u avg_ns=%.2f total_us=%.2f\n",
        (unsigned)scan_n, scan_avg_ns, scan_tot_us);
    for (size_t i = 0; i < rows.size(); ++i) {
        std::printf("%s on [%s] intent=%s %u times\n",
            rows[i].m_job.c_str(), rows[i].m_geo_clr.c_str(), rows[i].m_intent.c_str(), (unsigned)rows[i].m_n);
    }
    std::printf("-------------------------------------------------------\n");
    std::printf("totals by improvement:\n");
    {
        std::string cur;
        u32 sum = 0;
        for (size_t i = 0; i < rows.size(); ++i) {
            if (i == 0) {
                cur = rows[i].m_job;
                sum = rows[i].m_n;
                continue;
            }
            if (rows[i].m_job != cur) {
                std::printf("  %s: %u\n", cur.c_str(), (unsigned)sum);
                cur = rows[i].m_job;
                sum = rows[i].m_n;
            } else {
                sum = sum + rows[i].m_n;
            }
        }
        if (!cur.empty()) {
            std::printf("  %s: %u\n", cur.c_str(), (unsigned)sum);
        }
    }
    std::printf("-------------------------------------------------------\n");
    std::printf("improvement maps:\n");
    {
        const u32 idx = (do_update ? 2u : 0u) + (do_opt ? 1u : 0u);
        char pfx[64];
        if (std::snprintf(pfx, sizeof(pfx), "%u_wg_stats%s%s", (unsigned)idx,
                do_update ? "_upd" : "", do_opt ? "_opt" : "") <= 0) {
            std::snprintf(pfx, sizeof(pfx), "%u_wg_stats", (unsigned)idx);
        }
        write_imp_maps(map, st, marks, pfx);
    }

    WorkerGuidance::bind_map(nullptr);
    WorkerGuidance::bind_statics(nullptr);
    CityTileManager::bind_cities(nullptr);
    CityBorder::bind_map(nullptr);
    TileWorking::bind_map(nullptr);
    TileYields::bind_ctx(nullptr);
    TileYields::bind_map(nullptr);
    map.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
