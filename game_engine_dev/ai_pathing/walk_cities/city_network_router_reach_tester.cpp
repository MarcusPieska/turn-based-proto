//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "city.h"
#include "city_array.h"
#include "city_network.h"
#include "city_network_router.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "land_mass_index.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-42/terrain.ppm";
static const char* G_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-42/climate.ppm";
static const char* G_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-42/rivers.ppm";
static const char* G_IN_OV = "/home/w/Projects/simple-map-gen/p1-seed-42/overlay.ppm";

static const u32 G_CITY_TGT = 10000u;
static const u32 G_CITY_CAP = 65535u;
static const u32 G_SEED = 42u;
static const u32 G_SHUF_SEED = 99u;
static const f64 G_NOISE_FRAC = 0.4;

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

static u16 mass_at (const LandMassIndexRslt& mass, u16 x, u16 y) {
    if (mass.m_ov == nullptr || x >= mass.m_w || y >= mass.m_h) {
        return static_cast<u16>(LAND_MASS_IDX_NONE);
    }
    return mass.m_ov[static_cast<u32>(y) * static_cast<u32>(mass.m_w) + static_cast<u32>(x)];
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

static u32 count_mass_land (const GameArraySimple& map, const LandMassIndexRslt& mass, u16 mass_id) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (mass_at(mass, x, y) != mass_id) {
                continue;
            }
            if (is_city_ok(map.get_terrain(x, y))) {
                ++n;
            }
        }
    }
    return n;
}

static u32 place_cities (const GameArraySimple& map, const LandMassIndexRslt& mass, u16 mass_id, PlacePos* out,
    u32 cap) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 land_n = count_mass_land(map, mass, mass_id);
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
            if (mass_at(mass, ux, uy) != mass_id) {
                continue;
            }
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
    std::printf("mass_id=%u mass_land=%u step=%u tgt=%u placed=%u\n",
        static_cast<u32>(mass_id), land_n, step, tgt, n);
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

static void clear_marks (GameArraySimple& map, PlacePos* pool, u32 pool_n) {
    for (u32 i = 0; i < pool_n; ++i) {
        map.set_tile_add(pool[i].m_x, pool[i].m_y, U16_KEY_NULL, 0);
    }
}

static bool is_nbr (const CityNetwork& net, u16 a, u16 b) {
    if (a >= net.city_n() || b >= net.city_n()) {
        return false;
    }
    const City* c = net.cities()->get_city(a);
    for (u8 d = 0; d < 4u; ++d) {
        if (c->get_conn_city(d) == b) {
            return true;
        }
    }
    return false;
}

static u32 collect_reach (const CityNetwork& net, u16 src, u16* out, u32 cap) {
    const u16 n = net.city_n();
    if (src >= n || out == nullptr || cap == 0) {
        return 0;
    }
    u8* seen = new u8[n];
    u16* q = new u16[n];
    for (u16 i = 0; i < n; ++i) {
        seen[i] = 0;
    }
    u32 qh = 0;
    u32 qt = 0;
    u32 out_n = 0;
    seen[src] = 1;
    q[qt++] = src;
    while (qh < qt) {
        const u16 cur = q[qh++];
        if (out_n < cap) {
            out[out_n++] = cur;
        }
        const City* cc = net.cities()->get_city(cur);
        for (u8 s = 0; s < 4u; ++s) {
            const u16 j = cc->get_conn_city(s);
            if (j == U16_KEY_NULL || j >= n || seen[j] != 0) {
                continue;
            }
            seen[j] = 1;
            q[qt++] = j;
        }
    }
    delete[] q;
    delete[] seen;
    return out_n;
}

static bool walk_route (const CityNetworkRouter& router, const CityNetwork& net, u16 src, u16 dst, u32* steps) {
    *steps = 0;
    if (src == dst) {
        return true;
    }
    const u32 hop_max = static_cast<u32>(net.city_n()) + 1u;
    u16 cur = src;
    while (cur != dst) {
        const u16 hop = router.next_hop(cur, dst);
        if (hop == U16_KEY_NULL || hop == cur) {
            return false;
        }
        if (!is_nbr(net, cur, hop)) {
            return false;
        }
        cur = hop;
        ++(*steps);
        if (*steps > hop_max) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
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
    if (!mass.generate(terr, map.width(), map.height())) {
        std::printf("failed to generate land mass index\n");
        delete[] terr;
        return -1;
    }
    delete[] terr;
    const LandMassIndexRslt& mr = mass.result();
    std::printf("land_masses=%u largest=%u\n",
        static_cast<u32>(mr.m_mass_n), static_cast<u32>(mr.m_largest_idx));
    if (mr.m_largest_idx == static_cast<u16>(LAND_MASS_IDX_NONE)) {
        std::printf("no land mass\n");
        return -1;
    }
    PlacePos* pool = new PlacePos[G_CITY_CAP];
    const u32 pool_n = place_cities(map, mr, mr.m_largest_idx, pool, G_CITY_CAP);
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
    const u16 src = 0;
    u16* reach = new u16[net.city_n()];
    const u32 reach_n = collect_reach(net, src, reach, net.city_n());
    std::printf("source=%u reachable=%u\n", static_cast<u32>(src), reach_n);
    u32 ok_n = 0;
    u32 fail_n = 0;
    u32 sum_steps = 0;
    f64 sum_us = 0.0;
    f64 min_us = 1.0e300;
    f64 max_us = 0.0;
    for (u32 i = 0; i < reach_n; ++i) {
        const u16 dst = reach[i];
        if (dst == src) {
            continue;
        }
        u32 steps = 0;
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = walk_route(router, net, src, dst, &steps);
        const auto t1 = std::chrono::steady_clock::now();
        const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
        sum_us += us;
        if (us < min_us) {
            min_us = us;
        }
        if (us > max_us) {
            max_us = us;
        }
        if (ok) {
            ++ok_n;
            sum_steps += steps;
            std::printf("src=%u dst=%u steps=%u time_us=%.2f ok\n",
                static_cast<u32>(src), static_cast<u32>(dst), steps, us);
        } else {
            ++fail_n;
            std::printf("src=%u dst=%u steps=%u time_us=%.2f FAIL\n",
                static_cast<u32>(src), static_cast<u32>(dst), steps, us);
        }
    }
    const u32 route_n = ok_n + fail_n;
    std::printf("summary routes=%u ok=%u fail=%u\n", route_n, ok_n, fail_n);
    if (route_n > 0) {
        std::printf("summary avg_us=%.2f min_us=%.2f max_us=%.2f\n",
            sum_us / static_cast<f64>(route_n), min_us, max_us);
    }
    if (ok_n > 0) {
        std::printf("summary avg_steps=%.2f\n",
            static_cast<f64>(sum_steps) / static_cast<f64>(ok_n));
    }
    delete[] reach;
    clear_marks(map, pool, pool_n);
    delete[] pool;
    return (fail_n == 0) ? 0 : -1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
