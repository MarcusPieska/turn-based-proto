//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_own_xfer_save_timeline_mng.h"

#include <cstdio>

#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_map_defs.h"
#include "map_own_xfer_save_snap.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u16 k_seat_cap = 256u;
static const char* k_grn = "\033[92m";
static const char* k_rst = "\033[0m";

static bool is_land_tile (u8 terr) {
    if (terr == TERR_OCEAN[0] || terr == TERR_SEA[0] || terr == TERR_COASTAL[0]) {
        return false;
    }
    if (terr == TERR_INLAND_SEA[0] || terr == TERR_INLAND_LAKE[0]) {
        return false;
    }
    return true;
}

static u32 count_land_tiles (const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 n = 0u;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (is_land_tile(map.get_terrain(x, y))) {
                ++n;
            }
        }
    }
    return n;
}

static u32 count_agg_own (const GameArraySimple& map, const u16* seats, u16 n) {
    if (seats == nullptr || n == 0u) {
        return 0u;
    }
    bool agg[k_seat_cap];
    for (u16 s = 0; s < k_seat_cap; ++s) {
        agg[s] = false;
    }
    for (u16 i = 0; i < n; ++i) {
        if (seats[i] < k_seat_cap) {
            agg[seats[i]] = true;
        }
    }
    const u16 w = map.width();
    const u16 h = map.height();
    u32 own = 0u;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!is_land_tile(map.get_terrain(x, y))) {
                continue;
            }
            const u8 o = map.get_civ_owner(x, y);
            if (o < k_seat_cap && agg[o]) {
                ++own;
            }
        }
    }
    return own;
}

static void print_net_pos (const i32* net, u32 land_n, const GameArraySimple& map) {
    if (net == nullptr || land_n == 0u) {
        return;
    }
    u16 seats[k_seat_cap];
    u16 n = 0u;
    i32 tot = 0;
    for (u16 s = 0; s < k_seat_cap; ++s) {
        if (net[s] <= 0) {
            continue;
        }
        seats[n++] = s;
        tot += net[s];
    }
    for (u16 i = 1; i < n; ++i) {
        const u16 s = seats[i];
        u16 j = i;
        while (j > 0u) {
            const u16 p = seats[j - 1u];
            if (net[p] > net[s] || (net[p] == net[s] && p < s)) {
                break;
            }
            seats[j] = p;
            --j;
        }
        seats[j] = s;
    }
    const u32 agg_own = count_agg_own(map, seats, n);
    const double agg_pct = 100.0 * static_cast<double>(agg_own) / static_cast<double>(land_n);
    const double tot_pct = 100.0 * static_cast<double>(tot) / static_cast<double>(land_n);
    std::printf("======================================\n");
    std::printf("Aggressor ownership: %s%.1f %%%s\n", k_grn, agg_pct, k_rst);
    std::printf("Total transfer: %s%.1f %%%s\n", k_grn, tot_pct, k_rst);
    for (u16 i = 0; i < n; ++i) {
        const u16 s = seats[i];
        const double pct = 100.0 * static_cast<double>(net[s]) / static_cast<double>(land_n);
        std::printf("  - Seat %u: %s%.1f %%%s\n", static_cast<u32>(s), k_grn, pct, k_rst);
    }
    std::printf("======================================\n");
}

//================================================================================================================================
//=> - MapOwnXferSaveTimelineMng -
//================================================================================================================================

MapOwnXferSaveTimelineMng::MapOwnXferSaveTimelineMng ()
    : m_save_n(0),
      m_wrote_n(0) {
}

MapOwnXferSaveTimelineMng::~MapOwnXferSaveTimelineMng () {
    clr();
}

void MapOwnXferSaveTimelineMng::clr () {
    m_save_n = 0;
    m_wrote_n = 0;
}

bool MapOwnXferSaveTimelineMng::setup (u16 save_n) {
    clr();
    if (save_n < 2) {
        return false;
    }
    m_save_n = save_n;
    return true;
}

bool MapOwnXferSaveTimelineMng::fill (const EvalPaths& paths, cstr out_dir, cstr share_dir) {
    if (out_dir == nullptr || out_dir[0] == 0 || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    m_wrote_n = 0;
    char map_p[512];
    if (!paths.map_path(paths.save_turn_at(0), map_p, sizeof(map_p))) {
        return false;
    }
    GameArraySimple buf_a;
    GameArraySimple buf_b;
    if (!GameIo::load_map_tiles(map_p, buf_a)) {
        return false;
    }
    const u16 w = buf_a.width();
    const u16 h = buf_a.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 land_n = count_land_tiles(buf_a);
    u8* acc = new u8[tile_n];
    if (acc == nullptr) {
        return false;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        acc[i] = U8_KEY_NULL;
    }
    i32 net[k_seat_cap];
    for (u16 s = 0; s < k_seat_cap; ++s) {
        net[s] = 0;
    }
    GameArraySimple* prev = &buf_a;
    GameArraySimple* cur = &buf_b;
    for (u16 si = 1; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        if (!paths.map_path(turn, map_p, sizeof(map_p))) {
            delete[] acc;
            return false;
        }
        char out_p[512];
        if (std::snprintf(out_p, sizeof(out_p), "%s/xfer_t%04u.ppm", out_dir, turn) <= 0) {
            delete[] acc;
            return false;
        }
        cur->clear();
        if (!GameIo::load_map_tiles(map_p, *cur)) {
            delete[] acc;
            return false;
        }
        if (cur->width() != w || cur->height() != h) {
            delete[] acc;
            return false;
        }
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u8 now = cur->get_civ_owner(x, y);
                const u8 was = prev->get_civ_owner(x, y);
                if (now == was || was == U8_KEY_NULL || now == U8_KEY_NULL) {
                    continue;
                }
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                acc[i] = now;
                if (!is_land_tile(cur->get_terrain(x, y))) {
                    continue;
                }
                if (now < k_seat_cap) {
                    net[now] += 1;
                }
                if (was < k_seat_cap) {
                    net[was] -= 1;
                }
            }
        }
        if (!MapOwnXferSaveSnap::write(out_p, *cur, *prev)) {
            delete[] acc;
            return false;
        }
        GameArraySimple* tmp = prev;
        prev = cur;
        cur = tmp;
        m_wrote_n = static_cast<u16>(m_wrote_n + 1u);
        std::printf("wrote %s\n", out_p);
    }
    char all_p[512];
    if (std::snprintf(all_p, sizeof(all_p), "%s/xfer_all.ppm", out_dir) <= 0) {
        delete[] acc;
        return false;
    }
    if (!MapOwnXferSaveSnap::write_seats(all_p, *prev, acc)) {
        delete[] acc;
        return false;
    }
    std::printf("wrote %s\n", all_p);
    if (share_dir != nullptr && share_dir[0] != 0) {
        char share_p[512];
        if (std::snprintf(share_p, sizeof(share_p), "%s/tile_own_xfer.ppm", share_dir) <= 0) {
            delete[] acc;
            return false;
        }
        if (!MapOwnXferSaveSnap::write_seats(share_p, *prev, acc)) {
            delete[] acc;
            return false;
        }
        std::printf("wrote %s\n", share_p);
    }
    print_net_pos(net, land_n, *prev);
    delete[] acc;
    return true;
}

u16 MapOwnXferSaveTimelineMng::save_n () const {
    return m_save_n;
}

u16 MapOwnXferSaveTimelineMng::wrote_n () const {
    return m_wrote_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
