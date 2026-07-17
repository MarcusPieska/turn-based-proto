//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "land_mass_index.h"
#include "sector_network.h"
#include "sector_network_router.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-42/terrain.ppm";
static const char* G_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-42/climate.ppm";
static const char* G_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-42/rivers.ppm";
static const char* G_IN_OV = "/home/w/Projects/simple-map-gen/p1-seed-42/overlay.ppm";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void fill_terr (const GameArraySimple& map, u8* out) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            out[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)] = map.get_terrain(x, y);
        }
    }
}

static u16 mass_at (const LandMassIndexRslt& mass, u16 x, u16 y) {
    if (mass.m_ov == nullptr || x >= mass.m_w || y >= mass.m_h) {
        return static_cast<u16>(LAND_MASS_IDX_NONE);
    }
    return mass.m_ov[static_cast<u32>(y) * static_cast<u32>(mass.m_w) + static_cast<u32>(x)];
}

static bool is_nbr (const SectorNetwork& net, u16 a, u16 b) {
    for (u8 bit = 0; bit < 6u; ++bit) {
        if (net.nbr(a, bit) == b) {
            return true;
        }
    }
    return false;
}

static u16 pick_src (const SectorNetwork& net, const LandMassIndexRslt& mass, u16 mass_id) {
    const u16 n = net.sector_n();
    u8* seen = new u8[n];
    u16* q = new u16[n];
    for (u16 i = 0; i < n; ++i) {
        seen[i] = 0;
    }
    u16 best_seed = U16_KEY_NULL;
    u32 best_n = 0;
    for (u16 seed = 0; seed < n; ++seed) {
        if (seen[seed] != 0) {
            continue;
        }
        const Sector* s = net.get(seed);
        if (s->m_mask == 0 || mass_at(mass, s->m_x, s->m_y) != mass_id) {
            seen[seed] = 1;
            continue;
        }
        u32 qh = 0;
        u32 qt = 0;
        u32 comp_n = 0;
        seen[seed] = 1;
        q[qt++] = seed;
        while (qh < qt) {
            const u16 cur = q[qh++];
            ++comp_n;
            for (u8 b = 0; b < 6u; ++b) {
                const u16 j = net.nbr(cur, b);
                if (j == U16_KEY_NULL || j >= n || seen[j] != 0) {
                    continue;
                }
                seen[j] = 1;
                q[qt++] = j;
            }
        }
        if (comp_n > best_n) {
            best_n = comp_n;
            best_seed = seed;
        }
    }
    delete[] q;
    delete[] seen;
    return best_seed;
}

static u32 collect_reach (const SectorNetwork& net, u16 src, u16* out, u32 cap) {
    const u16 n = net.sector_n();
    const u16 hop_cap = static_cast<u16>(SN_PATH_MAX);
    if (src >= n || out == nullptr || cap == 0) {
        return 0;
    }
    u8* seen = new u8[n];
    u16* q = new u16[n];
    u16* dep = new u16[n];
    for (u16 i = 0; i < n; ++i) {
        seen[i] = 0;
    }
    u32 qh = 0;
    u32 qt = 0;
    u32 out_n = 0;
    seen[src] = 1;
    q[qt] = src;
    dep[qt] = 0;
    ++qt;
    while (qh < qt) {
        const u16 cur = q[qh];
        const u16 h = dep[qh];
        ++qh;
        if (out_n < cap) {
            out[out_n++] = cur;
        }
        if (h >= hop_cap) {
            continue;
        }
        for (u8 b = 0; b < 6u; ++b) {
            const u16 j = net.nbr(cur, b);
            if (j == U16_KEY_NULL || j >= n || seen[j] != 0) {
                continue;
            }
            seen[j] = 1;
            q[qt] = j;
            dep[qt] = static_cast<u16>(h + 1u);
            ++qt;
        }
    }
    delete[] dep;
    delete[] q;
    delete[] seen;
    return out_n;
}

static bool walk_route (const SectorNetworkRouter& router, const SectorNetwork& net, u16 src, u16 dst,
    u32* steps, f64* find_us) {
    *steps = 0;
    *find_us = 0.0;
    SectorPath path;
    const auto t0 = std::chrono::steady_clock::now();
    const bool found = router.find(src, dst, &path);
    const auto t1 = std::chrono::steady_clock::now();
    *find_us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
    if (!found) {
        return false;
    }
    u16 cur = src;
    for (u8 i = 0; i < path.m_n; ++i) {
        const u16 hop = net.nbr(cur, path.get(i));
        if (hop == U16_KEY_NULL || !is_nbr(net, cur, hop)) {
            return false;
        }
        cur = hop;
        ++(*steps);
    }
    return cur == dst && static_cast<u32>(path.m_n) == *steps;
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
    const LandMassIndexRslt& mr = mass.result();
    std::printf("land_masses=%u largest=%u\n",
        static_cast<u32>(mr.m_mass_n), static_cast<u32>(mr.m_largest_idx));
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
    const u16 src = pick_src(net, mr, mr.m_largest_idx);
    if (src == U16_KEY_NULL) {
        std::printf("no source sector on largest mass\n");
        delete[] terr;
        return -1;
    }
    u16* reach = new u16[net.sector_n()];
    const u32 reach_n = collect_reach(net, src, reach, net.sector_n());
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
        f64 us = 0.0;
        const bool ok = walk_route(router, net, src, dst, &steps, &us);
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
    delete[] terr;
    return (fail_n == 0) ? 0 : -1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
