//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "city_connector.h"
#include "city_tile_manager.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "land_mass_index.h"
#include "map_bit_overlay.h"
#include "runtime_static_loader.h"
#include "unit_movement_mng.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/road-flood-compare-test";
static const u32 G_SEED = 43u;
static const u16 G_CITY_MAX = 5000;
static const u16 G_EDGE_PAD = 4;
static const u16 G_DIST_BLACK = 4;
static const u16 G_DIST_GRASS = 8;
static const u16 G_DIST_PLAINS = 12;
static const u16 G_CLAIM_CULT = 100u;
static const u16 G_WIN = 41;
static const u16 G_WIN_R = 20;

static const i32 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i32 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_res[320];
static u16 g_cx[G_CITY_MAX];
static u16 g_cy[G_CITY_MAX];
static u16 g_city_n = 0;

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
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool ensure_out_dir () {
    return mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static bool is_city_terr (u8 terr) {
    return terr == TERR_PLAINS[0] || terr == TERR_HILLS[0];
}

static bool is_city_clim (u8 clim) {
    return clim == CLIMATE_GRASSLAND || clim == CLIMATE_PLAINS || clim == CLIMATE_BLACK_SOIL;
}

static u16 clim_min_dist (u8 clim) {
    if (clim == CLIMATE_BLACK_SOIL) {
        return G_DIST_BLACK;
    }
    if (clim == CLIMATE_GRASSLAND) {
        return G_DIST_GRASS;
    }
    return G_DIST_PLAINS;
}

static bool far_enough (u16 x, u16 y, u16 min_d) {
    for (u16 i = 0; i < g_city_n; ++i) {
        const i32 dx = static_cast<i32>(x) - static_cast<i32>(g_cx[i]);
        const i32 dy = static_cast<i32>(y) - static_cast<i32>(g_cy[i]);
        const u16 adx = static_cast<u16>(dx < 0 ? -dx : dx);
        const u16 ady = static_cast<u16>(dy < 0 ? -dy : dy);
        const u16 d = adx > ady ? adx : ady;
        if (d < min_d) {
            return false;
        }
    }
    return true;
}

static u16 collect_city_pts (const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    g_city_n = 0;
    for (u16 y = G_EDGE_PAD; y + G_EDGE_PAD < h && g_city_n < G_CITY_MAX; ++y) {
        for (u16 x = G_EDGE_PAD; x + G_EDGE_PAD < w && g_city_n < G_CITY_MAX; ++x) {
            if (map.get_river(x, y) == 0) {
                continue;
            }
            if (!is_city_terr(map.get_terrain(x, y)) || !is_city_clim(map.get_climate(x, y))) {
                continue;
            }
            const u16 min_d = clim_min_dist(map.get_climate(x, y));
            if (!far_enough(x, y, min_d)) {
                continue;
            }
            g_cx[g_city_n] = x;
            g_cy[g_city_n] = y;
            g_city_n = static_cast<u16>(g_city_n + 1u);
        }
    }
    return g_city_n;
}

static bool tile_block (const GameState& state, u16 x, u16 y) {
    if (x >= state.m_map.width() || y >= state.m_map.height()) {
        return true;
    }
    const u8 terr = state.m_map.get_terrain(x, y);
    if (overlay_is_water_terr(terr)) {
        return true;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_NONE[0]) {
        return true;
    }
    return false;
}

static bool road_pass (const GameState& state, u16 x, u16 y) {
    if (x >= state.m_map.width() || y >= state.m_map.height()) {
        return false;
    }
    if (state.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
        return true;
    }
    return state.m_map.tile(x, y)->m_road_typ != ROAD_NONE;
}

static i32 sgn (i32 v) {
    return (v > 0) - (v < 0);
}

static bool step_linear (u16 x0, u16 y0, u16 x1, u16 y1, u16* ox, u16* oy) {
    if (x0 == x1 && y0 == y1) {
        return false;
    }
    const i32 ax = static_cast<i32>(x1) - static_cast<i32>(x0);
    const i32 ay = static_cast<i32>(y1) - static_cast<i32>(y0);
    const i32 adx = ax < 0 ? -ax : ax;
    const i32 ady = ay < 0 ? -ay : ay;
    const i32 sx = sgn(ax);
    const i32 sy = sgn(ay);
    i32 nx = static_cast<i32>(x0);
    i32 ny = static_cast<i32>(y0);
    if (adx > ady) {
        if (ady == 0 || (ady + ady) <= adx) {
            nx += sx;
        } else {
            nx += sx;
            ny += sy;
        }
    } else if (ady > adx) {
        if (adx == 0 || (adx + adx) <= ady) {
            ny += sy;
        } else {
            nx += sx;
            ny += sy;
        }
    } else {
        nx += sx;
        ny += sy;
    }
    if (nx < 0 || ny < 0) {
        return false;
    }
    *ox = static_cast<u16>(nx);
    *oy = static_cast<u16>(ny);
    return true;
}

static void paint_link_road (GameState& state, u16 x0, u16 y0, u16 x1, u16 y1) {
    u16 x = x0;
    u16 y = y0;
    for (;;) {
        GameTileSimple* t = state.m_map.tile(x, y);
        if (t->m_road_typ == ROAD_NONE) {
            t->m_road_typ = ROAD_PATH;
        }
        if (x == x1 && y == y1) {
            return;
        }
        u16 nx = 0;
        u16 ny = 0;
        if (!step_linear(x, y, x1, y1, &nx, &ny)) {
            return;
        }
        x = nx;
        y = ny;
    }
}

static void paint_net_roads (GameState& state) {
    const u16 n = state.m_cities.get_city_count();
    for (u16 i = 0; i < n; ++i) {
        const City* a = state.m_cities.get_city(i);
        if (a == nullptr) {
            continue;
        }
        for (u8 d = 0; d < 4u; ++d) {
            const u16 j = a->get_conn_city(d);
            if (j == U16_KEY_NULL || j < i) {
                continue;
            }
            const City* b = state.m_cities.get_city(j);
            if (b == nullptr) {
                continue;
            }
            paint_link_road(state, a->get_x(), a->get_y(), b->get_x(), b->get_y());
        }
    }
}

static void fill_terr (const GameArraySimple& map, u8* terr) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            terr[static_cast<u32>(y) * w + x] = map.get_terrain(x, y);
        }
    }
}

static u16 mass_at (const LandMassIndexRslt& mass, u16 x, u16 y) {
    return mass.m_ov[static_cast<u32>(y) * mass.m_w + x];
}

static u32 dist2 (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    return static_cast<u32>(dx * dx + dy * dy);
}

static void label_net_comps (const GameState& state, u16* comp, u16* out_comp_n) {
    const u16 n = state.m_cities.get_city_count();
    for (u16 i = 0; i < n; ++i) {
        comp[i] = U16_KEY_NULL;
    }
    u16 comp_n = 0;
    std::vector<u16> q;
    q.reserve(n);
    for (u16 s = 0; s < n; ++s) {
        if (comp[s] != U16_KEY_NULL) {
            continue;
        }
        if (state.m_cities.get_city(s) == nullptr) {
            continue;
        }
        const u16 cid = comp_n;
        comp_n = static_cast<u16>(comp_n + 1u);
        comp[s] = cid;
        q.clear();
        q.push_back(s);
        for (u32 qi = 0; qi < q.size(); ++qi) {
            const u16 cur = q[qi];
            const City* c = state.m_cities.get_city(cur);
            if (c == nullptr) {
                continue;
            }
            for (u8 d = 0; d < 4u; ++d) {
                const u16 j = c->get_conn_city(d);
                if (j == U16_KEY_NULL || j >= n || comp[j] != U16_KEY_NULL) {
                    continue;
                }
                comp[j] = cid;
                q.push_back(j);
            }
        }
    }
    *out_comp_n = comp_n;
}

static bool net_reachable (const GameState& state, u16 a, u16 b) {
    const u16 n = state.m_cities.get_city_count();
    if (a >= n || b >= n) {
        return false;
    }
    if (a == b) {
        return true;
    }
    std::vector<u8> seen(n, 0);
    std::vector<u16> q;
    q.reserve(n);
    seen[a] = 1;
    q.push_back(a);
    for (u32 qi = 0; qi < q.size(); ++qi) {
        const u16 cur = q[qi];
        if (cur == b) {
            return true;
        }
        const City* c = state.m_cities.get_city(cur);
        if (c == nullptr) {
            continue;
        }
        for (u8 d = 0; d < 4u; ++d) {
            const u16 j = c->get_conn_city(d);
            if (j == U16_KEY_NULL || j >= n || seen[j] != 0) {
                continue;
            }
            seen[j] = 1;
            q.push_back(j);
        }
    }
    return false;
}

static bool pick_far_pair (
    const GameState& state,
    const LandMassIndexRslt& mass,
    const u16* comp,
    u16* out_a,
    u16* out_b)
{
    const u16 n = state.m_cities.get_city_count();
    const u16 mid = mass.m_largest_idx;
    u16 best_a = U16_KEY_NULL;
    u16 best_b = U16_KEY_NULL;
    u32 best_d = 0;
    for (u16 i = 0; i < n; ++i) {
        const City* ca = state.m_cities.get_city(i);
        if (ca == nullptr || comp[i] == U16_KEY_NULL) {
            continue;
        }
        if (mass_at(mass, ca->get_x(), ca->get_y()) != mid) {
            continue;
        }
        for (u16 j = static_cast<u16>(i + 1u); j < n; ++j) {
            const City* cb = state.m_cities.get_city(j);
            if (cb == nullptr || comp[j] == U16_KEY_NULL) {
                continue;
            }
            if (comp[i] != comp[j]) {
                continue;
            }
            if (mass_at(mass, cb->get_x(), cb->get_y()) != mid) {
                continue;
            }
            const u32 d = dist2(ca->get_x(), ca->get_y(), cb->get_x(), cb->get_y());
            if (d > best_d) {
                best_d = d;
                best_a = i;
                best_b = j;
            }
        }
    }
    if (best_a == U16_KEY_NULL || best_b == U16_KEY_NULL) {
        return false;
    }
    *out_a = best_a;
    *out_b = best_b;
    return true;
}

static u16 flood_walk (const GameState& state, u16 sx, u16 sy, u16 tx, u16 ty, u16* dist) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        dist[i] = U16_KEY_NULL;
    }
    std::vector<u32> q;
    q.reserve(n / 8u);
    const u32 s = static_cast<u32>(sy) * w + sx;
    const u32 goal = static_cast<u32>(ty) * w + tx;
    dist[s] = 0;
    if (s == goal) {
        return 0;
    }
    q.push_back(s);
    for (u32 qi = 0; qi < q.size(); ++qi) {
        const u32 cur = q[qi];
        const u16 d0 = dist[cur];
        const u16 cx = static_cast<u16>(cur % w);
        const u16 cy = static_cast<u16>(cur / w);
        for (u8 dir = 0; dir < 8u; ++dir) {
            const i32 nx = static_cast<i32>(cx) + k_dx[dir];
            const i32 ny = static_cast<i32>(cy) + k_dy[dir];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            if (tile_block(state, static_cast<u16>(nx), static_cast<u16>(ny))) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * w + static_cast<u32>(nx);
            if (dist[ni] != U16_KEY_NULL) {
                continue;
            }
            dist[ni] = static_cast<u16>(d0 + 1u);
            if (ni == goal) {
                return dist[ni];
            }
            q.push_back(ni);
        }
    }
    return U16_KEY_NULL;
}

static u16 flood_road (const GameState& state, u16 sx, u16 sy, u16 tx, u16 ty, u16* dist) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        dist[i] = U16_KEY_NULL;
    }
    std::vector<u32> q;
    q.reserve(n / 16u);
    const u32 s = static_cast<u32>(sy) * w + sx;
    const u32 goal = static_cast<u32>(ty) * w + tx;
    dist[s] = 0;
    if (s == goal) {
        return 0;
    }
    q.push_back(s);
    for (u32 qi = 0; qi < q.size(); ++qi) {
        const u32 cur = q[qi];
        const u16 d0 = dist[cur];
        const u16 cx = static_cast<u16>(cur % w);
        const u16 cy = static_cast<u16>(cur / w);
        for (u8 dir = 0; dir < 8u; ++dir) {
            const i32 nx = static_cast<i32>(cx) + k_dx[dir];
            const i32 ny = static_cast<i32>(cy) + k_dy[dir];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!road_pass(state, ux, uy)) {
                continue;
            }
            const u32 ni = static_cast<u32>(uy) * w + static_cast<u32>(ux);
            if (dist[ni] != U16_KEY_NULL) {
                continue;
            }
            dist[ni] = static_cast<u16>(d0 + 1u);
            if (ni == goal) {
                return dist[ni];
            }
            q.push_back(ni);
        }
    }
    return U16_KEY_NULL;
}

static u16 flood_nodes (const GameState& state, u16 src, u16 dst, u16* dist) {
    const u16 n = state.m_cities.get_city_count();
    for (u16 i = 0; i < n; ++i) {
        dist[i] = U16_KEY_NULL;
    }
    if (src >= n || dst >= n) {
        return U16_KEY_NULL;
    }
    std::vector<u16> q;
    q.reserve(n);
    dist[src] = 0;
    if (src == dst) {
        return 0;
    }
    q.push_back(src);
    for (u32 qi = 0; qi < q.size(); ++qi) {
        const u16 cur = q[qi];
        const u16 d0 = dist[cur];
        const City* c = state.m_cities.get_city(cur);
        if (c == nullptr) {
            continue;
        }
        for (u8 d = 0; d < 4u; ++d) {
            const u16 j = c->get_conn_city(d);
            if (j == U16_KEY_NULL || j >= n || dist[j] != U16_KEY_NULL) {
                continue;
            }
            dist[j] = static_cast<u16>(d0 + 1u);
            if (j == dst) {
                return dist[j];
            }
            q.push_back(j);
        }
    }
    return U16_KEY_NULL;
}

static bool recon_nodes (const GameState& state, const u16* dist, u16 src, u16 dst, std::vector<u16>* out) {
    out->clear();
    if (dist[dst] == U16_KEY_NULL) {
        return false;
    }
    u16 cur = dst;
    out->push_back(cur);
    while (cur != src) {
        const u16 d0 = dist[cur];
        if (d0 == 0) {
            return false;
        }
        const City* c = state.m_cities.get_city(cur);
        if (c == nullptr) {
            return false;
        }
        u16 prev = U16_KEY_NULL;
        for (u8 d = 0; d < 4u; ++d) {
            const u16 j = c->get_conn_city(d);
            if (j == U16_KEY_NULL || dist[j] != static_cast<u16>(d0 - 1u)) {
                continue;
            }
            prev = j;
            break;
        }
        if (prev == U16_KEY_NULL) {
            return false;
        }
        cur = prev;
        out->push_back(cur);
    }
    for (u32 i = 0, j = static_cast<u32>(out->size()); i + 1u < j; ++i, --j) {
        const u16 t = (*out)[i];
        (*out)[i] = (*out)[j - 1u];
        (*out)[j - 1u] = t;
    }
    return true;
}

static bool walk_grad (const u16* dist, u16 w, u16 h, u16 sx, u16 sy, u16 tx, u16 ty, std::vector<u32>* path) {
    path->clear();
    if (dist[static_cast<u32>(ty) * w + tx] == U16_KEY_NULL) {
        return false;
    }
    u16 x = tx;
    u16 y = ty;
    for (;;) {
        path->push_back(static_cast<u32>(y) * w + x);
        if (x == sx && y == sy) {
            break;
        }
        const u16 d0 = dist[static_cast<u32>(y) * w + x];
        if (d0 == 0 || d0 == U16_KEY_NULL) {
            return false;
        }
        u16 nx = x;
        u16 ny = y;
        bool hit = false;
        for (u8 dir = 0; dir < 8u; ++dir) {
            const i32 ix = static_cast<i32>(x) + k_dx[dir];
            const i32 iy = static_cast<i32>(y) + k_dy[dir];
            if (ix < 0 || iy < 0 || ix >= static_cast<i32>(w) || iy >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(ix);
            const u16 uy = static_cast<u16>(iy);
            if (dist[static_cast<u32>(uy) * w + ux] != static_cast<u16>(d0 - 1u)) {
                continue;
            }
            nx = ux;
            ny = uy;
            hit = true;
            break;
        }
        if (!hit) {
            return false;
        }
        x = nx;
        y = ny;
    }
    return true;
}

static u16 flood_road_win (const GameState& state, u16 sx, u16 sy, u16 tx, u16 ty, u16* dist) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    const i32 x0 = static_cast<i32>(sx) - static_cast<i32>(G_WIN_R);
    const i32 y0 = static_cast<i32>(sy) - static_cast<i32>(G_WIN_R);
    const i32 x1 = static_cast<i32>(sx) + static_cast<i32>(G_WIN_R);
    const i32 y1 = static_cast<i32>(sy) + static_cast<i32>(G_WIN_R);
    const u16 xmin = static_cast<u16>(x0 < 0 ? 0 : x0);
    const u16 ymin = static_cast<u16>(y0 < 0 ? 0 : y0);
    const u16 xmax = static_cast<u16>(x1 >= static_cast<i32>(w) ? w - 1u : static_cast<u16>(x1));
    const u16 ymax = static_cast<u16>(y1 >= static_cast<i32>(h) ? h - 1u : static_cast<u16>(y1));
    for (u16 y = ymin; y <= ymax; ++y) {
        for (u16 x = xmin; x <= xmax; ++x) {
            dist[static_cast<u32>(y) * w + x] = U16_KEY_NULL;
        }
    }
    if (tx < xmin || tx > xmax || ty < ymin || ty > ymax) {
        return U16_KEY_NULL;
    }
    if (sx < xmin || sx > xmax || sy < ymin || sy > ymax) {
        return U16_KEY_NULL;
    }
    std::vector<u32> q;
    q.reserve(static_cast<u32>(G_WIN) * static_cast<u32>(G_WIN));
    const u32 sink = static_cast<u32>(ty) * w + tx;
    const u32 src = static_cast<u32>(sy) * w + sx;
    dist[sink] = 0;
    if (sink == src) {
        return 0;
    }
    q.push_back(sink);
    for (u32 qi = 0; qi < q.size(); ++qi) {
        const u32 cur = q[qi];
        const u16 d0 = dist[cur];
        const u16 cx = static_cast<u16>(cur % w);
        const u16 cy = static_cast<u16>(cur / w);
        for (u8 dir = 0; dir < 8u; ++dir) {
            const i32 nx = static_cast<i32>(cx) + k_dx[dir];
            const i32 ny = static_cast<i32>(cy) + k_dy[dir];
            if (nx < static_cast<i32>(xmin) || ny < static_cast<i32>(ymin) ||
                nx > static_cast<i32>(xmax) || ny > static_cast<i32>(ymax)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!road_pass(state, ux, uy)) {
                continue;
            }
            const u32 ni = static_cast<u32>(uy) * w + static_cast<u32>(ux);
            if (dist[ni] != U16_KEY_NULL) {
                continue;
            }
            dist[ni] = static_cast<u16>(d0 + 1u);
            if (ni == src) {
                return dist[ni];
            }
            q.push_back(ni);
        }
    }
    return U16_KEY_NULL;
}

static bool walk_nodes (
    const GameState& state,
    const std::vector<u16>& nodes,
    u16* dist,
    std::vector<u32>* path)
{
    path->clear();
    if (nodes.empty()) {
        return false;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    for (u32 i = 0; i + 1u < nodes.size(); ++i) {
        const City* a = state.m_cities.get_city(nodes[i]);
        const City* b = state.m_cities.get_city(nodes[i + 1u]);
        if (a == nullptr || b == nullptr) {
            return false;
        }
        const u16 sx = a->get_x();
        const u16 sy = a->get_y();
        const u16 tx = b->get_x();
        const u16 ty = b->get_y();
        if (flood_road_win(state, sx, sy, tx, ty, dist) == U16_KEY_NULL) {
            return false;
        }
        std::vector<u32> seg;
        if (!walk_grad(dist, w, h, tx, ty, sx, sy, &seg)) {
            return false;
        }
        for (u32 k = 0; k < seg.size(); ++k) {
            if (!path->empty() && path->back() == seg[k]) {
                continue;
            }
            path->push_back(seg[k]);
        }
    }
    return true;
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
    }
}

static void grad_rgb (u16 d, u16 max_d, u8* r, u8* g, u8* b) {
    if (max_d == 0) {
        *r = 255;
        *g = 0;
        *b = 0;
        return;
    }
    const f32 t = static_cast<f32>(d) / static_cast<f32>(max_d);
    *r = static_cast<u8>(255.0f * (1.0f - t) + 0.5f);
    *g = 0;
    *b = static_cast<u8>(255.0f * t + 0.5f);
}

static bool wr_grad_ppm (
    const GameState& state,
    const u16* dist,
    u16 max_d,
    u16 ax,
    u16 ay,
    u16 bx,
    u16 by,
    cstr path)
{
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    if (max_d == U16_KEY_NULL) {
        max_d = 0;
    }
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
            const u16 d = dist[static_cast<u32>(y) * w + x];
            if (d == U16_KEY_NULL) {
                p[0] = 0;
                p[1] = 0;
                p[2] = 0;
            } else {
                grad_rgb(d, max_d, &p[0], &p[1], &p[2]);
            }
        }
    }
    auto mark = [&](u16 x, u16 y, u8 r, u8 g, u8 b) {
        if (x >= w || y >= h) {
            return;
        }
        u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
        p[0] = r;
        p[1] = g;
        p[2] = b;
    };
    mark(ax, ay, 0, 255, 0);
    mark(bx, by, 255, 255, 0);
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void put_px (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
    p[0] = r;
    p[1] = g;
    p[2] = b;
}

static bool wr_path_ppm (const GameState& state, const std::vector<u32>& path, cstr out) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
            terr_rgb(state.m_map.get_terrain(x, y), &p[0], &p[1], &p[2]);
        }
    }
    for (u32 i = 0; i < path.size(); ++i) {
        const u32 t = path[i];
        put_px(rgb.data(), w, h, static_cast<u16>(t % w), static_cast<u16>(t / w), 0, 0, 0);
    }
    FILE* fp = std::fopen(out, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void draw_line (u8* rgb, u16 w, u16 h, u16 x0, u16 y0, u16 x1, u16 y1, u8 r, u8 g, u8 b) {
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
        put_px(rgb, w, h, static_cast<u16>(ax), static_cast<u16>(ay), r, g, b);
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

static bool wr_node_grad_ppm (
    const GameState& state,
    const u16* dist,
    u16 max_d,
    u16 ia,
    u16 ib,
    cstr path)
{
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    const u16 n = state.m_cities.get_city_count();
    if (max_d == U16_KEY_NULL) {
        max_d = 0;
    }
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u, 0);
    for (u16 i = 0; i < n; ++i) {
        const City* a = state.m_cities.get_city(i);
        if (a == nullptr || dist[i] == U16_KEY_NULL) {
            continue;
        }
        for (u8 d = 0; d < 4u; ++d) {
            const u16 j = a->get_conn_city(d);
            if (j == U16_KEY_NULL || j < i || dist[j] == U16_KEY_NULL) {
                continue;
            }
            const City* b = state.m_cities.get_city(j);
            if (b == nullptr) {
                continue;
            }
            draw_line(rgb.data(), w, h, a->get_x(), a->get_y(), b->get_x(), b->get_y(), 40, 40, 40);
        }
    }
    for (u16 i = 0; i < n; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || dist[i] == U16_KEY_NULL) {
            continue;
        }
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        grad_rgb(dist[i], max_d, &r, &g, &b);
        put_px(rgb.data(), w, h, c->get_x(), c->get_y(), r, g, b);
    }
    const City* ca = state.m_cities.get_city(ia);
    const City* cb = state.m_cities.get_city(ib);
    if (ca != nullptr) {
        put_px(rgb.data(), w, h, ca->get_x(), ca->get_y(), 0, 255, 0);
    }
    if (cb != nullptr) {
        put_px(rgb.data(), w, h, cb->get_x(), cb->get_y(), 255, 255, 0);
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool init_state (GameState* state, RuntimeStatics& st) {
    state->clear();
    state->m_statics = &st;
    if (!UnitMovementMng::setup_mvt_costs(st)) {
        return false;
    }
    state->m_civ_relations.reset(st.civ().get_item_count());
    if (!Factory_GameArraySimple::load_map_gen_data(&state->m_map, g_terr, g_clim, g_riv)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&state->m_map, g_res)) {
        return false;
    }
    const u16 w = state->m_map.width();
    const u16 h = state->m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    if (!state->m_cities.bind_statics(st)) {
        return false;
    }
    CityBorder::bind_map(&state->m_map);
    CityTileManager::bind_cities(&state->m_cities);
    PlayerState* seat = new PlayerState();
    if (seat == nullptr) {
        return false;
    }
    seat->m_ai_controlled = 1;
    seat->m_is_active = 1;
    seat->m_civ_index = 0;
    seat->m_explored_overlay = new MapBitOverlay(w, h);
    seat->m_techs_researched = nullptr;
    if (seat->m_explored_overlay == nullptr) {
        delete seat;
        return false;
    }
    state->m_player_states = seat;
    state->m_player_n = 1;
    state->m_players_remaining = 1;
    return true;
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("fail build paths\n");
        return 1;
    }
    if (!ensure_out_dir()) {
        std::printf("fail mkdir %s\n", G_OUT_DIR);
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("fail load runtime statics\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    GameState state;
    if (!init_state(&state, st)) {
        std::printf("fail init state\n");
        return 1;
    }
    const u16 city_n = collect_city_pts(state.m_map);
    if (city_n < 2u) {
        std::printf("fail city pts got %u\n", (unsigned)city_n);
        return 1;
    }
    for (u16 i = 0; i < city_n; ++i) {
        const u16 idx = state.m_cities.get_next_new_city_idx();
        City* city = state.m_cities.get_city(idx);
        if (city == nullptr) {
            std::printf("fail city alloc at %u\n", (unsigned)i);
            return 1;
        }
        city->init(0, g_cx[i], g_cy[i]);
        if (!state.m_map.set_tile_add(g_cx[i], g_cy[i], idx, BUILD_ADD_CITY)) {
            std::printf("fail city tile at (%u,%u)\n", (unsigned)g_cx[i], (unsigned)g_cy[i]);
            return 1;
        }
        CityBorder::claim_expand(g_cx[i], g_cy[i], 0, G_CLAIM_CULT, 0);
    }
    if (!CityConnector::begin(state)) {
        std::printf("fail city connector begin\n");
        return 1;
    }
    paint_net_roads(state);
    CityConnector::clear();
    const u16 city_cap = state.m_cities.get_city_count();
    u16* net_comp = new u16[city_cap];
    u16 net_comp_n = 0;
    label_net_comps(state, net_comp, &net_comp_n);
    std::printf("city_net_components=%u\n", (unsigned)net_comp_n);
    const u32 tile_n = state.m_map.tile_n();
    u8* terr = new u8[tile_n];
    fill_terr(state.m_map, terr);
    LandMassIndex mass;
    if (!mass.generate(terr, state.m_map.width(), state.m_map.height()) || !mass.is_valid()) {
        std::printf("fail land mass index\n");
        delete[] terr;
        delete[] net_comp;
        return 1;
    }
    delete[] terr;
    const LandMassIndexRslt& mr = mass.result();
    std::printf("land_masses=%u largest=%u land_tiles=%u cities=%u\n",
        (unsigned)mr.m_mass_n, (unsigned)mr.m_largest_idx, mr.m_land_n, (unsigned)city_n);
    u16 ia = U16_KEY_NULL;
    u16 ib = U16_KEY_NULL;
    if (!pick_far_pair(state, mr, net_comp, &ia, &ib)) {
        std::printf("fail pick far pair on largest mass in same city-net component\n");
        delete[] net_comp;
        return 1;
    }
    if (!net_reachable(state, ia, ib)) {
        std::printf("fail city net not connected for pair %u -> %u\n", (unsigned)ia, (unsigned)ib);
        delete[] net_comp;
        return 1;
    }
    std::printf("city_net_connected=yes comp=%u\n", (unsigned)net_comp[ia]);
    delete[] net_comp;
    const City* ca = state.m_cities.get_city(ia);
    const City* cb = state.m_cities.get_city(ib);
    const u16 ax = ca->get_x();
    const u16 ay = ca->get_y();
    const u16 bx = cb->get_x();
    const u16 by = cb->get_y();
    std::printf("pair a=%u (%u,%u) b=%u (%u,%u) dist2=%u\n",
        (unsigned)ia, (unsigned)ax, (unsigned)ay,
        (unsigned)ib, (unsigned)bx, (unsigned)by,
        dist2(ax, ay, bx, by));
    u16* dist_w = new u16[tile_n];
    u16* dist_r = new u16[tile_n];
    u16* dist_n = new u16[city_cap];
    const auto t0 = std::chrono::high_resolution_clock::now();
    const u16 d_w = flood_walk(state, ax, ay, bx, by, dist_w);
    const auto t1 = std::chrono::high_resolution_clock::now();
    const auto t2 = std::chrono::high_resolution_clock::now();
    const u16 d_r = flood_road(state, ax, ay, bx, by, dist_r);
    const auto t3 = std::chrono::high_resolution_clock::now();
    const auto t4 = std::chrono::high_resolution_clock::now();
    const u16 d_n = flood_nodes(state, ia, ib, dist_n);
    const auto t5 = std::chrono::high_resolution_clock::now();
    const double ms_w = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double ms_r = std::chrono::duration<double, std::milli>(t3 - t2).count();
    const double ms_n = std::chrono::duration<double, std::milli>(t5 - t4).count();
    std::printf("walkable_flood_ms=%.2f reach_b=%s d=%u\n",
        ms_w, d_w == U16_KEY_NULL ? "no" : "yes", d_w == U16_KEY_NULL ? 0u : (unsigned)d_w);
    std::printf("road_flood_ms=%.2f reach_b=%s d=%u\n",
        ms_r, d_r == U16_KEY_NULL ? "no" : "yes", d_r == U16_KEY_NULL ? 0u : (unsigned)d_r);
    std::printf("node_bfs_ms=%.2f reach_b=%s hops=%u\n",
        ms_n, d_n == U16_KEY_NULL ? "no" : "yes", d_n == U16_KEY_NULL ? 0u : (unsigned)d_n);
    char path[512];
    std::snprintf(path, sizeof(path), "%s/01_walkable_grad.ppm", G_OUT_DIR);
    if (!wr_grad_ppm(state, dist_w, d_w == U16_KEY_NULL ? 0 : d_w, ax, ay, bx, by, path)) {
        std::printf("fail write %s\n", path);
        delete[] dist_w;
        delete[] dist_r;
        delete[] dist_n;
        return 1;
    }
    std::printf("wrote %s\n", path);
    std::snprintf(path, sizeof(path), "%s/02_road_grad.ppm", G_OUT_DIR);
    if (!wr_grad_ppm(state, dist_r, d_r == U16_KEY_NULL ? 0 : d_r, ax, ay, bx, by, path)) {
        std::printf("fail write %s\n", path);
        delete[] dist_w;
        delete[] dist_r;
        delete[] dist_n;
        return 1;
    }
    std::printf("wrote %s\n", path);
    std::snprintf(path, sizeof(path), "%s/03_node_bfs_grad.ppm", G_OUT_DIR);
    if (!wr_node_grad_ppm(state, dist_n, d_n == U16_KEY_NULL ? 0 : d_n, ia, ib, path)) {
        std::printf("fail write %s\n", path);
        delete[] dist_w;
        delete[] dist_r;
        delete[] dist_n;
        return 1;
    }
    std::printf("wrote %s\n", path);
    std::vector<u32> path_w;
    std::vector<u32> path_r;
    std::vector<u32> path_n;
    std::vector<u16> nodes;
    const u16 mw = state.m_map.width();
    const u16 mh = state.m_map.height();
    const auto tw0 = std::chrono::high_resolution_clock::now();
    const bool ok_w = (d_w != U16_KEY_NULL) && walk_grad(dist_w, mw, mh, ax, ay, bx, by, &path_w);
    const auto tw1 = std::chrono::high_resolution_clock::now();
    const auto tw2 = std::chrono::high_resolution_clock::now();
    const bool ok_r = (d_r != U16_KEY_NULL) && walk_grad(dist_r, mw, mh, ax, ay, bx, by, &path_r);
    const auto tw3 = std::chrono::high_resolution_clock::now();
    const auto tw4 = std::chrono::high_resolution_clock::now();
    const bool ok_n = (d_n != U16_KEY_NULL) && recon_nodes(state, dist_n, ia, ib, &nodes) &&
        walk_nodes(state, nodes, dist_r, &path_n);
    const auto tw5 = std::chrono::high_resolution_clock::now();
    const double ms_ww = std::chrono::duration<double, std::milli>(tw1 - tw0).count();
    const double ms_rw = std::chrono::duration<double, std::milli>(tw3 - tw2).count();
    const double ms_nw = std::chrono::duration<double, std::milli>(tw5 - tw4).count();
    std::printf("walkable_walk_ms=%.2f ok=%s len=%u\n",
        ms_ww, ok_w ? "yes" : "no", (unsigned)path_w.size());
    std::printf("road_walk_ms=%.2f ok=%s len=%u\n",
        ms_rw, ok_r ? "yes" : "no", (unsigned)path_r.size());
    std::printf("node_walk_ms=%.2f ok=%s nodes=%u len=%u\n",
        ms_nw, ok_n ? "yes" : "no", (unsigned)nodes.size(), (unsigned)path_n.size());
    std::snprintf(path, sizeof(path), "%s/04_walkable_path.ppm", G_OUT_DIR);
    if (!wr_path_ppm(state, path_w, path)) {
        std::printf("fail write %s\n", path);
        delete[] dist_w;
        delete[] dist_r;
        delete[] dist_n;
        return 1;
    }
    std::printf("wrote %s\n", path);
    std::snprintf(path, sizeof(path), "%s/05_road_path.ppm", G_OUT_DIR);
    if (!wr_path_ppm(state, path_r, path)) {
        std::printf("fail write %s\n", path);
        delete[] dist_w;
        delete[] dist_r;
        delete[] dist_n;
        return 1;
    }
    std::printf("wrote %s\n", path);
    std::snprintf(path, sizeof(path), "%s/06_node_path.ppm", G_OUT_DIR);
    if (!wr_path_ppm(state, path_n, path)) {
        std::printf("fail write %s\n", path);
        delete[] dist_w;
        delete[] dist_r;
        delete[] dist_n;
        return 1;
    }
    std::printf("wrote %s\n", path);
    delete[] dist_w;
    delete[] dist_r;
    delete[] dist_n;
    CityTileManager::bind_cities(nullptr);
    CityBorder::bind_map(nullptr);
    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
