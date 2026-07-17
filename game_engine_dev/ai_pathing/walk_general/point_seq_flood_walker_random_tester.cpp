//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "land_mass_index.h"
#include "point_seq_flood_walker.h"
#include "sector_network.h"
#include "sector_network_router.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-42/terrain.ppm";
static const char* G_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-42/climate.ppm";
static const char* G_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-42/rivers.ppm";
static const char* G_IN_OV = "/home/w/Projects/simple-map-gen/p1-seed-42/overlay.ppm";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/point-seq-flood-walker-random-test";
static const u32 G_TRIAL_N = 100u;
static const u32 G_STEP_MAX = 200000u;
static const u32 G_PICK_MAX = 10000u;
static const i32 G_MARK_R = 5;
static const u32 G_RAND_SEED = 42u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

struct LandPt {
    u16 x;
    u16 y;
};

static bool ensure_dir (const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

static bool wr_ppm (const char* path, const u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
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

static void put_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    rgb[i * 3u + 0] = r;
    rgb[i * 3u + 1] = g;
    rgb[i * 3u + 2] = b;
}

static void draw_disc (u8* rgb, u16 w, u16 h, u16 cx, u16 cy, i32 rad, u8 r, u8 g, u8 b) {
    const i32 r2 = rad * rad;
    for (i32 dy = -rad; dy <= rad; ++dy) {
        for (i32 dx = -rad; dx <= rad; ++dx) {
            if (dx * dx + dy * dy <= r2) {
                put_px(rgb, w, h, static_cast<i32>(cx) + dx, static_cast<i32>(cy) + dy, r, g, b);
            }
        }
    }
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
    } else if (cls == TERR_VOLCANO[0]) {
        *r = TERR_VOLCANO[1]; *g = TERR_VOLCANO[2]; *b = TERR_VOLCANO[3];
    } else if (cls == TERR_INLAND_SEA[0]) {
        *r = TERR_INLAND_SEA[1]; *g = TERR_INLAND_SEA[2]; *b = TERR_INLAND_SEA[3];
    } else if (cls == TERR_INLAND_LAKE[0]) {
        *r = TERR_INLAND_LAKE[1]; *g = TERR_INLAND_LAKE[2]; *b = TERR_INLAND_LAKE[3];
    }
}

static void fill_terr (const GameArraySimple& map, u8* out) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            out[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)] = map.get_terrain(x, y);
        }
    }
}

static bool walk_ok (u8 terr) {
    if (overlay_is_water_terr(terr)) {
        return false;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_NONE[0]) {
        return false;
    }
    return true;
}

static u16 mass_at (const LandMassIndexRslt& mass, u16 x, u16 y) {
    if (mass.m_ov == nullptr || x >= mass.m_w || y >= mass.m_h) {
        return static_cast<u16>(LAND_MASS_IDX_NONE);
    }
    return mass.m_ov[static_cast<u32>(y) * static_cast<u32>(mass.m_w) + static_cast<u32>(x)];
}

static u32 gather_mass_walk (const u8* terr, u16 w, u16 h, const LandMassIndexRslt& mass, u16 mass_id,
    LandPt* out, u32 cap) {
    u32 n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (mass_at(mass, x, y) != mass_id) {
                continue;
            }
            if (!walk_ok(terr[static_cast<u32>(y) * static_cast<u32>(w) + x])) {
                continue;
            }
            if (n >= cap) {
                return n;
            }
            out[n].x = x;
            out[n].y = y;
            ++n;
        }
    }
    return n;
}

using Steady = std::chrono::steady_clock;

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!ensure_dir(G_OUT_DIR)) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, G_IN_TERR, G_IN_CLIM, G_IN_RIV, G_IN_OV)) {
        std::printf("failed to load map\n");
        return -1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    u8* terr = new u8[map.tile_n()];
    fill_terr(map, terr);
    LandMassIndex mass;
    if (!mass.generate(terr, w, h)) {
        std::printf("failed land mass index\n");
        delete[] terr;
        return -1;
    }
    const LandMassIndexRslt& mr = mass.result();
    if (mr.m_largest_idx == LAND_MASS_IDX_NONE) {
        std::printf("no land mass\n");
        delete[] terr;
        return -1;
    }
    LandPt* pool = new LandPt[mr.m_land_n];
    const u32 pool_n = gather_mass_walk(terr, w, h, mr, mr.m_largest_idx, pool, mr.m_land_n);
    if (pool_n < 2u) {
        std::printf("walkable pool too small\n");
        delete[] pool;
        delete[] terr;
        return -1;
    }
    SectorNetwork net;
    if (!net.begin(w, h, terr)) {
        std::printf("failed sector begin\n");
        delete[] pool;
        delete[] terr;
        return -1;
    }
    SectorNetworkRouter router;
    if (!router.begin(net)) {
        std::printf("failed router begin\n");
        delete[] pool;
        delete[] terr;
        return -1;
    }
    PointSeqFloodWalker walk;
    if (!walk.begin(net, router, terr, w, h)) {
        std::printf("failed walker begin\n");
        delete[] pool;
        delete[] terr;
        return -1;
    }
    u8* rgb = new u8[static_cast<size_t>(map.tile_n()) * 3u];
    u8* base = new u8[static_cast<size_t>(map.tile_n()) * 3u];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 cls = map.get_terrain(x, y);
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            if (overlay_is_water_terr(cls)) {
                r = 0;
                g = 90;
                b = 255;
            } else {
                terr_rgb(cls, &r, &g, &b);
            }
            put_px(base, w, h, x, y, r, g, b);
        }
    }
    u16* px = new u16[G_STEP_MAX + 1u];
    u16* py = new u16[G_STEP_MAX + 1u];
    std::srand(G_RAND_SEED);
    u32 trial = 0;
    u32 picks = 0;
    while (trial < G_TRIAL_N && picks < G_PICK_MAX) {
        ++picks;
        const u32 ia = static_cast<u32>(std::rand()) % pool_n;
        u32 ib = static_cast<u32>(std::rand()) % pool_n;
        if (ib == ia) {
            ib = (ib + 1u) % pool_n;
        }
        const u16 x0 = pool[ia].x;
        const u16 y0 = pool[ia].y;
        const u16 x1 = pool[ib].x;
        const u16 y1 = pool[ib].y;
        if (mass_at(mr, x0, y0) != mass_at(mr, x1, y1)) {
            continue;
        }
        const Steady::time_point tp0 = Steady::now();
        const bool started = walk.start(x0, y0, x1, y1);
        const Steady::time_point tp1 = Steady::now();
        if (!started) {
            continue;
        }
        const f64 path_us = std::chrono::duration<f64, std::micro>(tp1 - tp0).count();
        u32 n = 0;
        px[0] = walk.x();
        py[0] = walk.y();
        f64 sum_us = 0.0;
        while (!walk.done() && n < G_STEP_MAX) {
            const Steady::time_point ts0 = Steady::now();
            const bool moved = walk.step();
            const Steady::time_point ts1 = Steady::now();
            const f64 us = std::chrono::duration<f64, std::micro>(ts1 - ts0).count();
            if (!moved) {
                break;
            }
            sum_us += us;
            ++n;
            px[n] = walk.x();
            py[n] = walk.y();
        }
        const f64 avg_us = (n > 0) ? (sum_us / static_cast<f64>(n)) : 0.0;
        const u32 arrived = (walk.done() && walk.x() == x1 && walk.y() == y1) ? 1u : 0u;
        std::printf("trial %u: path_us=%.2f steps=%u sum_us=%.2f avg_us=%.2f arrived=%u\n",
            trial, path_us, n, sum_us, avg_us, arrived);
        std::fflush(stdout);
        std::memcpy(rgb, base, static_cast<size_t>(map.tile_n()) * 3u);
        for (u32 i = 0; i <= n; ++i) {
            put_px(rgb, w, h, px[i], py[i], 0, 0, 0);
        }
        draw_disc(rgb, w, h, x0, y0, G_MARK_R, 0, 220, 0);
        draw_disc(rgb, w, h, x1, y1, G_MARK_R, 220, 0, 0);
        if (arrived == 0u) {
            draw_disc(rgb, w, h, walk.x(), walk.y(), G_MARK_R, 255, 255, 0);
        }
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%03u_walk.ppm", G_OUT_DIR, trial);
        if (!wr_ppm(path, rgb, w, h)) {
            std::printf("failed to write %s\n", path);
            delete[] py;
            delete[] px;
            delete[] base;
            delete[] rgb;
            delete[] pool;
            delete[] terr;
            return -1;
        }
        ++trial;
    }
    if (trial < G_TRIAL_N) {
        std::printf("only completed %u/%u trials\n", trial, G_TRIAL_N);
        delete[] py;
        delete[] px;
        delete[] base;
        delete[] rgb;
        delete[] pool;
        delete[] terr;
        return -1;
    }
    delete[] py;
    delete[] px;
    delete[] base;
    delete[] rgb;
    delete[] pool;
    delete[] terr;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
