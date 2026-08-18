//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>
#include <vector>

#include "city_blocking_mask.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "gen_settlement_order.h"
#include "gen_settlement_targets.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "sector_network.h"
#include "sector_network_router.h"
#include "settler_mission_manager.h"
#include "starting_point_generator.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u16 G_PICK_N = 100u;
static const u16 G_LATT_DIV = 10u;
static const u16 G_TURN_N = 200u;
static const u16 G_SPAWN = 10u;
static const u16 G_SET_MAX = 16384u;
static const int G_DOT_R = 1;
static const u8 G_RIV_R = 40u;
static const u8 G_RIV_G = 100u;
static const u8 G_RIV_B = 220u;
static const u8 G_DOT_R0 = 255u;
static const u8 G_DOT_G0 = 236u;
static const u8 G_DOT_B0 = 236u;
static const u8 G_DOT_R1 = 48u;
static const u8 G_DOT_G1 = 0u;
static const u8 G_DOT_B1 = 0u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_dir[256];
static char g_out[320];
static char g_locd[320];
static bool g_img = false;
static bool g_opp = false;
static bool g_loc = false;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

struct MockSet {
    u8 m_live;
    u16 m_pl;
    u16 m_x;
    u16 m_y;
    u16 m_slot;
};

struct MockCity {
    u16 m_pl;
    u16 m_x;
    u16 m_y;
    u16 m_born;
};

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
        total_test_fails++;
        if (print_level > 0) {
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

static bool build_paths () {
    if (std::snprintf(g_dir, sizeof(g_dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_out, sizeof(g_out), "%s/settler_mission", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_locd, sizeof(g_locd), "%s/loc", g_out) <= 0) {
        return false;
    }
    return true;
}

static bool mk_dir (cstr path) {
    if (mkdir(path, 0755) == 0) {
        return true;
    }
    return errno == EEXIST;
}

static void latt_for_map (u16 w, u16 h, u16* rows, u16* cols) {
    u16 r = h / G_LATT_DIV;
    u16 c = w / G_LATT_DIV;
    if (r == 0) {
        r = 1;
    }
    if (c == 0) {
        c = 1;
    }
    *rows = r;
    *cols = c;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    static const u8* const k_rows[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS,
        TERR_MOUNTAINS, TERR_VOLCANO, TERR_INLAND_SEA, TERR_INLAND_LAKE};
    for (unsigned i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == cls) {
            *r = k_rows[i][1];
            *g = k_rows[i][2];
            *b = k_rows[i][3];
            return;
        }
    }
    *r = 0;
    *g = 0;
    *b = 0;
}

static void fill_terr_riv (std::vector<u8>& rgb, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(map.get_terrain(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0u) {
                r = G_RIV_R;
                g = G_RIV_G;
                b = G_RIV_B;
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
}

static void pl_rgb (u16 pl, u8* r, u8* g, u8* b) {
    *r = static_cast<u8>(48u + (static_cast<u32>(pl) * 91u) % 200u);
    *g = static_cast<u8>(48u + (static_cast<u32>(pl) * 47u) % 200u);
    *b = static_cast<u8>(48u + (static_cast<u32>(pl) * 157u) % 200u);
}

static u8 lerp_u8 (u8 a, u8 b, u32 i, u32 n) {
    if (n <= 1u) {
        return a;
    }
    const u32 d = n - 1u;
    return static_cast<u8>((static_cast<u32>(a) * (d - i) + static_cast<u32>(b) * i) / d);
}

static void paint_sq (std::vector<u8>& rgb, u16 w, u16 h, u16 cx, u16 cy, u8 r, u8 g, u8 b) {
    for (int dy = -G_DOT_R; dy <= G_DOT_R; ++dy) {
        for (int dx = -G_DOT_R; dx <= G_DOT_R; ++dx) {
            const int x = static_cast<int>(cx) + dx;
            const int y = static_cast<int>(cy) + dy;
            if (x < 0 || y < 0 || x >= static_cast<int>(w) || y >= static_cast<int>(h)) {
                continue;
            }
            const u32 i = (static_cast<u32>(y) * w + static_cast<u32>(x)) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
}

static bool write_ppm (cstr path, const std::vector<u8>& rgb, u16 w, u16 h) {
    FILE* f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    const size_t nbytes = rgb.size();
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, f) == nbytes;
    std::fclose(f);
    return ok;
}

static bool write_turn (
    u16 turn,
    const GameArraySimple& map,
    const std::vector<MockCity>& cities,
    const MockSet* sets,
    u16 set_n)
{
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    fill_terr_riv(rgb, map);
    for (u32 i = 0; i < cities.size(); ++i) {
        const MockCity& c = cities[i];
        u8 cr = 0;
        u8 cg = 0;
        u8 cb = 0;
        pl_rgb(c.m_pl, &cr, &cg, &cb);
        paint_sq(rgb, w, h, c.m_x, c.m_y, cr, cg, cb);
    }
    for (u16 i = 0; i < set_n; ++i) {
        if (sets[i].m_live == 0) {
            continue;
        }
        const u16 x = sets[i].m_x;
        const u16 y = sets[i].m_y;
        if (x >= w || y >= h) {
            continue;
        }
        const u32 pi = (static_cast<u32>(y) * w + x) * 3u;
        rgb[pi] = 0;
        rgb[pi + 1] = 0;
        rgb[pi + 2] = 0;
    }
    char path[384];
    std::snprintf(path, sizeof(path), "%s/t%04u.ppm", g_out, turn);
    return write_ppm(path, rgb, w, h);
}

static bool write_order_ppm (cstr path, const GameArraySimple& map, const GenSettlementOrder& ord, u16 p) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = ord.n(p);
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    fill_terr_riv(rgb, map);
    for (u32 i = 0; i < n; ++i) {
        const SpgCoordPair pt = ord.at(p, i);
        const u8 r = lerp_u8(G_DOT_R0, G_DOT_R1, i, n);
        const u8 g = lerp_u8(G_DOT_G0, G_DOT_G1, i, n);
        const u8 b = lerp_u8(G_DOT_B0, G_DOT_B1, i, n);
        paint_sq(rgb, w, h, pt.x, pt.y, r, g, b);
    }
    return write_ppm(path, rgb, w, h);
}

static bool write_start (const GameArraySimple& map, const GenSettlementOrder& ord) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    fill_terr_riv(rgb, map);
    for (u16 p = 0; p < ord.pn(); ++p) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        pl_rgb(p, &r, &g, &b);
        const u32 n = ord.n(p);
        for (u32 i = 0; i < n; ++i) {
            const SpgCoordPair pt = ord.at(p, i);
            paint_sq(rgb, w, h, pt.x, pt.y, r, g, b);
        }
    }
    char path[384];
    std::snprintf(path, sizeof(path), "%s/start.ppm", g_out);
    return write_ppm(path, rgb, w, h);
}

static void pr_list_n (const GenSettlementOrder& ord) {
    const u16 pn = ord.pn();
    u32 ns[GSO_MAX_PN];
    for (u16 p = 0; p < pn; ++p) {
        ns[p] = ord.n(p);
    }
    for (u16 i = 1; i < pn; ++i) {
        const u32 v = ns[i];
        u16 j = i;
        while (j > 0u && ns[static_cast<u16>(j - 1u)] > v) {
            ns[j] = ns[static_cast<u16>(j - 1u)];
            j = static_cast<u16>(j - 1u);
        }
        ns[j] = v;
    }
    std::printf("list_n");
    for (u16 p = 0; p < pn; ++p) {
        std::printf(" %u", ns[p]);
    }
    std::printf("\n");
}

static bool write_locs (const GameArraySimple& map, const GenSettlementOrder& ord) {
    if (!mk_dir(g_locd)) {
        return false;
    }
    bool ok = true;
    for (u16 p = 0; p < ord.pn(); ++p) {
        char path[384];
        std::snprintf(path, sizeof(path), "%s/p%03u.ppm", g_locd, p);
        if (!write_order_ppm(path, map, ord, p)) {
            ok = false;
        }
    }
    return ok;
}

static bool occ_at (const MockSet* sets, u16 n, u16 x, u16 y) {
    for (u16 i = 0; i < n; ++i) {
        if (sets[i].m_live != 0 && sets[i].m_x == x && sets[i].m_y == y) {
            return true;
        }
    }
    return false;
}

static u16 add_set (MockSet* sets, u16* n, u16 pl, u16 x, u16 y) {
    u16 i = U16_KEY_NULL;
    for (u16 k = 0; k < *n; ++k) {
        if (sets[k].m_live == 0) {
            i = k;
            break;
        }
    }
    if (i == U16_KEY_NULL) {
        if (*n >= G_SET_MAX) {
            return U16_KEY_NULL;
        }
        i = *n;
        *n = static_cast<u16>(*n + 1u);
    }
    sets[i].m_live = 1;
    sets[i].m_pl = pl;
    sets[i].m_x = x;
    sets[i].m_y = y;
    sets[i].m_slot = U16_KEY_NULL;
    return i;
}

static void try_spawn (MockSet* sets, u16* n, const MockCity& c) {
    if (occ_at(sets, *n, c.m_x, c.m_y)) {
        return;
    }
    add_set(sets, n, c.m_pl, c.m_x, c.m_y);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--img") == 0) {
            g_img = true;
        } else if (std::strcmp(argv[i], "--opp") == 0) {
            g_opp = true;
        } else if (std::strcmp(argv[i], "--loc") == 0) {
            g_loc = true;
        } else {
            print_level = std::atoi(argv[i]);
        }
    }
    note_result(build_paths(), "build map paths");
    note_result(mk_dir(g_out), "make output dir");

    MapTerrainData terr;
    note_result(MapLoader::load_terrain_ppm(g_terr, terr), "load terrain");
    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");

    const u16 mw = map.width();
    const u16 mh = map.height();
    const u32 tn = map.tile_n();
    std::vector<u8> clim(tn);
    std::vector<u8> ov(tn);
    std::vector<u8> riv(tn);
    std::vector<u8> terr_u8(tn);
    for (u16 y = 0; y < mh; ++y) {
        for (u16 x = 0; x < mw; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(mw) + static_cast<u32>(x);
            clim[i] = map.get_climate(x, y);
            ov[i] = map.get_overlay(x, y);
            riv[i] = map.get_river(x, y);
            terr_u8[i] = map.get_terrain(x, y);
        }
    }
    u16 latt_r = 0;
    u16 latt_c = 0;
    latt_for_map(mw, mh, &latt_r, &latt_c);
    StartingPointGeneratorParams par = {};
    par.map = &terr;
    par.climate = clim.data();
    par.overlay = ov.data();
    par.river = riv.data();
    par.pick_n = G_PICK_N;
    par.latt_rows = latt_r;
    par.latt_cols = latt_c;
    par.seed = G_SEED;
    StartingPointGenerator spg(par);
    note_result(spg.generate(), "generate starting points");
    const SpgPickCoords starts = spg.picks_coords();
    note_result(starts.n == G_PICK_N, "have 100 starting points");
    std::printf("starts=%u map=%ux%u\n", starts.n, mw, mh);

    GenSettlementTargetsRslt rslt = {};
    note_result(GenSettlementTargets::generate(map, starts.pts, starts.n, &rslt), "GenSettlementTargets::generate");
    std::printf("planned total=%u\n", rslt.m_n);

    WhiteboardMng::init(mw, mh);
    GenSettlementOrder ord;
    note_result(ord.gen_excl(map, starts.pts, starts.n), "GenSettlementOrder::gen_excl");
    note_result(write_start(map, ord), "wrote start image");
    pr_list_n(ord);
    if (g_loc) {
        note_result(write_locs(map, ord), "wrote loc images");
    }

    SectorNetwork net;
    note_result(net.begin(mw, mh, terr_u8.data()), "SectorNetwork::begin");
    SectorNetworkRouter rt;
    note_result(rt.begin(net), "SectorNetworkRouter::begin");

    SettlerMissionManager::punch(map);
    const u16 pn = static_cast<u16>(starts.n);
    SettlerMissionManager* mgrs = new SettlerMissionManager[pn];
    bool beg_ok = true;
    for (u16 p = 0; p < pn; ++p) {
        if (!mgrs[p].begin(net, rt, terr_u8.data(), mw, mh)) {
            beg_ok = false;
        }
    }
    note_result(beg_ok, "SettlerMissionManager::begin per player");
    for (u16 p = 0; p < pn; ++p) {
        mgrs[p].opp(g_opp);
    }

    std::vector<MockCity> cities;
    for (u32 p = 0; p < starts.n; ++p) {
        const u16 x = starts.pts[p].x;
        const u16 y = starts.pts[p].y;
        map.set_planned_city(x, y, 0u);
        CityBlockingMask::stamp(map, x, y);
        MockCity c = {};
        c.m_pl = static_cast<u16>(p);
        c.m_x = x;
        c.m_y = y;
        c.m_born = 0;
        cities.push_back(c);
    }
    for (u16 y = 0; y < mh; ++y) {
        for (u16 x = 0; x < mw; ++x) {
            if (map.get_settler_blocked(x, y) != 0u) {
                map.set_planned_city(x, y, 0u);
            }
        }
    }

    MockSet sets[G_SET_MAX];
    u16 set_n = 0;
    u32 found_n = 0;
    u32 wait_n = 0;
    bool img_ok = true;
    double path_ms = 0.0;
    for (u16 t = 0; t < G_TURN_N; ++t) {
        for (u32 ci = 0; ci < cities.size(); ++ci) {
            const MockCity& c = cities[ci];
            if (t < c.m_born) {
                continue;
            }
            if (((t - c.m_born) % G_SPAWN) != 0u) {
                continue;
            }
            try_spawn(sets, &set_n, c);
        }
        const auto t0 = std::chrono::steady_clock::now();
        for (u16 i = 0; i < set_n; ++i) {
            MockSet& u = sets[i];
            if (u.m_live == 0 || u.m_slot != U16_KEY_NULL) {
                continue;
            }
            if (u.m_pl >= pn) {
                wait_n += 1u;
                continue;
            }
            if (mgrs[u.m_pl].idle() == 0u) {
                wait_n += 1u;
                continue;
            }
            const u16 s = mgrs[u.m_pl].asgn(map, ord, u.m_pl, u.m_x, u.m_y);
            if (s == U16_KEY_NULL) {
                wait_n += 1u;
                continue;
            }
            u.m_slot = s;
        }
        for (u16 i = 0; i < set_n; ++i) {
            MockSet& u = sets[i];
            if (u.m_live == 0 || u.m_slot == U16_KEY_NULL || u.m_pl >= pn) {
                continue;
            }
            SettlerMissionManager& mgr = mgrs[u.m_pl];
            const u8 ev = mgr.step(map, u.m_slot);
            u.m_x = mgr.x(u.m_slot);
            u.m_y = mgr.y(u.m_slot);
            if (ev == SMM_FOUND) {
                MockCity c = {};
                c.m_pl = mgr.pl(u.m_slot);
                c.m_x = u.m_x;
                c.m_y = u.m_y;
                c.m_born = t;
                cities.push_back(c);
                found_n += 1u;
                u.m_live = 0;
                u.m_slot = U16_KEY_NULL;
                try_spawn(sets, &set_n, c);
            } else if (ev == SMM_DROP) {
                u.m_slot = U16_KEY_NULL;
            }
        }
        const auto t1 = std::chrono::steady_clock::now();
        path_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (g_img || (t + 1u) == G_TURN_N) {
            if (!write_turn(t, map, cities, sets, set_n)) {
                img_ok = false;
            }
        }
        if ((t % G_SPAWN) == 0u) {
            u16 busy = 0;
            u16 live = 0;
            for (u16 i = 0; i < set_n; ++i) {
                live = static_cast<u16>(live + sets[i].m_live);
                if (sets[i].m_live != 0 && sets[i].m_slot != U16_KEY_NULL) {
                    busy = static_cast<u16>(busy + 1u);
                }
            }
            std::printf(
                "t=%04u cities=%u found=%u live=%u busy=%u\n",
                t,
                static_cast<u32>(cities.size()),
                found_n,
                live,
                busy);
        }
    }
    note_result(img_ok, "wrote turn images");
    note_result(found_n > 0u, "founded extra cities");
    const double per = path_ms / static_cast<double>(G_TURN_N);
    std::printf(
        "path_ms=%.3f path_ms_per_turn=%.3f turns=%u cities=%u found=%u waits=%u img=%u opp=%u loc=%u out=%s\n",
        path_ms,
        per,
        G_TURN_N,
        static_cast<u32>(cities.size()),
        found_n,
        wait_n,
        g_img ? 1u : 0u,
        g_opp ? 1u : 0u,
        g_loc ? 1u : 0u,
        g_out);

    delete[] mgrs;

    std::printf("=======================================================\n");
    std::printf(" SETTLER MISSION MANAGER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
