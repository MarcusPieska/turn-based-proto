//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#include "factory_game_array_simple.h"
#include "city.h"
#include "city_array.h"
#include "city_network.h"
#include "game_map_defs.h"
#include "land_mass_index.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

struct PlacePos {
    u16 m_x;
    u16 m_y;
};

static const char* G_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-42/terrain.ppm";
static const char* G_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-42/climate.ppm";
static const char* G_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-42/rivers.ppm";
static const char* G_IN_OV = "/home/w/Projects/simple-map-gen/p1-seed-42/overlay.ppm";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-network-test";
static const char* G_OUT_PPM = "/home/w/Projects/simple-map-gen/city-network-test/01_cities.ppm";
static const char* G_OUT_MASS = "/home/w/Projects/simple-map-gen/city-network-test/02_land_mass.ppm";
static const char* G_OUT_NET = "/home/w/Projects/simple-map-gen/city-network-test/03_network.ppm";

static const u32 G_CITY_TGT = 10000u;
static const u32 G_CITY_CAP = 65535u;
static const u32 G_SEED = 42u;
static const f64 G_NOISE_FRAC = 0.4;

static const u8 G_MASS_PAL[10][3] = {
    {255, 64, 64},
    {64, 255, 64},
    {64, 128, 255},
    {255, 220, 64},
    {255, 64, 220},
    {64, 255, 220},
    {255, 140, 64},
    {180, 64, 255},
    {140, 255, 64},
    {64, 200, 255}
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool ensure_dir (cstr path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

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
    if (terr == TERR_MOUNTAINS[0]) {
        return false;
    }
    if (terr == TERR_NONE[0]) {
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

static bool wr_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
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

static bool save_city_map (const GameArraySimple& map, const PlacePos* cities, u32 city_n, cstr path) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = map.tile_n();
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
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
    for (u32 c = 0; c < city_n; ++c) {
        const u32 i = static_cast<u32>(cities[c].m_y) * static_cast<u32>(w) + static_cast<u32>(cities[c].m_x);
        rgb[i * 3u + 0] = 255;
        rgb[i * 3u + 1] = 0;
        rgb[i * 3u + 2] = 0;
    }
    const bool ok = wr_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static void fill_terr (const GameArraySimple& map, u8* out) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            out[i] = map.get_terrain(x, y);
        }
    }
}

static bool save_mass_map (const LandMassIndexRslt& r, cstr path) {
    if (r.m_ov == nullptr || r.m_w == 0 || r.m_h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(r.m_w) * static_cast<u32>(r.m_h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u32 i = 0; i < n; ++i) {
        const u16 idx = r.m_ov[i];
        if (idx == static_cast<u16>(LAND_MASS_IDX_NONE)) {
            rgb[i * 3u + 0] = 0;
            rgb[i * 3u + 1] = 0;
            rgb[i * 3u + 2] = 0;
            continue;
        }
        const u32 pi = static_cast<u32>((idx - 1u) % 10u);
        rgb[i * 3u + 0] = G_MASS_PAL[pi][0];
        rgb[i * 3u + 1] = G_MASS_PAL[pi][1];
        rgb[i * 3u + 2] = G_MASS_PAL[pi][2];
    }
    const bool ok = wr_ppm(path, rgb, r.m_w, r.m_h);
    delete[] rgb;
    return ok;
}

static void put_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
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

static void draw_link (u8* rgb, u16 w, u16 h, const CityArray& cities, u16 a, u16 b) {
    if (b == U16_KEY_NULL) {
        return;
    }
    const City* ca = cities.get_city(a);
    const City* cb = cities.get_city(b);
    draw_line(rgb, w, h, ca->get_x(), ca->get_y(), cb->get_x(), cb->get_y());
}

static bool save_net_map (const GameArraySimple& map, const CityNetwork& pathing, cstr path) {
    const CityArray* cities = pathing.cities();
    const u16 city_n = pathing.city_n();
    if (cities == nullptr || city_n == 0) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = map.tile_n();
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
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
    for (u16 i = 0; i < city_n; ++i) {
        const CityNetLinks& L = cities->get_city(i)->links();
        draw_link(rgb, w, h, *cities, i, L.m_ne);
        draw_link(rgb, w, h, *cities, i, L.m_nw);
        draw_link(rgb, w, h, *cities, i, L.m_se);
        draw_link(rgb, w, h, *cities, i, L.m_sw);
    }
    for (u16 c = 0; c < city_n; ++c) {
        const City* city = cities->get_city(c);
        put_px(rgb, w, h, city->get_x(), city->get_y(), 255, 0, 0);
    }
    const bool ok = wr_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}


static u32 count_links (const CityNetwork& pathing) {
    const CityArray* cities = pathing.cities();
    u32 filled = 0;
    for (u16 i = 0; i < pathing.city_n(); ++i) {
        const CityNetLinks& L = cities->get_city(i)->links();
        if (L.m_ne != U16_KEY_NULL) {
            ++filled;
        }
        if (L.m_nw != U16_KEY_NULL) {
            ++filled;
        }
        if (L.m_se != U16_KEY_NULL) {
            ++filled;
        }
        if (L.m_sw != U16_KEY_NULL) {
            ++filled;
        }
    }
    return filled;
}

static bool fill_net (
    CityNetwork* net,
    CityArray* cities,
    GameArraySimple& map,
    const PlacePos* pool,
    u16 city_n,
    f64* out_total_us) {
    for (u16 i = 0; i < city_n; ++i) {
        map.set_tile_add(pool[i].m_x, pool[i].m_y, U16_KEY_NULL, 0);
    }
    if (!net->begin(*cities, map)) {
        return false;
    }
    const auto t0 = std::chrono::steady_clock::now();
    for (u16 i = 0; i < city_n; ++i) {
        const u16 idx = cities->get_next_new_city_idx();
        if (idx == U16_KEY_NULL) {
            return false;
        }
        cities->get_city(idx)->init(0, pool[i].m_x, pool[i].m_y);
        map.set_tile_add(pool[i].m_x, pool[i].m_y, idx, CN_ADD_TYP_CITY);
        if (!net->add(idx)) {
            return false;
        }
    }
    const auto t1 = std::chrono::steady_clock::now();
    if (out_total_us != nullptr) {
        *out_total_us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
    }
    return net->is_valid();
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
    LandMassIndex mass;
    if (!mass.generate(terr, map.width(), map.height()) || !mass.is_valid()) {
        std::printf("failed to build land mass index\n");
        delete[] terr;
        return -1;
    }
    const LandMassIndexRslt& mr = mass.result();
    std::printf("land_masses=%u land_tiles=%u largest=%u\n",
        static_cast<u32>(mr.m_mass_n),
        mr.m_land_n,
        static_cast<u32>(mr.m_largest_idx));
    if (!save_mass_map(mr, G_OUT_MASS)) {
        std::printf("failed to save: %s\n", G_OUT_MASS);
        delete[] terr;
        return -1;
    }
    std::printf("saved: %s\n", G_OUT_MASS);
    PlacePos* pool = new PlacePos[G_CITY_CAP];
    const u32 city_n_u = place_cities(map, pool, G_CITY_CAP);
    if (city_n_u == 0) {
        std::printf("no cities placed\n");
        delete[] pool;
        delete[] terr;
        return -1;
    }
    const u16 city_n = static_cast<u16>(city_n_u);
    if (!save_city_map(map, pool, city_n, G_OUT_PPM)) {
        std::printf("failed to save: %s\n", G_OUT_PPM);
        delete[] pool;
        delete[] terr;
        return -1;
    }
    std::printf("saved: %s (%u cities)\n", G_OUT_PPM, static_cast<u32>(city_n));
    CityArray cities;
    CityNetwork net;
    f64 total_us = 0.0;
    if (!fill_net(&net, &cities, map, pool, city_n, &total_us)) {
        std::printf("failed to build network\n");
        delete[] pool;
        delete[] terr;
        return -1;
    }
    const u32 filled = count_links(net);
    std::printf("network links=%u / %u total_build_us=%.2f flood=%u\n",
        filled, static_cast<u32>(city_n) * 4u, total_us, net.flood_n());
    if (!save_net_map(map, net, G_OUT_NET)) {
        std::printf("failed to save: %s\n", G_OUT_NET);
        delete[] pool;
        delete[] terr;
        return -1;
    }
    std::printf("saved: %s\n", G_OUT_NET);
    delete[] pool;
    delete[] terr;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
