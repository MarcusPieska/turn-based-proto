//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>

#include "city.h"
#include "city_array.h"
#include "city_network.h"
#include "city_network_router.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-42/terrain.ppm";
static const char* G_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-42/climate.ppm";
static const char* G_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-42/rivers.ppm";
static const char* G_IN_OV = "/home/w/Projects/simple-map-gen/p1-seed-42/overlay.ppm";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-network-router-test";

static const u32 G_CITY_TGT = 10000u;
static const u32 G_CITY_CAP = 65535u;
static const u32 G_SEED = 42u;
static const u32 G_SHUF_SEED = 99u;
static const f64 G_NOISE_FRAC = 0.4;
static const i32 G_BLUE_R = 3;
static const u32 G_HOP_MAX = 256u;

struct PlacePos {
    u16 m_x;
    u16 m_y;
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 rng_next (u32* state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static i32 rng_i32 (u32* state, i32 lo, i32 hi) {
    if (hi <= lo) {
        return lo;
    }
    const u32 span = static_cast<u32>(hi - lo + 1);
    return lo + static_cast<i32>(rng_next(state) % span);
}

static bool is_city_ok (u8 terr) {
    if (overlay_is_water_terr(terr)) {
        return false;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_NONE[0]) {
        return false;
    }
    return true;
}

static u32 rng_pct (u32* state) {
    return rng_next(state) % 101u;
}

static bool keep_city (const GameArraySimple& map, u16 x, u16 y, u32* rng) {
    const u8 terr = map.get_terrain(x, y);
    const u8 clim = map.get_climate(x, y);
    const u8 riv = map.get_river(x, y);
    if (clim == CLIMATE_DESERT && riv == 0) {
        return false;
    }
    if (terr == TERR_HILLS[0] && rng_pct(rng) > 50u) {
        return false;
    }
    if (clim == CLIMATE_PLAINS && rng_pct(rng) > 50u) {
        return false;
    }
    return true;
}

static u32 count_land (const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (is_city_ok(map.get_terrain(x, y))) {
                ++n;
            }
        }
    }
    return n;
}

static u32 place_cities (const GameArraySimple& map, PlacePos* out, u32 cap) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 land_n = count_land(map);
    u32 tgt = G_CITY_TGT;
    if (tgt > cap) {
        tgt = cap;
    }
    if (tgt > land_n) {
        tgt = land_n;
    }
    if (tgt == 0) {
        return 0;
    }
    u32 step = static_cast<u32>(std::sqrt(static_cast<f64>(land_n) / static_cast<f64>(tgt)));
    if (step < 1u) {
        step = 1u;
    }
    const i32 noise_max = static_cast<i32>(static_cast<f64>(step) * G_NOISE_FRAC);
    u8* used = new u8[map.tile_n()];
    for (u32 i = 0; i < map.tile_n(); ++i) {
        used[i] = 0;
    }
    u32 rng = G_SEED;
    u32 n = 0;
    for (u32 gy = 0; gy * step < static_cast<u32>(h) && n < tgt; ++gy) {
        for (u32 gx = 0; gx * step < static_cast<u32>(w) && n < tgt; ++gx) {
            const i32 bx = static_cast<i32>(gx * step + step / 2u);
            const i32 by = static_cast<i32>(gy * step + step / 2u);
            i32 x = bx + rng_i32(&rng, -noise_max, noise_max);
            i32 y = by + rng_i32(&rng, -noise_max, noise_max);
            if (x < 0) {
                x = 0;
            }
            if (y < 0) {
                y = 0;
            }
            if (x >= static_cast<i32>(w)) {
                x = static_cast<i32>(w) - 1;
            }
            if (y >= static_cast<i32>(h)) {
                y = static_cast<i32>(h) - 1;
            }
            const u16 ux = static_cast<u16>(x);
            const u16 uy = static_cast<u16>(y);
            const u32 ti = static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux);
            if (used[ti] != 0) {
                continue;
            }
            if (!is_city_ok(map.get_terrain(ux, uy))) {
                continue;
            }
            if (!keep_city(map, ux, uy, &rng)) {
                continue;
            }
            used[ti] = 1;
            out[n].m_x = ux;
            out[n].m_y = uy;
            ++n;
        }
    }
    delete[] used;
    std::printf("land_n=%u step=%u tgt=%u placed=%u\n", land_n, step, tgt, n);
    return n;
}

static void shuffle (PlacePos* arr, u32 n, u32 seed) {
    u32 rng = seed;
    for (u32 i = n; i > 1u; --i) {
        const u32 j = rng_next(&rng) % i;
        const PlacePos tmp = arr[i - 1u];
        arr[i - 1u] = arr[j];
        arr[j] = tmp;
    }
}

static bool ensure_dir (const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    return mkdir(path, 0755) == 0 || errno == EEXIST;
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

static void fill_base (const GameArraySimple& map, u8* rgb) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(map.get_terrain(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0) {
                r = 0;
                g = 180;
                b = 255;
            }
            rgb[i * 3u + 0] = r;
            rgb[i * 3u + 1] = g;
            rgb[i * 3u + 2] = b;
        }
    }
}

static void draw_net (u8* rgb, u16 w, u16 h, const CityNetwork& net) {
    const CityArray* cities = net.cities();
    const u16 city_n = net.city_n();
    for (u16 i = 0; i < city_n; ++i) {
        const City* a = cities->get_city(i);
        for (u8 s = 0; s < 4u; ++s) {
            const u16 j = a->get_conn_city(s);
            if (j == U16_KEY_NULL || j < i) {
                continue;
            }
            const City* b = cities->get_city(j);
            draw_line(rgb, w, h, a->get_x(), a->get_y(), b->get_x(), b->get_y());
        }
    }
    for (u16 c = 0; c < city_n; ++c) {
        const City* city = cities->get_city(c);
        put_px(rgb, w, h, city->get_x(), city->get_y(), 255, 0, 0);
    }
}

static void mark_blue (u8* rgb, u16 w, u16 h, const CityNetwork& net, u16 id) {
    if (id == U16_KEY_NULL || id >= net.city_n()) {
        return;
    }
    const City* c = net.cities()->get_city(id);
    draw_disc(rgb, w, h, c->get_x(), c->get_y(), G_BLUE_R, 0, 80, 255);
}

static bool save_hop (const GameArraySimple& map, const CityNetwork& net, u16 a, u16 b, u16 hop,
    u32 frame, const char* dir) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = map.tile_n();
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
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

static void pick_ends (const CityNetwork& net, u16* out_a, u16* out_b) {
    const CityArray* cities = net.cities();
    const u16 n = net.city_n();
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
        u32 qh = 0;
        u32 qt = 0;
        u16* comp = new u16[n];
        u32 comp_n = 0;
        seen[seed] = 1;
        q[qt++] = seed;
        while (qh < qt) {
            const u16 cur = q[qh++];
            comp[comp_n++] = cur;
            const City* cc = cities->get_city(cur);
            for (u8 s = 0; s < 4u; ++s) {
                const u16 j = cc->get_conn_city(s);
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
                const City* ca = cities->get_city(comp[i]);
                for (u32 j = i + stride; j < comp_n; j += stride) {
                    const City* cb = cities->get_city(comp[j]);
                    const u32 d = dist2(ca->get_x(), ca->get_y(), cb->get_x(), cb->get_y());
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

static void clear_marks (GameArraySimple& map, PlacePos* pool, u32 pool_n) {
    for (u32 i = 0; i < pool_n; ++i) {
        map.set_tile_add(pool[i].m_x, pool[i].m_y, U16_KEY_NULL, 0);
    }
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
    PlacePos* pool = new PlacePos[G_CITY_CAP];
    const u32 pool_n = place_cities(map, pool, G_CITY_CAP);
    if (pool_n == 0) {
        std::printf("no cities placed\n");
        delete[] pool;
        return -1;
    }
    shuffle(pool, pool_n, G_SHUF_SEED);
    clear_marks(map, pool, pool_n);
    CityArray cities;
    CityNetwork net;
    if (!net.begin(cities, map)) {
        std::printf("failed to begin network\n");
        delete[] pool;
        return -1;
    }
    for (u32 i = 0; i < pool_n; ++i) {
        const u16 idx = cities.get_next_new_city_idx();
        if (idx == U16_KEY_NULL) {
            std::printf("failed to alloc city %u\n", i);
            delete[] pool;
            return -1;
        }
        cities.get_city(idx)->init(0, pool[i].m_x, pool[i].m_y);
        map.set_tile_add(pool[i].m_x, pool[i].m_y, idx, BUILD_ADD_CITY);
        if (!net.add(idx)) {
            std::printf("failed to add city %u\n", i);
            delete[] pool;
            return -1;
        }
    }
    std::printf("network cities=%u\n", static_cast<u32>(net.city_n()));
    CityNetworkRouter router;
    if (!router.begin(net)) {
        std::printf("failed to begin router\n");
        delete[] pool;
        return -1;
    }
    u16 src = 0;
    u16 dst = 0;
    pick_ends(net, &src, &dst);
    std::printf("route %u -> %u\n", static_cast<u32>(src), static_cast<u32>(dst));
    u8* seen = new u8[net.city_n()];
    for (u16 i = 0; i < net.city_n(); ++i) {
        seen[i] = 0;
    }
    u16 cur = src;
    seen[cur] = 1;
    u32 frame = 1;
    f64 sum_us = 0.0;
    u32 hop_n = 0;
    while (cur != dst && hop_n < G_HOP_MAX) {
        const auto t0 = std::chrono::steady_clock::now();
        const u16 hop = router.next_hop(cur, dst);
        const auto t1 = std::chrono::steady_clock::now();
        const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
        sum_us += us;
        ++hop_n;
        std::printf("next_hop %u -> %u => %u  %.2f us\n",
            static_cast<u32>(cur), static_cast<u32>(dst), static_cast<u32>(hop), us);
        if (!save_hop(map, net, src, dst, hop, frame, G_OUT_DIR)) {
            std::printf("failed to save hop frame %u\n", frame);
            delete[] seen;
            delete[] pool;
            return -1;
        }
        ++frame;
        if (hop == U16_KEY_NULL || hop == cur) {
            std::printf("stuck at %u\n", static_cast<u32>(cur));
            break;
        }
        if (seen[hop] != 0) {
            std::printf("cycle at %u\n", static_cast<u32>(hop));
            break;
        }
        seen[hop] = 1;
        cur = hop;
    }
    if (hop_n > 0) {
        std::printf("avg_next_hop_us=%.2f\n", sum_us / static_cast<f64>(hop_n));
    }
    std::printf("hops=%u arrived=%u\n", hop_n, static_cast<u32>(cur == dst));
    delete[] seen;
    clear_marks(map, pool, pool_n);
    delete[] pool;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
