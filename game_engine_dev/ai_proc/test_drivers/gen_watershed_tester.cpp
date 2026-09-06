//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "gen_watershed.h"
#include "land_mass_index.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/gen-watershed";
static const u32 G_SEED = 43u;
static const u16 G_SYS_CAP = 100u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];

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
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool ensure_out_dir () {
    if (::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

static void terr_rgb (u8 terr, u8* r, u8* g, u8* b) {
    static const u8* k_terr[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS, TERR_MOUNTAINS, TERR_VOLCANO,
        TERR_INLAND_SEA, TERR_INLAND_LAKE
    };
    for (u16 i = 0; i < 10u; ++i) {
        if (k_terr[i][0] == terr) {
            *r = k_terr[i][1];
            *g = k_terr[i][2];
            *b = k_terr[i][3];
            return;
        }
    }
    *r = 40;
    *g = 40;
    *b = 40;
}

static bool write_sys_ppm (cstr path, const GameArraySimple& map, const Whiteboard_1B& ov) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(map.get_terrain(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0u) {
                r = 40;
                g = 90;
                b = 200;
            }
            if (ov.rd(x, y) != 0u) {
                r = 220;
                g = 30;
                b = 30;
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const bool ok = std::fwrite(rgb.data(), 1, rgb.size(), fp) == rgb.size();
    std::fclose(fp);
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths() || !ensure_out_dir()) {
        std::printf("path/out failed\n");
        return 1;
    }
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov)) {
        std::printf("load map failed\n");
        return 1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tn = map.tile_n();
    u8* terr = new u8[tn];
    for (u32 i = 0; i < tn; ++i) {
        terr[i] = map.get_terrain(static_cast<u16>(i % w), static_cast<u16>(i / w));
    }
    LandMassIndex mass;
    if (!mass.generate(terr, w, h) || !mass.is_valid()) {
        std::printf("LandMassIndex failed\n");
        delete[] terr;
        return 1;
    }
    delete[] terr;
    const LandMassIndexRslt& mr = mass.result();
    std::printf("land_masses=%u land_tiles=%u largest=%u\n",
        static_cast<unsigned>(mr.m_mass_n),
        static_cast<unsigned>(mr.m_land_n),
        static_cast<unsigned>(mr.m_largest_idx));

    u32* mass_sz = new u32[static_cast<u32>(mr.m_mass_n) + 1u];
    u16* mass_ord = new u16[static_cast<u32>(mr.m_mass_n)];
    for (u16 i = 0; i <= mr.m_mass_n; ++i) {
        mass_sz[i] = 0u;
    }
    for (u32 i = 0; i < tn; ++i) {
        const u16 id = mr.m_ov[i];
        if (id != LAND_MASS_IDX_NONE && id <= mr.m_mass_n) {
            ++mass_sz[id];
        }
    }
    for (u16 i = 0; i < mr.m_mass_n; ++i) {
        mass_ord[i] = static_cast<u16>(i + 1u);
    }
    for (u16 a = 0; a < mr.m_mass_n; ++a) {
        for (u16 b = static_cast<u16>(a + 1u); b < mr.m_mass_n; ++b) {
            if (mass_sz[mass_ord[b]] > mass_sz[mass_ord[a]]) {
                const u16 t = mass_ord[a];
                mass_ord[a] = mass_ord[b];
                mass_ord[b] = t;
            }
        }
    }

    WhiteboardMng::init(w, h);
    GenWatershed ws;
    if (!ws.begin(map)) {
        std::printf("GenWatershed::begin failed\n");
        delete[] mass_sz;
        delete[] mass_ord;
        WhiteboardMng::terminate();
        return 1;
    }
    Whiteboard_1B seen("gen_watershed_tester", "seen", 0u);
    if (!seen.ok()) {
        std::printf("seen whiteboard failed\n");
        delete[] mass_sz;
        delete[] mass_ord;
        WhiteboardMng::terminate();
        return 1;
    }
    std::memset(seen.raw(), 0, static_cast<size_t>(WhiteboardMng::tile_n()));

    u16 sys_n = 0;
    u32* q = new u32[tn];
    Whiteboard_1B land_seen("gen_watershed_tester", "land", 0u);
    if (!land_seen.ok()) {
        std::printf("land_seen whiteboard failed\n");
        delete[] mass_sz;
        delete[] mass_ord;
        delete[] q;
        WhiteboardMng::terminate();
        return 1;
    }
    static const i8 k_dx[4] = {0, 1, 0, -1};
    static const i8 k_dy[4] = {-1, 0, 1, 0};
    for (u16 mi = 0; mi < mr.m_mass_n && sys_n < G_SYS_CAP; ++mi) {
        const u16 mass_id = mass_ord[mi];
        u16 seed_x = 0;
        u16 seed_y = 0;
        bool have_seed = false;
        for (u16 y = 0; y < h && !have_seed; ++y) {
            for (u16 x = 0; x < w && !have_seed; ++x) {
                const u32 ti = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                if (mr.m_ov[ti] == mass_id) {
                    seed_x = x;
                    seed_y = y;
                    have_seed = true;
                }
            }
        }
        if (!have_seed) {
            continue;
        }
        std::memset(land_seen.raw(), 0, static_cast<size_t>(WhiteboardMng::tile_n()));
        u32 qh = 0;
        u32 qt = 0;
        const u32 sidx = static_cast<u32>(seed_y) * static_cast<u32>(w) + static_cast<u32>(seed_x);
        q[qt++] = sidx;
        land_seen.wr(seed_x, seed_y, 1u);
        while (qh < qt && sys_n < G_SYS_CAP) {
            const u32 i = q[qh++];
            const u32 py = i / static_cast<u32>(w);
            const u32 px = i - py * static_cast<u32>(w);
            const u16 x = static_cast<u16>(px);
            const u16 y = static_cast<u16>(py);
            if (map.get_river(x, y) != 0u && seen.rd(x, y) == 0u) {
                const auto t0 = std::chrono::steady_clock::now();
                const u32 n = ws.fill_rivers(x, y);
                const auto t1 = std::chrono::steady_clock::now();
                const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
                if (n > 0u) {
                    const Whiteboard_1B& ov = ws.overlay();
                    for (u16 ty = 0; ty < h; ++ty) {
                        for (u16 tx = 0; tx < w; ++tx) {
                            if (ov.rd(tx, ty) != 0u) {
                                seen.wr(tx, ty, 1u);
                            }
                        }
                    }
                    ++sys_n;
                    char ppm[400];
                    std::snprintf(ppm, sizeof(ppm), "%s/river_sys_%03u.ppm", G_OUT_DIR, static_cast<unsigned>(sys_n));
                    if (!write_sys_ppm(ppm, map, ov)) {
                        std::printf("write failed %s\n", ppm);
                        delete[] mass_sz;
                        delete[] mass_ord;
                        delete[] q;
                        WhiteboardMng::terminate();
                        return 1;
                    }
                    std::printf("system %u mass=%u seed=(%u,%u) tiles=%u fill_us=%.2f\n",
                        static_cast<unsigned>(sys_n),
                        static_cast<unsigned>(mass_id),
                        static_cast<unsigned>(x),
                        static_cast<unsigned>(y),
                        static_cast<unsigned>(n),
                        us);
                }
            }
            for (u8 d = 0; d < 4u; ++d) {
                const i32 nx = static_cast<i32>(px) + static_cast<i32>(k_dx[d]);
                const i32 ny = static_cast<i32>(py) + static_cast<i32>(k_dy[d]);
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                const u32 ni = static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux);
                if (mr.m_ov[ni] != mass_id || land_seen.rd(ux, uy) != 0u) {
                    continue;
                }
                land_seen.wr(ux, uy, 1u);
                q[qt++] = ni;
            }
        }
    }
    std::printf("done systems=%u out=%s\n", static_cast<unsigned>(sys_n), G_OUT_DIR);
    delete[] mass_sz;
    delete[] mass_ord;
    delete[] q;
    WhiteboardMng::terminate();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
