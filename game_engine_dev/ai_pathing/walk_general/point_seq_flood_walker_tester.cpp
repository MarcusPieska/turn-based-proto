//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#include "factory_game_array_simple.h"
#include "game_map_defs.h"
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
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/point-seq-flood-walker-test";
static const char* G_OUT_PPM = "/home/w/Projects/simple-map-gen/point-seq-flood-walker-test/01_walk.ppm";
static const u32 G_STEP_MAX = 100000u;
static const i32 G_SEC_R = 2;
static const i32 G_END_R = 5;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

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

static bool land_ok (u8 terr) {
    if (overlay_is_water_terr(terr)) {
        return false;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_NONE[0]) {
        return false;
    }
    return true;
}

static u32 dist2 (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    return static_cast<u32>(dx * dx + dy * dy);
}

static bool sec_land (const SectorNetwork& net, const u8* terr, u16 w, u16 id) {
    const Sector* s = net.get(id);
    if (s == nullptr || s->m_mask == 0) {
        return false;
    }
    return land_ok(terr[static_cast<u32>(s->m_y) * static_cast<u32>(w) + s->m_x]);
}

static u16 bfs_far (const SectorNetwork& net, const u8* terr, u16 w, u16 seed, u16* q, u16* dep, u8* seen,
    u8 mark, u16 hop_cap) {
    u16 qh = 0;
    u16 qt = 0;
    q[qt] = seed;
    dep[qt] = 0;
    ++qt;
    seen[seed] = mark;
    u16 far = seed;
    u16 best_h = 0;
    u32 best_d = 0;
    const Sector* ss = net.get(seed);
    while (qh < qt) {
        const u16 cur = q[qh];
        const u16 h = dep[qh];
        ++qh;
        if (sec_land(net, terr, w, cur)) {
            const Sector* c = net.get(cur);
            const u32 d = dist2(ss->m_x, ss->m_y, c->m_x, c->m_y);
            if (h > best_h || (h == best_h && d > best_d)) {
                best_h = h;
                best_d = d;
                far = cur;
            }
        }
        if (h >= hop_cap) {
            continue;
        }
        for (u8 bit = 0; bit < 6; ++bit) {
            const u16 nxt = net.nbr(cur, bit);
            if (nxt == U16_KEY_NULL || seen[nxt] == mark) {
                continue;
            }
            seen[nxt] = mark;
            q[qt] = nxt;
            dep[qt] = static_cast<u16>(h + 1u);
            ++qt;
        }
    }
    return far;
}

static bool pick_land_pair (const SectorNetwork& net, const u8* terr, u16 w, u16 h, u16* out_src,
    u16* out_dst) {
    const u16 n = net.sector_n();
    const u16 mx = static_cast<u16>(w / 2u);
    const u16 my = static_cast<u16>(h / 2u);
    const u16 cap = static_cast<u16>(SN_PATH_MAX);
    u16 seed = U16_KEY_NULL;
    u32 best_c = 0xffffffffu;
    for (u16 i = 0; i < n; ++i) {
        if (!sec_land(net, terr, w, i)) {
            continue;
        }
        const Sector* s = net.get(i);
        const u32 d = dist2(s->m_x, s->m_y, mx, my);
        if (d < best_c) {
            best_c = d;
            seed = i;
        }
    }
    if (seed == U16_KEY_NULL) {
        return false;
    }
    u16* q = new u16[n];
    u16* dep = new u16[n];
    u8* seen = new u8[n];
    for (u16 i = 0; i < n; ++i) {
        seen[i] = 0;
    }
    const u16 a = bfs_far(net, terr, w, seed, q, dep, seen, 1, cap);
    const u16 b = bfs_far(net, terr, w, a, q, dep, seen, 2, cap);
    delete[] seen;
    delete[] dep;
    delete[] q;
    if (a == U16_KEY_NULL || b == U16_KEY_NULL || a == b) {
        return false;
    }
    if (!sec_land(net, terr, w, a) || !sec_land(net, terr, w, b)) {
        return false;
    }
    *out_src = a;
    *out_dst = b;
    return true;
}

using Steady = std::chrono::steady_clock;

static void log_ms (const char* label, Steady::time_point t0, Steady::time_point t1) {
    const f64 ms = std::chrono::duration<f64, std::milli>(t1 - t0).count();
    std::printf("%s: %.2f ms\n", label, ms);
    std::fflush(stdout);
}

static void log_begin (const char* label) {
    std::printf("%s...\n", label);
    std::fflush(stdout);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    const Steady::time_point t_all = Steady::now();
    log_begin("ensure_dir");
    Steady::time_point t0 = Steady::now();
    if (!ensure_dir(G_OUT_DIR)) {
        std::printf("failed to ensure output dir\n");
        std::fflush(stdout);
        return -1;
    }
    log_ms("ensure_dir", t0, Steady::now());

    log_begin("load_map");
    t0 = Steady::now();
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, G_IN_TERR, G_IN_CLIM, G_IN_RIV, G_IN_OV)) {
        std::printf("failed to load map\n");
        std::fflush(stdout);
        return -1;
    }
    log_ms("load_map", t0, Steady::now());

    const u16 w = map.width();
    const u16 h = map.height();
    std::printf("map %u x %u\n", static_cast<u32>(w), static_cast<u32>(h));
    std::fflush(stdout);

    log_begin("fill_terr");
    t0 = Steady::now();
    u8* terr = new u8[map.tile_n()];
    fill_terr(map, terr);
    log_ms("fill_terr", t0, Steady::now());

    log_begin("sector_network.begin");
    t0 = Steady::now();
    SectorNetwork net;
    if (!net.begin(w, h, terr)) {
        std::printf("failed sector begin\n");
        std::fflush(stdout);
        delete[] terr;
        return -1;
    }
    log_ms("sector_network.begin", t0, Steady::now());
    std::printf("sectors=%u\n", static_cast<u32>(net.sector_n()));
    std::fflush(stdout);

    log_begin("router.begin");
    t0 = Steady::now();
    SectorNetworkRouter router;
    if (!router.begin(net)) {
        std::printf("failed router begin\n");
        std::fflush(stdout);
        delete[] terr;
        return -1;
    }
    log_ms("router.begin", t0, Steady::now());

    log_begin("walker.begin");
    t0 = Steady::now();
    PointSeqFloodWalker walk;
    if (!walk.begin(net, router, terr, w, h)) {
        std::printf("failed walker begin\n");
        std::fflush(stdout);
        delete[] terr;
        return -1;
    }
    log_ms("walker.begin", t0, Steady::now());

    log_begin("select_pair");
    t0 = Steady::now();
    u16 src = U16_KEY_NULL;
    u16 dst = U16_KEY_NULL;
    if (!pick_land_pair(net, terr, w, h, &src, &dst)) {
        std::printf("no land pair\n");
        std::fflush(stdout);
        delete[] terr;
        return -1;
    }
    log_ms("select_pair", t0, Steady::now());
    std::printf("pair sectors %u -> %u (land centers)\n", static_cast<u32>(src), static_cast<u32>(dst));
    std::fflush(stdout);
    const Sector* sa = net.get(src);
    const Sector* sb = net.get(dst);

    log_begin("router.find hops");
    t0 = Steady::now();
    SectorPath hops;
    if (!router.find(src, dst, &hops)) {
        std::printf("path rebuild failed\n");
        std::fflush(stdout);
        delete[] terr;
        return -1;
    }
    log_ms("router.find hops", t0, Steady::now());

    std::printf("walk %u,%u -> %u,%u (sectors %u -> %u hops=%u)\n",
        static_cast<u32>(sa->m_x), static_cast<u32>(sa->m_y),
        static_cast<u32>(sb->m_x), static_cast<u32>(sb->m_y),
        static_cast<u32>(src), static_cast<u32>(dst), static_cast<u32>(hops.m_n));
    std::fflush(stdout);

    log_begin("alloc trail buffers");
    t0 = Steady::now();
    u16* px = new u16[G_STEP_MAX + 1u];
    u16* py = new u16[G_STEP_MAX + 1u];
    f64* tus = new f64[G_STEP_MAX];
    log_ms("alloc trail buffers", t0, Steady::now());

    log_begin("walker.start");
    t0 = Steady::now();
    if (!walk.start(sa->m_x, sa->m_y, sb->m_x, sb->m_y)) {
        std::printf("start failed\n");
        std::fflush(stdout);
        delete[] tus;
        delete[] py;
        delete[] px;
        delete[] terr;
        return -1;
    }
    log_ms("walker.start", t0, Steady::now());

    log_begin("walk steps");
    const Steady::time_point t_walk = Steady::now();
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
            if (!walk.done()) {
                std::printf("stuck at %u,%u after %u steps\n", static_cast<u32>(walk.x()), static_cast<u32>(walk.y()), n);
                std::fflush(stdout);
                delete[] tus;
                delete[] py;
                delete[] px;
                delete[] terr;
                return -1;
            }
            break;
        }
        tus[n] = us;
        sum_us += us;
        ++n;
        px[n] = walk.x();
        py[n] = walk.y();
        std::printf("step %u: %.2f us  pos=%u,%u\n", n, us, static_cast<u32>(walk.x()), static_cast<u32>(walk.y()));
        std::fflush(stdout);
    }
    log_ms("walk steps", t_walk, Steady::now());
    const f64 avg_us = (n > 0) ? (sum_us / static_cast<f64>(n)) : 0.0;
    std::printf("steps=%u sum_us=%.2f avg_us=%.2f arrived=%u pos=%u,%u\n",
        n, sum_us, avg_us, static_cast<u32>(walk.done()),
        static_cast<u32>(walk.x()), static_cast<u32>(walk.y()));
    std::fflush(stdout);

    log_begin("draw map rgb");
    t0 = Steady::now();
    u8* rgb = new u8[static_cast<size_t>(map.tile_n()) * 3u];
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
            put_px(rgb, w, h, x, y, r, g, b);
        }
    }
    log_ms("draw map rgb", t0, Steady::now());

    log_begin("draw path + markers");
    t0 = Steady::now();
    for (u32 i = 0; i <= n; ++i) {
        put_px(rgb, w, h, px[i], py[i], 0, 0, 0);
    }
    {
        u16 sid = src;
        draw_disc(rgb, w, h, net.get(sid)->m_x, net.get(sid)->m_y, G_SEC_R, 255, 0, 0);
        for (u8 i = 0; i < hops.m_n; ++i) {
            sid = net.nbr(sid, hops.get(i));
            if (sid == U16_KEY_NULL) {
                break;
            }
            const Sector* s = net.get(sid);
            draw_disc(rgb, w, h, s->m_x, s->m_y, G_SEC_R, 255, 0, 0);
        }
    }
    draw_disc(rgb, w, h, sa->m_x, sa->m_y, G_END_R, 255, 0, 0);
    draw_disc(rgb, w, h, sb->m_x, sb->m_y, G_END_R, 255, 0, 0);
    log_ms("draw path + markers", t0, Steady::now());

    log_begin("write ppm");
    t0 = Steady::now();
    if (!wr_ppm(G_OUT_PPM, rgb, w, h)) {
        std::printf("failed to write %s\n", G_OUT_PPM);
        std::fflush(stdout);
        delete[] rgb;
        delete[] tus;
        delete[] py;
        delete[] px;
        delete[] terr;
        return -1;
    }
    log_ms("write ppm", t0, Steady::now());
    std::printf("wrote %s\n", G_OUT_PPM);
    std::fflush(stdout);

    log_ms("total", t_all, Steady::now());
    delete[] rgb;
    delete[] tus;
    delete[] py;
    delete[] px;
    delete[] terr;
    return walk.done() ? 0 : -1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
