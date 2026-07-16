//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u16 G_N = 100;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];

enum SortMode {
    SM_FOOD = 0,
    SM_PRODUCTION = 1,
    SM_COMMERCE = 2
};

struct TileCand {
    u16 m_x;
    u16 m_y;
    TileYield m_yld;
};

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
    return true;
}

static i32 cmp_cand (const TileCand& a, const TileCand& b, SortMode mode) {
    if (mode == SM_FOOD) {
        if (a.m_yld.m_food != b.m_yld.m_food) {
            return static_cast<i32>(a.m_yld.m_food) - static_cast<i32>(b.m_yld.m_food);
        }
        if (a.m_yld.m_production != b.m_yld.m_production) {
            return static_cast<i32>(a.m_yld.m_production) - static_cast<i32>(b.m_yld.m_production);
        }
        return static_cast<i32>(a.m_yld.m_commerce) - static_cast<i32>(b.m_yld.m_commerce);
    }
    if (mode == SM_PRODUCTION) {
        if (a.m_yld.m_production != b.m_yld.m_production) {
            return static_cast<i32>(a.m_yld.m_production) - static_cast<i32>(b.m_yld.m_production);
        }
        if (a.m_yld.m_food != b.m_yld.m_food) {
            return static_cast<i32>(a.m_yld.m_food) - static_cast<i32>(b.m_yld.m_food);
        }
        return static_cast<i32>(a.m_yld.m_commerce) - static_cast<i32>(b.m_yld.m_commerce);
    }
    if (a.m_yld.m_commerce != b.m_yld.m_commerce) {
        return static_cast<i32>(a.m_yld.m_commerce) - static_cast<i32>(b.m_yld.m_commerce);
    }
    if (a.m_yld.m_food != b.m_yld.m_food) {
        return static_cast<i32>(a.m_yld.m_food) - static_cast<i32>(b.m_yld.m_food);
    }
    return static_cast<i32>(a.m_yld.m_production) - static_cast<i32>(b.m_yld.m_production);
}

static void swap_cand (TileCand& a, TileCand& b) {
    const TileCand t = a;
    a = b;
    b = t;
}

static void sort_ins (std::vector<TileCand>& a, u32 lo, u32 hi, SortMode mode) {
    for (u32 i = lo + 1; i <= hi; ++i) {
        TileCand key = a[i];
        u32 j = i;
        while (j > lo && cmp_cand(a[j - 1], key, mode) < 0) {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = key;
    }
}

static u32 piv_idx (std::vector<TileCand>& a, u32 lo, u32 hi, SortMode mode) {
    const u32 mid = lo + (hi - lo) / 2;
    if (cmp_cand(a[mid], a[lo], mode) < 0) {
        swap_cand(a[mid], a[lo]);
    }
    if (cmp_cand(a[hi], a[lo], mode) < 0) {
        swap_cand(a[hi], a[lo]);
    }
    if (cmp_cand(a[hi], a[mid], mode) < 0) {
        swap_cand(a[hi], a[mid]);
    }
    swap_cand(a[mid], a[hi]);
    return hi;
}

static u32 part_qs (std::vector<TileCand>& a, u32 lo, u32 hi, SortMode mode) {
    const u32 p = piv_idx(a, lo, hi, mode);
    const TileCand piv = a[p];
    u32 i = lo;
    for (u32 j = lo; j < hi; ++j) {
        if (cmp_cand(a[j], piv, mode) > 0) {
            swap_cand(a[i], a[j]);
            ++i;
        }
    }
    swap_cand(a[i], a[hi]);
    return i;
}

static void sort_qs_rec (std::vector<TileCand>& a, u32 lo, u32 hi, SortMode mode) {
    while (lo < hi) {
        if (hi - lo < 12) {
            sort_ins(a, lo, hi, mode);
            return;
        }
        const u32 p = part_qs(a, lo, hi, mode);
        if (p - lo < hi - p) {
            if (lo < p) {
                sort_qs_rec(a, lo, p - 1, mode);
            }
            lo = p + 1;
        } else {
            if (p < hi) {
                sort_qs_rec(a, p + 1, hi, mode);
            }
            if (p == 0) {
                return;
            }
            hi = p - 1;
        }
    }
}

static void sort_qs (std::vector<TileCand>& a, SortMode mode) {
    if (a.size() <= 1) {
        return;
    }
    sort_qs_rec(a, 0, static_cast<u32>(a.size() - 1), mode);
}

static void sort_sel (std::vector<TileCand>& a, SortMode mode) {
    const u32 n = static_cast<u32>(a.size());
    if (n <= 16) {
        sort_ins(a, 0, n - 1, mode);
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        u32 best = i;
        for (u32 j = i + 1; j < n; ++j) {
            const i32 c = cmp_cand(a[j], a[best], mode);
            best = static_cast<u32>(c > 0) * j + static_cast<u32>(c <= 0) * best;
        }
        if (best != i) {
            swap_cand(a[i], a[best]);
        }
    }
}

static u8 pri_key (const TileCand& c, SortMode mode) {
    if (mode == SM_FOOD) {
        return c.m_yld.m_food;
    }
    if (mode == SM_PRODUCTION) {
        return c.m_yld.m_production;
    }
    return c.m_yld.m_commerce;
}

static u8 pri_bkt (u8 key) {
    return key >= 5 ? 5 : key;
}

static void pick_cnt (std::vector<TileCand>& a, u32 pick_n, SortMode mode) {
    const u32 n = static_cast<u32>(a.size());
    if (n == 0 || pick_n == 0) {
        return;
    }
    if (pick_n > n) {
        pick_n = n;
    }
    u32 cnt[6] = {};
    for (u32 i = 0; i < n; ++i) {
        ++cnt[pri_bkt(pri_key(a[i], mode))];
    }
    u32 take[6] = {};
    u32 need = pick_n;
    for (i32 b = 5; b >= 0 && need > 0; --b) {
        const u32 bi = static_cast<u32>(b);
        if (cnt[bi] <= need) {
            take[bi] = cnt[bi];
            need -= cnt[bi];
        } else {
            take[bi] = need;
            need = 0;
        }
    }
    std::vector<TileCand> out;
    out.reserve(pick_n);
    for (i32 b = 5; b >= 0; --b) {
        const u32 bi = static_cast<u32>(b);
        if (take[bi] == 0) {
            continue;
        }
        if (take[bi] < cnt[bi]) {
            std::vector<TileCand> sub;
            sub.reserve(cnt[bi]);
            for (u32 i = 0; i < n; ++i) {
                if (pri_bkt(pri_key(a[i], mode)) == bi) {
                    sub.push_back(a[i]);
                }
            }
            if (!sub.empty()) {
                sort_ins(sub, 0, static_cast<u32>(sub.size() - 1), mode);
            }
            for (u32 i = 0; i < take[bi]; ++i) {
                out.push_back(sub[i]);
            }
            continue;
        }
        u32 rem = take[bi];
        for (u32 i = 0; i < n && rem > 0; ++i) {
            if (pri_bkt(pri_key(a[i], mode)) == bi) {
                out.push_back(a[i]);
                --rem;
            }
        }
    }
    if (pick_n == n) {
        a = out;
        return;
    }
    for (u32 i = 0; i < pick_n; ++i) {
        a[i] = out[i];
    }
}

static u64 sig (const std::vector<TileCand>& a, u16 top_n) {
    u64 h = 1469598103934665603ull;
    const u16 n = static_cast<u16>(a.size() < top_n ? a.size() : top_n);
    for (u16 i = 0; i < n; ++i) {
        const u64 v =
            (static_cast<u64>(a[i].m_x) << 48) ^
            (static_cast<u64>(a[i].m_y) << 32) ^
            (static_cast<u64>(a[i].m_yld.m_food) << 16) ^
            (static_cast<u64>(a[i].m_yld.m_production) << 8) ^
            static_cast<u64>(a[i].m_yld.m_commerce);
        h ^= v;
        h *= 1099511628211ull;
    }
    return h;
}

static const char* mode_name (SortMode mode) {
    if (mode == SM_FOOD) {
        return "food";
    }
    if (mode == SM_PRODUCTION) {
        return "production";
    }
    return "commerce";
}

template <typename Fn>
static void time_one (const char* alg, SortMode mode, const std::vector<TileCand>& src, Fn fn) {
    std::vector<TileCand> a = src;
    const auto t0 = std::chrono::high_resolution_clock::now();
    fn(a);
    const auto t1 = std::chrono::high_resolution_clock::now();
    const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
    std::printf("%-8s mode=%-8s n=%zu us=%.2f sig=%llu\n",
        alg, mode_name(mode), a.size(), us, static_cast<unsigned long long>(sig(a, 128)));
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("fail build paths\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("fail load runtime statics\n");
        return 1;
    }
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, nullptr)) {
        std::printf("fail load map\n");
        return 1;
    }
    TileYields::bind_map(&map);
    const u16 n = static_cast<u16>(G_N < map.width() ? (G_N < map.height() ? G_N : map.height()) : map.width());
    std::vector<TileCand> src;
    src.reserve(static_cast<size_t>(n) * static_cast<size_t>(n));
    for (u16 y = 0; y < n; ++y) {
        for (u16 x = 0; x < n; ++x) {
            TileCand c = {};
            c.m_x = x;
            c.m_y = y;
            c.m_yld = TileYields::get(x, y);
            src.push_back(c);
        }
    }
    std::printf("window=%ux%u n=%zu\n", static_cast<unsigned>(n), static_cast<unsigned>(n), src.size());
    const SortMode modes[3] = {SM_FOOD, SM_PRODUCTION, SM_COMMERCE};
    for (u8 i = 0; i < 3; ++i) {
        const SortMode mode = modes[i];
        time_one("quick", mode, src, [mode](std::vector<TileCand>& a) { sort_qs(a, mode); });
        time_one("select", mode, src, [mode](std::vector<TileCand>& a) { sort_sel(a, mode); });
        time_one("bucket", mode, src, [mode](std::vector<TileCand>& a) {
            pick_cnt(a, static_cast<u32>(a.size()), mode);
        });
    }
    TileYields::bind_map(nullptr);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
