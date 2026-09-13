//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
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
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/gen-land-sectors";
static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const u32 G_SEED = 43u;
static const u32 G_RNG = 43u;
static const u16 k_pal_n = 24u;

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

static void hsv_med (u16 hi, u8* r, u8* g, u8* b) {
    const float h = (static_cast<float>(hi % k_pal_n) * 360.0f) / static_cast<float>(k_pal_n);
    const float s = 0.58f;
    const float v = 0.78f;
    const float c = v * s;
    const float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    const float m = v - c;
    float rf = 0.0f;
    float gf = 0.0f;
    float bf = 0.0f;
    if (h < 60.0f) {
        rf = c;
        gf = x;
    } else if (h < 120.0f) {
        rf = x;
        gf = c;
    } else if (h < 180.0f) {
        gf = c;
        bf = x;
    } else if (h < 240.0f) {
        gf = x;
        bf = c;
    } else if (h < 300.0f) {
        rf = x;
        bf = c;
    } else {
        rf = c;
        bf = x;
    }
    *r = static_cast<u8>((rf + m) * 255.0f + 0.5f);
    *g = static_cast<u8>((gf + m) * 255.0f + 0.5f);
    *b = static_cast<u8>((bf + m) * 255.0f + 0.5f);
}

static u16 hue_dist (u16 a, u16 b) {
    const u16 d = a > b ? static_cast<u16>(a - b) : static_cast<u16>(b - a);
    const u16 wrap = static_cast<u16>(k_pal_n - d);
    return d < wrap ? d : wrap;
}

static void assign_adj_colors (u16 sec_n, const LandSectorNetwork& net, std::vector<u8>& rr, std::vector<u8>& gg, std::vector<u8>& bb) {
    rr.assign(sec_n, 80u);
    gg.assign(sec_n, 80u);
    bb.assign(sec_n, 80u);
    if (sec_n == 0u) {
        return;
    }
    std::vector<u16> deg(sec_n, 0u);
    for (u16 i = 0; i < net.link_n(); ++i) {
        const LandSectorLink* L = net.get(i);
        if (L == nullptr || L->m_a >= sec_n || L->m_b >= sec_n) {
            continue;
        }
        deg[L->m_a] = static_cast<u16>(deg[L->m_a] + 1u);
        deg[L->m_b] = static_cast<u16>(deg[L->m_b] + 1u);
    }
    std::vector<u16> head(sec_n, U16_KEY_NULL);
    std::vector<u16> nxt(net.link_n() * 2u, U16_KEY_NULL);
    std::vector<u16> to(net.link_n() * 2u, U16_KEY_NULL);
    u16 en = 0u;
    for (u16 i = 0; i < net.link_n(); ++i) {
        const LandSectorLink* L = net.get(i);
        if (L == nullptr || L->m_a >= sec_n || L->m_b >= sec_n) {
            continue;
        }
        nxt[en] = head[L->m_a];
        to[en] = L->m_b;
        head[L->m_a] = en;
        ++en;
        nxt[en] = head[L->m_b];
        to[en] = L->m_a;
        head[L->m_b] = en;
        ++en;
    }
    std::vector<i16> hue(sec_n, -1);
    for (u16 s = 0; s < sec_n; ++s) {
        u16 best_h = 0u;
        u16 best_sc = 0u;
        for (u16 h = 0; h < k_pal_n; ++h) {
            u16 mn = k_pal_n;
            for (u16 e = head[s]; e != U16_KEY_NULL; e = nxt[e]) {
                const u16 n = to[e];
                if (hue[n] < 0) {
                    continue;
                }
                const u16 d = hue_dist(h, static_cast<u16>(hue[n]));
                if (d < mn) {
                    mn = d;
                }
            }
            if (mn > best_sc || (mn == best_sc && h < best_h)) {
                best_sc = mn;
                best_h = h;
            }
        }
        hue[s] = static_cast<i16>(best_h);
        hsv_med(best_h, &rr[s], &gg[s], &bb[s]);
        (void)deg;
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

static void put_px (std::vector<u8>& rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * w + static_cast<u32>(x)) * 3u;
    rgb[i] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void put_dot (std::vector<u8>& rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            put_px(rgb, w, h, static_cast<i32>(x) + dx, static_cast<i32>(y) + dy, r, g, b);
        }
    }
}

static void draw_line (std::vector<u8>& rgb, u16 w, u16 h, u16 x0, u16 y0, u16 x1, u16 y1) {
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
        put_px(rgb, w, h, x, y, 20u, 20u, 20u);
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

static void paint_base (
    std::vector<u8>& rgb,
    const GameArraySimple& map,
    const GenLandSectors& gls,
    const std::vector<u8>& rr,
    const std::vector<u8>& gg,
    const std::vector<u8>& bb) {
    const Whiteboard_2B& sec = gls.sectors();
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 sn = gls.sector_n();
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
                    r = rr[id];
                    g = gg[id];
                    b = bb[id];
                }
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
}

static bool write_edges_ppm (
    cstr path,
    const GameArraySimple& map,
    const GenLandSectors& gls,
    const LandSectorSeeds& seeds,
    const LandSectorNetwork& net,
    const std::vector<u8>& rr,
    const std::vector<u8>& gg,
    const std::vector<u8>& bb) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    paint_base(rgb, map, gls, rr, gg, bb);
    for (u16 i = 0; i < net.link_n(); ++i) {
        const LandSectorLink* L = net.get(i);
        if (L == nullptr || seeds.m_pts == nullptr) {
            continue;
        }
        if (L->m_a >= seeds.m_n || L->m_b >= seeds.m_n) {
            continue;
        }
        const LandSectorSeedPt& a = seeds.m_pts[L->m_a];
        const LandSectorSeedPt& b = seeds.m_pts[L->m_b];
        draw_line(rgb, w, h, a.m_x, a.m_y, b.m_x, b.m_y);
    }
    for (u16 i = 0; i < seeds.m_n; ++i) {
        put_dot(rgb, w, h, seeds.m_pts[i].m_x, seeds.m_pts[i].m_y, 220u, 40u, 40u);
    }
    return write_ppm(path, rgb, w, h);
}

static bool write_passages_ppm (
    cstr path,
    const GameArraySimple& map,
    const GenLandSectors& gls,
    const LandSectorNetwork& net,
    const std::vector<u8>& rr,
    const std::vector<u8>& gg,
    const std::vector<u8>& bb) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    paint_base(rgb, map, gls, rr, gg, bb);
    for (u16 i = 0; i < net.link_n(); ++i) {
        const LandSectorLink* L = net.get(i);
        if (L == nullptr) {
            continue;
        }
        put_dot(rgb, w, h, L->m_x, L->m_y, 255u, 240u, 40u);
    }
    return write_ppm(path, rgb, w, h);
}

static double ms_since (std::chrono::steady_clock::time_point t0) {
    const auto dt = std::chrono::steady_clock::now() - t0;
    return std::chrono::duration<double, std::milli>(dt).count();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
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
    std::printf("sectors=%u paint=%u\n",
        static_cast<unsigned>(gls.sector_n()),
        static_cast<unsigned>(gls.paint_n()));

    LandSectorNetwork net;
    auto t0 = std::chrono::steady_clock::now();
    const bool net_ok = GenLandSectorNetwork::build(gls.sectors(), gls.sector_n(), map, &net);
    const double net_ms = ms_since(t0);
    note_result(net_ok && net.ok(), "GenLandSectorNetwork::build");
    note_result(net.link_n() > 0u, "link_n > 0");
    std::printf("network: links=%u  wall_ms=%.3f\n",
        static_cast<unsigned>(net.link_n()), net_ms);

    std::vector<u8> rr;
    std::vector<u8> gg;
    std::vector<u8> bb;
    assign_adj_colors(gls.sector_n(), net, rr, gg, bb);

    char edges_path[384];
    char pass_path[384];
    std::snprintf(edges_path, sizeof(edges_path), "%s/land_sector_network_edges.ppm", G_OUT_DIR);
    std::snprintf(pass_path, sizeof(pass_path), "%s/land_sector_network_passages.ppm", G_OUT_DIR);
    note_result(write_edges_ppm(edges_path, map, gls, seeds, net, rr, gg, bb), edges_path);
    note_result(write_passages_ppm(pass_path, map, gls, net, rr, gg, bb), pass_path);
    std::printf("wrote %s\n", edges_path);
    std::printf("wrote %s\n", pass_path);

    GenLandSectors::free_seeds(&seeds);
    WhiteboardMng::terminate();
    std::printf("=======================================================\n");
    std::printf(" TESTING GEN_LAND_SECTOR_NETWORK: TOTAL FAILURES: %d/%d\n",
        total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
