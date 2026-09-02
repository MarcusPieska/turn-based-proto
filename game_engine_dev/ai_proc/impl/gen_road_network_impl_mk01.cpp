//================================================================================================================================
//=> - Includes (mk01: multi-source Dijkstra + Kruskal MST) -
//================================================================================================================================

#include <algorithm>
#include <cstring>
#include <queue>
#include <vector>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Local (mk01) -
//================================================================================================================================

static const u16 k_none = 0xffffu;
static const u32 k_inf = 0xffffffffu;
static const i8 k_dx8[8] = {1, 1, 0, -1, -1, -1, 0, 1};
static const i8 k_dy8[8] = {0, 1, 1, 1, 0, -1, -1, -1};

struct GrnTerm {
    u16 m_x;
    u16 m_y;
};

struct GrnEdge {
    u16 m_a;
    u16 m_b;
    u32 m_w;
    u32 m_ia;
    u32 m_ib;
};

struct GrnNode {
    u32 m_i;
    u32 m_d;
};

struct GrnNodeCmp {
    bool operator() (const GrnNode& a, const GrnNode& b) const {
        return a.m_d > b.m_d;
    }
};

static u32 tix (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool is_mtn (u8 terr) {
    return terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0];
}

static bool tile_pass (const GameArraySimple& map, u16 x, u16 y) {
    const u8 terr = map.get_terrain(x, y);
    if (overlay_is_water_terr(terr) || terr == TERR_NONE[0]) {
        return false;
    }
    if (!is_mtn(terr)) {
        return true;
    }
    const u8 iv = map.get_ai_ov_intent(x, y);
    return iv == AI_TILE_OV_INTENT_MTN_PASS || iv == AI_TILE_OV_INTENT_FORT;
}

static u32 step_cost (const GameArraySimple& map, u16 x, u16 y) {
    const u8 rd = map.get_road_typ(x, y);
    if (rd != ROAD_NONE) {
        return 1u;
    }
    const u8 terr = map.get_terrain(x, y);
    if (is_mtn(terr)) {
        return 3u;
    }
    if (map.get_river(x, y) != 0u) {
        return 2u;
    }
    if (terr == TERR_HILLS[0]) {
        return 5u;
    }
    return 4u;
}

static bool add_term (std::vector<GrnTerm>* terms, u16 x, u16 y, u16 w, u16 h) {
    if (terms == nullptr || x >= w || y >= h) {
        return false;
    }
    terms->push_back(GrnTerm{x, y});
    return true;
}

static void dedup_terms (std::vector<GrnTerm>* terms) {
    if (terms == nullptr || terms->size() < 2u) {
        return;
    }
    std::sort(terms->begin(), terms->end(), [] (const GrnTerm& a, const GrnTerm& b) {
        if (a.m_y != b.m_y) {
            return a.m_y < b.m_y;
        }
        return a.m_x < b.m_x;
    });
    u32 w = 1;
    for (u32 i = 1; i < terms->size(); ++i) {
        if ((*terms)[i].m_x == (*terms)[w - 1u].m_x && (*terms)[i].m_y == (*terms)[w - 1u].m_y) {
            continue;
        }
        (*terms)[w] = (*terms)[i];
        ++w;
    }
    terms->resize(w);
}

static void collect_terms (
    const GameArraySimple& map,
    const SpgCoordPair* starts,
    u32 start_n,
    std::vector<GrnTerm>* terms)
{
    GAME_EXPECT(terms != nullptr, "GenRoadNetwork collect_terms got nullptr terms");
    terms->clear();
    const u16 w = map.width();
    const u16 h = map.height();
    if (starts != nullptr) {
        for (u32 i = 0; i < start_n; ++i) {
            add_term(terms, starts[i].x, starts[i].y, w, h);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 iv = map.get_ai_ov_intent(x, y);
            if (iv == AI_TILE_OV_INTENT_CITY || iv == AI_TILE_OV_INTENT_FORT) {
                add_term(terms, x, y, w, h);
            }
        }
    }
    dedup_terms(terms);
}

static u16 uf_find (u16* p, u16 x) {
    u16 r = x;
    while (p[r] != r) {
        r = p[r];
    }
    while (p[x] != x) {
        const u16 n = p[x];
        p[x] = r;
        x = n;
    }
    return r;
}

static bool uf_union (u16* p, u16 a, u16 b) {
    const u16 ra = uf_find(p, a);
    const u16 rb = uf_find(p, b);
    if (ra == rb) {
        return false;
    }
    p[rb] = ra;
    return true;
}

static void stamp_tile (GameArraySimple& map, Whiteboard_2B& road, u32* road_n, u16 x, u16 y) {
    if (road.rd(x, y) != 0u) {
        return;
    }
    road.wr(x, y, 1u);
    *road_n = *road_n + 1u;
    if (map.get_road_typ(x, y) == ROAD_NONE) {
        map.set_road_typ(x, y, ROAD_VIRTUAL);
    }
}

static void stamp_path (
    GameArraySimple& map,
    Whiteboard_2B& road,
    u32* road_n,
    const Whiteboard_4B& par,
    u32 i)
{
    u32 cur = i;
    while (cur != k_inf) {
        const u16 w = map.width();
        const u16 x = static_cast<u16>(cur % static_cast<u32>(w));
        const u16 y = static_cast<u16>(cur / static_cast<u32>(w));
        stamp_tile(map, road, road_n, x, y);
        const u32 p = par.rd_i(cur);
        if (p == cur) {
            break;
        }
        cur = p;
    }
}

//================================================================================================================================
//=> - GenRoadNetwork (mk01) -
//================================================================================================================================

bool GenRoadNetwork::build (const SpgCoordPair* starts, u32 start_n) {
    GAME_EXPECT_RET(m_ok && m_map != nullptr, false, "GenRoadNetwork build not begun");
    GameArraySimple& map = *m_map;
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tn = map.tile_n();
    std::memset(m_road.raw(), 0, static_cast<size_t>(tn) * sizeof(u16));
    std::memset(m_foll.raw(), 0, static_cast<size_t>(tn) * sizeof(u16));
    m_road_n = 0;
    m_term_n = 0;
    m_seg_n = 0;
    std::vector<GrnTerm> terms;
    collect_terms(map, starts, start_n, &terms);
    if (terms.empty()) {
        return true;
    }
    if (terms.size() > static_cast<size_t>(k_none)) {
        return false;
    }
    m_term_n = static_cast<u32>(terms.size());
    Whiteboard_2B own("GenRoadNetwork", "own", 0u);
    Whiteboard_4B dist("GenRoadNetwork", "dist", 0u);
    Whiteboard_4B par("GenRoadNetwork", "par", 0u);
    if (!own.ok() || !dist.ok() || !par.ok()) {
        return false;
    }
    for (u32 i = 0; i < tn; ++i) {
        own.wr_i(i, k_none);
        dist.wr_i(i, k_inf);
        par.wr_i(i, k_inf);
    }
    std::priority_queue<GrnNode, std::vector<GrnNode>, GrnNodeCmp> pq;
    for (u32 t = 0; t < m_term_n; ++t) {
        const u16 x = terms[t].m_x;
        const u16 y = terms[t].m_y;
        if (!tile_pass(map, x, y)) {
            continue;
        }
        const u32 i = tix(w, x, y);
        own.wr_i(i, static_cast<u16>(t));
        dist.wr_i(i, 0u);
        par.wr_i(i, i);
        pq.push(GrnNode{i, 0u});
    }
    while (!pq.empty()) {
        const GrnNode cur = pq.top();
        pq.pop();
        if (cur.m_d != dist.rd_i(cur.m_i)) {
            continue;
        }
        const u16 cx = static_cast<u16>(cur.m_i % static_cast<u32>(w));
        const u16 cy = static_cast<u16>(cur.m_i / static_cast<u32>(w));
        const u16 co = own.rd_i(cur.m_i);
        for (u8 k = 0; k < 8u; ++k) {
            const i32 nx = static_cast<i32>(cx) + static_cast<i32>(k_dx8[k]);
            const i32 ny = static_cast<i32>(cy) + static_cast<i32>(k_dy8[k]);
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!tile_pass(map, ux, uy)) {
                continue;
            }
            const u32 ni = tix(w, ux, uy);
            const u32 nd = cur.m_d + step_cost(map, ux, uy);
            if (nd >= dist.rd_i(ni)) {
                continue;
            }
            dist.wr_i(ni, nd);
            own.wr_i(ni, co);
            par.wr_i(ni, cur.m_i);
            pq.push(GrnNode{ni, nd});
        }
    }
    std::vector<GrnEdge> edges;
    edges.reserve(m_term_n * 4u);
    std::vector<u32> best_w(static_cast<size_t>(m_term_n) * static_cast<size_t>(m_term_n), k_inf);
    std::vector<u32> best_ia(static_cast<size_t>(m_term_n) * static_cast<size_t>(m_term_n), k_inf);
    std::vector<u32> best_ib(static_cast<size_t>(m_term_n) * static_cast<size_t>(m_term_n), k_inf);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tix(w, x, y);
            const u16 oa = own.rd_i(i);
            if (oa == k_none) {
                continue;
            }
            for (u8 k = 0; k < 4u; ++k) {
                const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_dx8[k]);
                const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_dy8[k]);
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u32 j = tix(w, static_cast<u16>(nx), static_cast<u16>(ny));
                const u16 ob = own.rd_i(j);
                if (ob == k_none || ob == oa) {
                    continue;
                }
                u16 a = oa;
                u16 b = ob;
                u32 ia = i;
                u32 ib = j;
                if (a > b) {
                    const u16 ts = a;
                    a = b;
                    b = ts;
                    const u32 ti = ia;
                    ia = ib;
                    ib = ti;
                }
                const u32 ew = dist.rd_i(ia) + dist.rd_i(ib);
                const u32 slot = static_cast<u32>(a) * m_term_n + static_cast<u32>(b);
                if (ew < best_w[slot]) {
                    best_w[slot] = ew;
                    best_ia[slot] = ia;
                    best_ib[slot] = ib;
                }
            }
        }
    }
    for (u32 a = 0; a < m_term_n; ++a) {
        for (u32 b = a + 1u; b < m_term_n; ++b) {
            const u32 slot = a * m_term_n + b;
            if (best_w[slot] == k_inf) {
                continue;
            }
            edges.push_back(GrnEdge{
                static_cast<u16>(a), static_cast<u16>(b), best_w[slot], best_ia[slot], best_ib[slot]});
        }
    }
    std::sort(edges.begin(), edges.end(), [] (const GrnEdge& a, const GrnEdge& b) {
        if (a.m_w != b.m_w) {
            return a.m_w < b.m_w;
        }
        if (a.m_a != b.m_a) {
            return a.m_a < b.m_a;
        }
        return a.m_b < b.m_b;
    });
    std::vector<u16> uf(m_term_n);
    for (u32 i = 0; i < m_term_n; ++i) {
        uf[i] = static_cast<u16>(i);
    }
    for (u32 t = 0; t < m_term_n; ++t) {
        stamp_tile(map, m_road, &m_road_n, terms[t].m_x, terms[t].m_y);
    }
    u32 used = 0;
    for (size_t e = 0; e < edges.size(); ++e) {
        if (!uf_union(uf.data(), edges[e].m_a, edges[e].m_b)) {
            continue;
        }
        stamp_path(map, m_road, &m_road_n, par, edges[e].m_ia);
        stamp_path(map, m_road, &m_road_n, par, edges[e].m_ib);
        ++used;
        if (used + 1u >= m_term_n) {
            break;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of mk01 -
//================================================================================================================================
