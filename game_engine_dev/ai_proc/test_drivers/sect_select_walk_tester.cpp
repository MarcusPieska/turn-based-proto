//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "gen_land_sector_network.h"
#include "gen_land_sectors.h"
#include "map_loader.h"
#include "runtime_static_loader.h"
#include "tile_attr_tables.h"
#include "expansion_state.h"
#include "land_sector_adj.h"
#include "sect_select_walk.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/sect-select-walk";
static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const u32 G_SEED = 43u;
static const u32 G_RNG = 43u;
static const u32 G_WALK_RNG = 7u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_flags[320];
static char g_dir[256];

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
    if (std::snprintf(g_flags, sizeof(g_flags), "%s/flags.ppm", g_dir) <= 0) {
        return false;
    }
    return true;
}

static void order_to_rgb (u16 step, u16 n, u8* r, u8* g, u8* b) {
    float t = 0.0f;
    if (n > 1u) {
        t = static_cast<float>(step) / static_cast<float>(n - 1u);
    }
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    *r = static_cast<u8>((1.0f - t) * 220.0f + 20.0f);
    *g = static_cast<u8>(40.0f);
    *b = static_cast<u8>(t * 220.0f + 20.0f);
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

static void put_px (std::vector<u8>& rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * w + static_cast<u32>(x)) * 3u;
    rgb[i] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void put_sq3 (std::vector<u8>& rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            put_px(rgb, w, h, static_cast<i32>(x) + dx, static_cast<i32>(y) + dy, r, g, b);
        }
    }
}

static void draw_line (
    std::vector<u8>& rgb,
    u16 w,
    u16 h,
    u16 x0,
    u16 y0,
    u16 x1,
    u16 y1,
    u8 r,
    u8 g,
    u8 b) {
    i32 dx = static_cast<i32>(x1) - static_cast<i32>(x0);
    i32 dy = static_cast<i32>(y1) - static_cast<i32>(y0);
    const i32 adx = dx < 0 ? -dx : dx;
    const i32 ady = dy < 0 ? -dy : dy;
    const i32 sx = dx < 0 ? -1 : 1;
    const i32 sy = dy < 0 ? -1 : 1;
    i32 err = adx - ady;
    i32 x = static_cast<i32>(x0);
    i32 y = static_cast<i32>(y0);
    for (;;) {
        put_px(rgb, w, h, x, y, r, g, b);
        if (x == static_cast<i32>(x1) && y == static_cast<i32>(y1)) {
            break;
        }
        const i32 e2 = err * 2;
        if (e2 > -ady) {
            err -= ady;
            x += sx;
        }
        if (e2 < adx) {
            err += adx;
            y += sy;
        }
    }
}

static bool write_net_ppm (
    cstr path,
    const GameArraySimple& map,
    const LandSectorSeeds& seeds,
    const LandSectorNetwork& net) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 t = map.get_terrain(x, y);
            u8 r = 32u;
            u8 g = 32u;
            u8 b = 32u;
            if (overlay_is_water_terr(t)) {
                r = 28u;
                g = 55u;
                b = 110u;
            } else if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
                r = 0u;
                g = 0u;
                b = 0u;
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    if (seeds.m_pts != nullptr) {
        for (u16 i = 0; i < net.link_n(); ++i) {
            const LandSectorLink* L = net.get(i);
            if (L == nullptr || L->m_a >= seeds.m_n || L->m_b >= seeds.m_n) {
                continue;
            }
            draw_line(rgb, w, h,
                seeds.m_pts[L->m_a].m_x, seeds.m_pts[L->m_a].m_y,
                seeds.m_pts[L->m_b].m_x, seeds.m_pts[L->m_b].m_y,
                255u, 255u, 255u);
        }
        for (u16 i = 0; i < seeds.m_n; ++i) {
            put_sq3(rgb, w, h, seeds.m_pts[i].m_x, seeds.m_pts[i].m_y, 240u, 220u, 40u);
        }
    }
    return write_ppm(path, rgb, w, h);
}

static bool write_walk_ppm (
    cstr path,
    const GameArraySimple& map,
    const GenLandSectors& gls,
    const LandSectorSeeds& seeds,
    const std::vector<u16>& ord) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 sn = gls.sector_n();
    const u16 on = static_cast<u16>(ord.size());
    const Whiteboard_2B& sec = gls.sectors();
    std::vector<u16> rank(sn, U16_KEY_NULL);
    for (u16 i = 0; i < on; ++i) {
        const u16 s = ord[i];
        if (s < sn) {
            rank[s] = i;
        }
    }
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 t = map.get_terrain(x, y);
            u8 r = 32u;
            u8 g = 32u;
            u8 b = 32u;
            if (overlay_is_water_terr(t)) {
                r = 255u;
                g = 255u;
                b = 255u;
            } else if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
                r = 0u;
                g = 0u;
                b = 0u;
            } else {
                const u16 tag = sec.rd(x, y);
                if (tag != GLS_IDX_NONE && tag <= sn) {
                    const u16 id = static_cast<u16>(tag - 1u);
                    if (rank[id] != U16_KEY_NULL) {
                        order_to_rgb(rank[id], on, &r, &g, &b);
                    }
                }
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    if (seeds.m_pts != nullptr) {
        for (u16 i = 1; i < on; ++i) {
            const u16 a = ord[static_cast<size_t>(i - 1u)];
            const u16 b = ord[i];
            if (a >= seeds.m_n || b >= seeds.m_n) {
                continue;
            }
            draw_line(rgb, w, h,
                seeds.m_pts[a].m_x, seeds.m_pts[a].m_y,
                seeds.m_pts[b].m_x, seeds.m_pts[b].m_y,
                12u, 12u, 12u);
        }
        for (u16 i = 0; i < on; ++i) {
            const u16 s = ord[i];
            if (s >= seeds.m_n) {
                continue;
            }
            put_sq3(rgb, w, h, seeds.m_pts[s].m_x, seeds.m_pts[s].m_y, 40u, 220u, 40u);
        }
    }
    return write_ppm(path, rgb, w, h);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    u16 walk_lim = 20u;
    if (argc > 1) {
        const int v = std::atoi(argv[1]);
        if (v > 0 && v <= 65535) {
            walk_lim = static_cast<u16>(v);
        }
    }
    if (argc > 2) {
        print_level = std::atoi(argv[2]);
    }
    note_result(build_paths(), "build map paths");
    note_result(::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST, "ensure out dir");

    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "RuntimeStaticLoader::load");
    note_result(TileAttrTables::setup(loader.statics()), "TileAttrTables::setup");

    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(Factory_GameArraySimple::load_flags_data(&map, g_flags), "load flags");
    note_result(map.width() > 0 && map.height() > 0, "map size");

    WhiteboardMng::init(map.width(), map.height());
    note_result(WhiteboardMng::width() == map.width(), "WhiteboardMng::init");

    GenLandSectors gls;
    note_result(gls.begin(map), "GenLandSectors::begin");
    LandSectorSeeds seeds = {};
    note_result(gls.gen_seeds(G_RNG, &seeds), "GenLandSectors::gen_seeds");
    note_result(seeds.m_n > 0u && gls.sector_n() > 0u, "sectors ready");

    LandSectorNetwork net;
    note_result(GenLandSectorNetwork::build(gls.sectors(), gls.sector_n(), map, &net), "network build");
    note_result(net.ok() && net.link_n() > 0u, "network ready");

    char net_path[384];
    std::snprintf(net_path, sizeof(net_path), "%s/sector_network.ppm", G_OUT_DIR);
    note_result(write_net_ppm(net_path, map, seeds, net), net_path);
    std::printf("wrote %s\n", net_path);

    LandSectorAdj adj;
    note_result(adj.build(net, gls.sector_n()), "LandSectorAdj::build");
    ExpansionState exp;
    note_result(SectorSelect_Walk::begin(exp, adj, seeds.m_pts, seeds.m_n, G_WALK_RNG), "SectorSelect_Walk::begin");
    std::vector<u16> ord;
    u16 sec = U16_KEY_NULL;
    u32 edge_n = 0u;
    u32 fwd_n = 0u;
    u32 back_n = 0u;
    while (ord.size() < static_cast<size_t>(walk_lim)) {
        const bool emit0 = exp.have();
        const u16 pn0 = exp.path_n();
        if (!SectorSelect_Walk::select(exp, adj, seeds.m_pts, &sec)) {
            const u32 pops = static_cast<u32>(pn0 - exp.path_n());
            back_n += pops;
            edge_n += pops;
            break;
        }
        if (!emit0) {
            const u32 pops = static_cast<u32>(pn0 + 1u - exp.path_n());
            back_n += pops;
            fwd_n += 1u;
            edge_n += pops + 1u;
        }
        ord.push_back(sec);
    }
    note_result(adj.ok() && exp.ok() && !ord.empty() && ord.size() <= static_cast<size_t>(walk_lim), "walk respects lim");
    std::printf("walk: start=%u order_n=%u lim=%u sectors=%u links=%u\n",
        static_cast<unsigned>(exp.start()),
        static_cast<unsigned>(ord.size()),
        static_cast<unsigned>(walk_lim),
        static_cast<unsigned>(gls.sector_n()),
        static_cast<unsigned>(net.link_n()));
    std::printf("edge_traversals=%u fwd=%u back=%u\n",
        static_cast<unsigned>(edge_n),
        static_cast<unsigned>(fwd_n),
        static_cast<unsigned>(back_n));

    char out_path[384];
    std::snprintf(out_path, sizeof(out_path), "%s/walk_order.ppm", G_OUT_DIR);
    note_result(write_walk_ppm(out_path, map, gls, seeds, ord), out_path);
    std::printf("wrote %s\n", out_path);

    GenLandSectors::free_seeds(&seeds);
    WhiteboardMng::terminate();
    std::printf("=======================================================\n");
    std::printf(" TESTING SECT_SELECT_WALK: TOTAL FAILURES: %d/%d\n",
        total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
