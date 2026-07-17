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
#include "sector_network.h"
#include "sector_network_router.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-42/terrain.ppm";
static const char* G_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-42/climate.ppm";
static const char* G_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-42/rivers.ppm";
static const char* G_IN_OV = "/home/w/Projects/simple-map-gen/p1-seed-42/overlay.ppm";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/sector-network-router-test";

static const i32 G_BLUE_R = 3;
static const u32 G_HOP_MAX = 2048u;

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

static void draw_line (u8* rgb, u16 w, u16 h, u16 x0, u16 y0, u16 x1, u16 y1) {
    i32 ax = static_cast<i32>(x0);
    i32 ay = static_cast<i32>(y0);
    const i32 bx = static_cast<i32>(x1);
    const i32 by = static_cast<i32>(y1);
    const i32 dx = (bx > ax) ? (bx - ax) : (ax - bx);
    const i32 dy = (by > ay) ? (by - ay) : (ay - by);
    const i32 sx = (ax < bx) ? 1 : -1;
    const i32 sy = (ay < by) ? 1 : -1;
    i32 err = dx - dy;
    for (;;) {
        put_px(rgb, w, h, ax, ay, 0, 0, 0);
        if (ax == bx && ay == by) {
            break;
        }
        const i32 e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            ax += sx;
        }
        if (e2 < dx) {
            err += dx;
            ay += sy;
        }
    }
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

static void fill_base (const GameArraySimple& map, u8* rgb) {
    const u16 w = map.width();
    const u16 h = map.height();
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
}

static void draw_net (u8* rgb, u16 w, u16 h, const SectorNetwork& net) {
    for (u16 i = 0; i < net.sector_n(); ++i) {
        const Sector* a = net.get(i);
        for (u8 bit = 0; bit < 6u; ++bit) {
            const u16 j = net.nbr(i, bit);
            if (j == U16_KEY_NULL || j < i) {
                continue;
            }
            const Sector* b = net.get(j);
            draw_line(rgb, w, h, a->m_x, a->m_y, b->m_x, b->m_y);
        }
    }
    for (u16 i = 0; i < net.sector_n(); ++i) {
        const Sector* s = net.get(i);
        put_px(rgb, w, h, s->m_x, s->m_y, 255, 0, 0);
    }
}

static void mark_blue (u8* rgb, u16 w, u16 h, const SectorNetwork& net, u16 id) {
    if (id == U16_KEY_NULL || id >= net.sector_n()) {
        return;
    }
    const Sector* s = net.get(id);
    draw_disc(rgb, w, h, s->m_x, s->m_y, G_BLUE_R, 0, 80, 255);
}

static bool save_hop (const GameArraySimple& map, const SectorNetwork& net, u16 a, u16 b, u16 hop,
    u32 frame, const char* dir) {
    const u16 w = map.width();
    const u16 h = map.height();
    u8* rgb = new u8[static_cast<size_t>(map.tile_n()) * 3u];
    fill_base(map, rgb);
    mark_blue(rgb, w, h, net, a);
    mark_blue(rgb, w, h, net, b);
    mark_blue(rgb, w, h, net, hop);
    draw_net(rgb, w, h, net);
    char path[512];
    std::snprintf(path, sizeof(path), "%s/%04u_hop.ppm", dir, frame);
    const bool ok = wr_ppm(path, rgb, w, h);
    delete[] rgb;
    if (ok) {
        std::printf("saved: %s\n", path);
    }
    return ok;
}

static u32 dist2 (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    return static_cast<u32>(dx * dx + dy * dy);
}

static void pick_ends (const SectorNetwork& net, u16* out_a, u16* out_b) {
    const u16 n = net.sector_n();
    u8* seen = new u8[n];
    u16* q = new u16[n];
    for (u16 i = 0; i < n; ++i) {
        seen[i] = 0;
    }
    u16 best_a = 0;
    u16 best_b = 0;
    u32 best_d = 0;
    for (u16 seed = 0; seed < n; ++seed) {
        if (seen[seed] != 0) {
            continue;
        }
        if (net.get(seed)->m_mask == 0) {
            seen[seed] = 1;
            continue;
        }
        u32 qh = 0;
        u32 qt = 0;
        u16* comp = new u16[n];
        u32 comp_n = 0;
        seen[seed] = 1;
        q[qt++] = seed;
        while (qh < qt) {
            const u16 cur = q[qh++];
            comp[comp_n++] = cur;
            for (u8 b = 0; b < 6u; ++b) {
                const u16 j = net.nbr(cur, b);
                if (j == U16_KEY_NULL || j >= n || seen[j] != 0) {
                    continue;
                }
                seen[j] = 1;
                q[qt++] = j;
            }
        }
        if (comp_n >= 2u) {
            const u32 stride = (comp_n > 64u) ? (comp_n / 64u) : 1u;
            for (u32 i = 0; i < comp_n; i += stride) {
                const Sector* ca = net.get(comp[i]);
                for (u32 j = i + stride; j < comp_n; j += stride) {
                    const Sector* cb = net.get(comp[j]);
                    const u32 d = dist2(ca->m_x, ca->m_y, cb->m_x, cb->m_y);
                    if (d > best_d) {
                        best_d = d;
                        best_a = comp[i];
                        best_b = comp[j];
                    }
                }
            }
        }
        delete[] comp;
    }
    delete[] q;
    delete[] seen;
    *out_a = best_a;
    *out_b = best_b;
}

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
        std::printf("failed to load map gen data\n");
        return -1;
    }
    std::printf("loaded %u x %u (%u tiles)\n",
        static_cast<u32>(map.width()),
        static_cast<u32>(map.height()),
        map.tile_n());
    u8* terr = new u8[map.tile_n()];
    fill_terr(map, terr);
    SectorNetwork net;
    if (!net.begin(map.width(), map.height(), terr)) {
        std::printf("failed to begin sector network\n");
        delete[] terr;
        return -1;
    }
    std::printf("sectors=%u\n", static_cast<u32>(net.sector_n()));
    SectorNetworkRouter router;
    if (!router.begin(net)) {
        std::printf("failed to begin router\n");
        delete[] terr;
        return -1;
    }
    u16 src = 0;
    u16 dst = 0;
    pick_ends(net, &src, &dst);
    std::printf("route %u -> %u\n", static_cast<u32>(src), static_cast<u32>(dst));
    SectorPath path;
    const auto t0 = std::chrono::steady_clock::now();
    const bool found = router.find(src, dst, &path);
    const auto t1 = std::chrono::steady_clock::now();
    const f64 find_us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
    std::printf("find steps=%u time_us=%.2f ok=%u\n",
        static_cast<u32>(path.m_n), find_us, static_cast<u32>(found));
    if (!found) {
        std::printf("failed to find path\n");
        delete[] terr;
        return -1;
    }
    u16 cur = src;
    u32 frame = 1;
    for (u8 i = 0; i < path.m_n; ++i) {
        const u16 hop = net.nbr(cur, path.get(i));
        std::printf("hop %u: %u -> %u (bit %u)\n",
            static_cast<u32>(i), static_cast<u32>(cur), static_cast<u32>(hop),
            static_cast<u32>(path.get(i)));
        if (hop == U16_KEY_NULL) {
            std::printf("bad hop bit at %u\n", static_cast<u32>(i));
            delete[] terr;
            return -1;
        }
        if (!save_hop(map, net, src, dst, hop, frame, G_OUT_DIR)) {
            std::printf("failed to save hop frame %u\n", frame);
            delete[] terr;
            return -1;
        }
        ++frame;
        cur = hop;
    }
    std::printf("hops=%u arrived=%u\n", static_cast<u32>(path.m_n), static_cast<u32>(cur == dst));
    delete[] terr;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
