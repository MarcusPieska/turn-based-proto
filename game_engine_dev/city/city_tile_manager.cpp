//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_tile_manager.h"

#include "circular_tile_areas.h"
#include "city.h"
#include "city_array.h"
#include "tile_working.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - TileCand -
//================================================================================================================================

struct TileCand {
    u16 m_x; // Map column
    u16 m_y; // Map row
    TileYield m_yld; // Yields on this tile
};

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const u16 CityTileManager::m_reach_r = 4;
const u16 CityTileManager::m_cand_max = 45;
CityArray* CityTileManager::m_cities = nullptr;

static const u16 k_cand_max = 45;

//================================================================================================================================
//=> - Pick helpers -
//================================================================================================================================

static u8 pri_key (const TileCand& c, u8 sort_food, u8 sort_production, u8 sort_commerce) {
    if (sort_food != 0) {
        (void)sort_production;
        (void)sort_commerce;
        return c.m_yld.m_food;
    }
    if (sort_production != 0) {
        (void)sort_commerce;
        return c.m_yld.m_production;
    }
    (void)sort_food;
    (void)sort_production;
    return c.m_yld.m_commerce;
}

static u8 pri_bkt (u8 key) {
    return key >= 5 ? 5 : key;
}

static const u8 k_bkt_n = 6;

static i32 cmp_cand (const TileCand& a, const TileCand& b, u8 sort_food, u8 sort_production, u8 sort_commerce) {
    if (sort_food != 0) {
        (void)sort_commerce;
        if (a.m_yld.m_food != b.m_yld.m_food) {
            return static_cast<i32>(a.m_yld.m_food) - static_cast<i32>(b.m_yld.m_food);
        }
        if (a.m_yld.m_production != b.m_yld.m_production) {
            return static_cast<i32>(a.m_yld.m_production) - static_cast<i32>(b.m_yld.m_production);
        }
        return static_cast<i32>(a.m_yld.m_commerce) - static_cast<i32>(b.m_yld.m_commerce);
    }
    if (sort_production != 0) {
        (void)sort_commerce;
        if (a.m_yld.m_production != b.m_yld.m_production) {
            return static_cast<i32>(a.m_yld.m_production) - static_cast<i32>(b.m_yld.m_production);
        }
        if (a.m_yld.m_food != b.m_yld.m_food) {
            return static_cast<i32>(a.m_yld.m_food) - static_cast<i32>(b.m_yld.m_food);
        }
        return static_cast<i32>(a.m_yld.m_commerce) - static_cast<i32>(b.m_yld.m_commerce);
    }
    (void)sort_food;
    (void)sort_production;
    if (a.m_yld.m_commerce != b.m_yld.m_commerce) {
        return static_cast<i32>(a.m_yld.m_commerce) - static_cast<i32>(b.m_yld.m_commerce);
    }
    if (a.m_yld.m_food != b.m_yld.m_food) {
        return static_cast<i32>(a.m_yld.m_food) - static_cast<i32>(b.m_yld.m_food);
    }
    return static_cast<i32>(a.m_yld.m_production) - static_cast<i32>(b.m_yld.m_production);
}

static void pick_cands (TileCand* cands, u16 n, u16 pick_n, u8 sort_food, u8 sort_production, u8 sort_commerce) {
    if (n == 0 || pick_n == 0) {
        return;
    }
    if (pick_n > n) {
        pick_n = n;
    }
    u16 cnt[k_bkt_n] = {};
    for (u16 i = 0; i < n; ++i) {
        const u8 key = pri_key(cands[i], sort_food, sort_production, sort_commerce);
        ++cnt[pri_bkt(key)];
    }
    u16 need = pick_n;
    u16 take[k_bkt_n] = {};
    for (i8 b = 5; b >= 0 && need > 0; --b) {
        const u8 bi = static_cast<u8>(b);
        if (cnt[bi] <= need) {
            take[bi] = cnt[bi];
            need -= cnt[bi];
        } else {
            take[bi] = need;
            need = 0;
        }
    }
    TileCand out[k_cand_max];
    u16 w = 0;
    for (i8 b = 5; b >= 0; --b) {
        const u8 bi = static_cast<u8>(b);
        if (take[bi] == 0) {
            continue;
        }
        if (take[bi] < cnt[bi]) {
            TileCand sub[k_cand_max];
            u16 m = 0;
            for (u16 i = 0; i < n; ++i) {
                const u8 key = pri_key(cands[i], sort_food, sort_production, sort_commerce);
                if (pri_bkt(key) != bi) {
                    continue;
                }
                sub[m] = cands[i];
                ++m;
            }
            for (u16 i = 1; i < m; ++i) {
                TileCand key = sub[i];
                u16 j = i;
                while (j > 0 && cmp_cand(sub[j - 1], key, sort_food, sort_production, sort_commerce) < 0) {
                    sub[j] = sub[j - 1];
                    --j;
                }
                sub[j] = key;
            }
            for (u16 i = 0; i < take[bi]; ++i) {
                out[w] = sub[i];
                ++w;
            }
            continue;
        }
        u16 rem = take[bi];
        for (u16 i = 0; i < n && rem > 0; ++i) {
            const u8 key = pri_key(cands[i], sort_food, sort_production, sort_commerce);
            if (pri_bkt(key) != bi) {
                continue;
            }
            out[w] = cands[i];
            ++w;
            --rem;
        }
    }
    for (u16 i = 0; i < w; ++i) {
        cands[i] = out[i];
    }
}

//================================================================================================================================
//=> - CityTileManager -
//================================================================================================================================

void CityTileManager::bind_cities (CityArray* cities) {
    m_cities = cities;
}

TotalTileYield CityTileManager::maximize_food (u16 player, u16 city_idx) {
    return assign_sorted(player, city_idx, 1, 0, 0);
}

TotalTileYield CityTileManager::maximize_production (u16 player, u16 city_idx) {
    return assign_sorted(player, city_idx, 0, 1, 0);
}

TotalTileYield CityTileManager::maximize_commerce (u16 player, u16 city_idx) {
    return assign_sorted(player, city_idx, 0, 0, 1);
}

TotalTileYield CityTileManager::add_new_food_tile (u16 player, u16 city_idx) {
    return assign_add_one(player, city_idx, 1, 0, 0);
}

TotalTileYield CityTileManager::add_new_production_tile (u16 player, u16 city_idx) {
    return assign_add_one(player, city_idx, 0, 1, 0);
}

TotalTileYield CityTileManager::add_new_commerce_tile (u16 player, u16 city_idx) {
    return assign_add_one(player, city_idx, 0, 0, 1);
}

TotalTileYield CityTileManager::gather_yields (u16 player, u16 city_idx) {
    TotalTileYield tot = {};
    if (m_cities == nullptr) {
        return tot;
    }
    City* city = m_cities->get_city(city_idx);
    if (city == nullptr || city->get_owner() != player) {
        return tot;
    }
    const u16 cx = city->get_x();
    const u16 cy = city->get_y();
    const CircArea area = CircularTileAreas::get(m_reach_r);
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux == cx && uy == cy) {
            continue;
        }
        if (!TileYields::in_bounds(ux, uy)) {
            continue;
        }
        if (TileWorking::get_worker(ux, uy) != city_idx) {
            continue;
        }
        const TileYield yld = TileYields::get(ux, uy);
        tot.m_food += yld.m_food;
        tot.m_production += yld.m_production;
        tot.m_commerce += yld.m_commerce;
    }
    return tot;
}

void CityTileManager::clear (u16 cx, u16 cy, u16 city_idx) {
    const CircArea area = CircularTileAreas::get(m_reach_r);
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (!TileYields::in_bounds(ux, uy)) {
            continue;
        }
        TileWorking::clear_worked(ux, uy, city_idx);
    }
}

u32 CityTileManager::count_worked (u16 cx, u16 cy, u16 city_idx) {
    const CircArea area = CircularTileAreas::get(m_reach_r);
    u32 n = 0;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (!TileYields::in_bounds(ux, uy)) {
            continue;
        }
        if (ux == cx && uy == cy) {
            continue;
        }
        if (TileWorking::get_worker(ux, uy) == city_idx) {
            ++n;
        }
    }
    return n;
}

TotalTileYield CityTileManager::assign_sorted (u16 player, u16 city_idx, u8 sort_food, u8 sort_production, u8 sort_commerce) {
    TotalTileYield tot = {};
    if (m_cities == nullptr) {
        return tot;
    }
    City* city = m_cities->get_city(city_idx);
    if (city == nullptr || city->get_owner() != player) {
        return tot;
    }
    const u16 cx = city->get_x();
    const u16 cy = city->get_y();
    const u16 workers = city->get_current_population();
    if (workers == 0) {
        return tot;
    }
    const CircArea area = CircularTileAreas::get(m_reach_r);
    if (area.m_lim == 0) {
        return tot;
    }
    clear(cx, cy, city_idx);
    TileCand cands[m_cand_max];
    u16 n = 0;
    for (u16 i = 0; i < area.m_lim && n < m_cand_max; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (!TileYields::in_bounds(ux, uy)) {
            continue;
        }
        if (ux == cx && uy == cy) {
            continue;
        }
        const u16 wk = TileWorking::get_worker(ux, uy);
        if (wk != U16_KEY_NULL && wk != city_idx) {
            continue;
        }
        cands[n].m_x = ux;
        cands[n].m_y = uy;
        cands[n].m_yld = TileYields::get(ux, uy);
        ++n;
    }
    pick_cands(cands, n, workers, sort_food, sort_production, sort_commerce);
    u16 assigned = 0;
    for (u16 i = 0; i < n && assigned < workers; ++i) {
        tot.m_food += cands[i].m_yld.m_food;
        tot.m_production += cands[i].m_yld.m_production;
        tot.m_commerce += cands[i].m_yld.m_commerce;
        TileWorking::mark_worked(cands[i].m_x, cands[i].m_y, city_idx);
        ++assigned;
    }
    return tot;
}

TotalTileYield CityTileManager::assign_add_one (u16 player, u16 city_idx, u8 sort_food, u8 sort_production, u8 sort_commerce) {
    TotalTileYield tot = {};
    if (m_cities == nullptr) {
        return tot;
    }
    City* city = m_cities->get_city(city_idx);
    if (city == nullptr || city->get_owner() != player) {
        return tot;
    }
    const u16 cx = city->get_x();
    const u16 cy = city->get_y();
    const u16 workers = city->get_current_population();
    if (workers == 0) {
        return tot;
    }
    const CircArea area = CircularTileAreas::get(m_reach_r);
    if (area.m_lim == 0) {
        return tot;
    }
    u32 worked_n = 0;
    TileCand best = {};
    u8 have_best = 0;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (!TileYields::in_bounds(ux, uy)) {
            continue;
        }
        if (ux == cx && uy == cy) {
            continue;
        }
        const u16 wk = TileWorking::get_worker(ux, uy);
        if (wk == city_idx) {
            ++worked_n;
            continue;
        }
        if (wk != U16_KEY_NULL) {
            continue;
        }
        TileCand cand = {};
        cand.m_x = ux;
        cand.m_y = uy;
        cand.m_yld = TileYields::get(ux, uy);
        if (have_best == 0 || cmp_cand(cand, best, sort_food, sort_production, sort_commerce) > 0) {
            best = cand;
            have_best = 1;
        }
    }
    if (worked_n >= workers || have_best == 0) {
        return tot;
    }
    tot.m_food = best.m_yld.m_food;
    tot.m_production = best.m_yld.m_production;
    tot.m_commerce = best.m_yld.m_commerce;
    TileWorking::mark_worked(best.m_x, best.m_y, city_idx);
    return tot;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
