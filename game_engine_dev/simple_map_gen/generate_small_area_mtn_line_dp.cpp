//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <vector>

#include "generate_small_area_mtn_line_dp.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const f64 k_pi = 3.14159265358979323846;

struct DpCtx {
    u32 m_min_tile_n;
    f64 m_angle_lim;
    f64 m_min_seg_len;
};

struct DpPt {
    u16 x;
    u16 y;
};

static bool same_pt (const DpPt& a, const DpPt& b) {
    return a.x == b.x && a.y == b.y;
}

static f64 seg_len (const DpPt& a, const DpPt& b) {
    const f64 dx = static_cast<f64>(b.x) - static_cast<f64>(a.x);
    const f64 dy = static_cast<f64>(b.y) - static_cast<f64>(a.y);
    return std::sqrt(dx * dx + dy * dy);
}

static f64 pt_seg_dist (const DpPt& p, const DpPt& a, const DpPt& b) {
    const f64 ax = static_cast<f64>(a.x);
    const f64 ay = static_cast<f64>(a.y);
    const f64 bx = static_cast<f64>(b.x);
    const f64 by = static_cast<f64>(b.y);
    const f64 px = static_cast<f64>(p.x);
    const f64 py = static_cast<f64>(p.y);
    const f64 dx = bx - ax;
    const f64 dy = by - ay;
    const f64 len_sq = dx * dx + dy * dy;
    if (len_sq < 1e-12) {
        const f64 ex = px - ax;
        const f64 ey = py - ay;
        return std::sqrt(ex * ex + ey * ey);
    }
    f64 t = ((px - ax) * dx + (py - ay) * dy) / len_sq;
    if (t < 0.0) {
        t = 0.0;
    } else if (t > 1.0) {
        t = 1.0;
    }
    const f64 cx = ax + t * dx;
    const f64 cy = ay + t * dy;
    const f64 ex = px - cx;
    const f64 ey = py - cy;
    return std::sqrt(ex * ex + ey * ey);
}

static f64 proj_t (const DpPt& p, const DpPt& a, const DpPt& b) {
    const f64 ax = static_cast<f64>(a.x);
    const f64 ay = static_cast<f64>(a.y);
    const f64 bx = static_cast<f64>(b.x);
    const f64 by = static_cast<f64>(b.y);
    const f64 px = static_cast<f64>(p.x);
    const f64 py = static_cast<f64>(p.y);
    const f64 dx = bx - ax;
    const f64 dy = by - ay;
    const f64 len_sq = dx * dx + dy * dy;
    if (len_sq < 1e-12) {
        return 0.0;
    }
    return ((px - ax) * dx + (py - ay) * dy) / len_sq;
}

static f64 inner_angle_deg (const DpPt& a, const DpPt& c, const DpPt& b) {
    const f64 v1x = static_cast<f64>(a.x) - static_cast<f64>(c.x);
    const f64 v1y = static_cast<f64>(a.y) - static_cast<f64>(c.y);
    const f64 v2x = static_cast<f64>(b.x) - static_cast<f64>(c.x);
    const f64 v2y = static_cast<f64>(b.y) - static_cast<f64>(c.y);
    const f64 l1 = std::sqrt(v1x * v1x + v1y * v1y);
    const f64 l2 = std::sqrt(v2x * v2x + v2y * v2y);
    if (l1 < 1e-9 || l2 < 1e-9) {
        return 180.0;
    }
    f64 dot = (v1x * v2x + v1y * v2y) / (l1 * l2);
    if (dot > 1.0) {
        dot = 1.0;
    } else if (dot < -1.0) {
        dot = -1.0;
    }
    return std::acos(dot) * 180.0 / k_pi;
}

static u32 pt_key (u16 x, u16 y, u16 w) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static void build_border_set (const std::vector<DpPt>& tiles, u16 w, std::map<u32, DpPt>& out) {
    out.clear();
    for (size_t i = 0; i < tiles.size(); ++i) {
        const DpPt& p = tiles[i];
        out[pt_key(p.x, p.y, w)] = p;
    }
}

static u16 border_deg (const std::map<u32, DpPt>& border, u16 w, u16 h, u16 x, u16 y) {
    u16 n = 0;
    if (x > 0) {
        if (border.find(pt_key(static_cast<u16>(x - 1u), y, w)) != border.end()) {
            ++n;
        }
    }
    if (x + 1u < w) {
        if (border.find(pt_key(static_cast<u16>(x + 1u), y, w)) != border.end()) {
            ++n;
        }
    }
    if (y > 0) {
        if (border.find(pt_key(x, static_cast<u16>(y - 1u), w)) != border.end()) {
            ++n;
        }
    }
    if (y + 1u < h) {
        if (border.find(pt_key(x, static_cast<u16>(y + 1u), w)) != border.end()) {
            ++n;
        }
    }
    return n;
}

static DpPt bfs_farthest (
    const std::map<u32, DpPt>& border,
    u16 w,
    u16 h,
    const DpPt& start) 
{
    const u32 sk = pt_key(start.x, start.y, w);
    if (border.find(sk) == border.end()) {
        return start;
    }
    std::map<u32, u32> dist;
    std::queue<u32> q;
    dist[sk] = 0;
    q.push(sk);
    u32 far_k = sk;
    u32 far_d = 0;
    while (!q.empty()) {
        const u32 k = q.front();
        q.pop();
        const DpPt p = border.find(k)->second;
        const u32 d = dist[k];
        if (d > far_d) {
            far_d = d;
            far_k = k;
        }
        const u16 nbs[4][2] = {
            {static_cast<u16>(p.x > 0 ? p.x - 1u : p.x), p.y},
            {static_cast<u16>(p.x + 1u < w ? p.x + 1u : p.x), p.y},
            {p.x, static_cast<u16>(p.y > 0 ? p.y - 1u : p.y)},
            {p.x, static_cast<u16>(p.y + 1u < h ? p.y + 1u : p.y)}};
        for (i32 ni = 0; ni < 4; ++ni) {
            const u16 nx = nbs[ni][0];
            const u16 ny = nbs[ni][1];
            if (nx == p.x && ny == p.y) {
                continue;
            }
            const u32 nk = pt_key(nx, ny, w);
            if (border.find(nk) == border.end() || dist.find(nk) != dist.end()) {
                continue;
            }
            dist[nk] = d + 1u;
            q.push(nk);
        }
    }
    return border.find(far_k)->second;
}

static bool find_endpts (const std::vector<DpPt>& tiles, u16 w, u16 h, DpPt& out_a, DpPt& out_b) {
    if (tiles.empty()) {
        return false;
    }
    if (tiles.size() == 1u) {
        out_a = tiles[0];
        out_b = tiles[0];
        return true;
    }
    std::map<u32, DpPt> border;
    build_border_set(tiles, w, border);
    std::vector<DpPt> leaves;
    leaves.reserve(tiles.size());
    for (std::map<u32, DpPt>::const_iterator it = border.begin(); it != border.end(); ++it) {
        const DpPt& p = it->second;
        if (border_deg(border, w, h, p.x, p.y) == 1u) {
            leaves.push_back(p);
        }
    }
    DpPt start = leaves.empty() ? tiles[0] : leaves[0];
    const DpPt a = bfs_farthest(border, w, h, start);
    const DpPt b = bfs_farthest(border, w, h, a);
    out_a = a;
    out_b = b;
    return true;
}

static void dp_rec (
    const DpCtx& ctx,
    const DpPt& a,
    const DpPt& b,
    const std::vector<DpPt>& tiles,
    std::vector<DpPt>& out) 
{
    out.clear();
    if (same_pt(a, b)) {
        out.push_back(a);
        return;
    }
    if (tiles.size() < ctx.m_min_tile_n) {
        out.push_back(a);
        out.push_back(b);
        return;
    }
    if (seg_len(a, b) < ctx.m_min_seg_len) {
        out.push_back(a);
        out.push_back(b);
        return;
    }
    f64 best_d = -1.0;
    DpPt best_p = a;
    for (size_t i = 0; i < tiles.size(); ++i) {
        const DpPt& p = tiles[i];
        if (same_pt(p, a) || same_pt(p, b)) {
            continue;
        }
        const f64 d = pt_seg_dist(p, a, b);
        if (d > best_d) {
            best_d = d;
            best_p = p;
        }
    }
    if (best_d < 0.5) {
        out.push_back(a);
        out.push_back(b);
        return;
    }
    const f64 ang = inner_angle_deg(a, best_p, b);
    if (ang >= ctx.m_angle_lim) {
        out.push_back(a);
        out.push_back(b);
        return;
    }
    if (seg_len(a, best_p) < ctx.m_min_seg_len || seg_len(best_p, b) < ctx.m_min_seg_len) {
        out.push_back(a);
        out.push_back(b);
        return;
    }
    const f64 tc = proj_t(best_p, a, b);
    std::vector<DpPt> left;
    std::vector<DpPt> right;
    left.reserve(tiles.size());
    right.reserve(tiles.size());
    for (size_t i = 0; i < tiles.size(); ++i) {
        const DpPt& p = tiles[i];
        if (proj_t(p, a, b) <= tc) {
            left.push_back(p);
        } else {
            right.push_back(p);
        }
    }
    if (left.empty()) {
        left.push_back(a);
        left.push_back(best_p);
    }
    if (right.empty()) {
        right.push_back(best_p);
        right.push_back(b);
    }
    std::vector<DpPt> lo;
    std::vector<DpPt> ro;
    dp_rec(ctx, a, best_p, left, lo);
    dp_rec(ctx, best_p, b, right, ro);
    out = lo;
    for (size_t i = 1; i < ro.size(); ++i) {
        out.push_back(ro[i]);
    }
}

static bool build_chain_pts (
    const DpCtx& ctx,
    u16 map_w,
    u16 map_h,
    const std::vector<DpPt>& tiles,
    DpPt& out_a,
    DpPt& out_b,
    std::vector<DpPt>& out) 
{
    if (tiles.empty()) {
        return false;
    }
    if (!find_endpts(tiles, map_w, map_h, out_a, out_b)) {
        return false;
    }
    dp_rec(ctx, out_a, out_b, tiles, out);
    return !out.empty();
}

static void collect_comp (
    u16 w,
    u16 h,
    const u16* ov,
    u16 oid,
    u32 seed_ti,
    std::vector<u32>& comp) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<u8> vis(n, 0);
    std::vector<u32> q;
    q.push_back(seed_ti);
    vis[seed_ti] = 1;
    comp.clear();
    for (size_t qi = 0; qi < q.size(); ++qi) {
        const u32 ti = q[qi];
        comp.push_back(ti);
        const u16 px = static_cast<u16>(ti % static_cast<u32>(w));
        const u16 py = static_cast<u16>(ti / static_cast<u32>(w));
        if (px > 0) {
            const u32 ni = ti - 1u;
            if (!vis[ni] && ov[ni] == oid) {
                vis[ni] = 1;
                q.push_back(ni);
            }
        }
        if (px + 1u < w) {
            const u32 ni = ti + 1u;
            if (!vis[ni] && ov[ni] == oid) {
                vis[ni] = 1;
                q.push_back(ni);
            }
        }
        if (py > 0) {
            const u32 ni = ti - static_cast<u32>(w);
            if (!vis[ni] && ov[ni] == oid) {
                vis[ni] = 1;
                q.push_back(ni);
            }
        }
        if (py + 1u < h) {
            const u32 ni = ti + static_cast<u32>(w);
            if (!vis[ni] && ov[ni] == oid) {
                vis[ni] = 1;
                q.push_back(ni);
            }
        }
    }
}

static bool ti_to_pt (u16 w, u32 ti, DpPt& out) {
    out.x = static_cast<u16>(ti % static_cast<u32>(w));
    out.y = static_cast<u16>(ti / static_cast<u32>(w));
    return true;
}

//================================================================================================================================
//=> - Generate_SmallAreaMtnLineDp -
//================================================================================================================================

SmallAreaMtnLineDpResult* Generate_SmallAreaMtnLineDp::generate (
    const WatershedMtnLinesResult* mtns,
    u16 seg_i,
    const SmallAreaMtnLineDpParams& params) 
{
    if (mtns == nullptr || mtns->overlay == nullptr || mtns->segs == nullptr || mtns->w == 0 || mtns->h == 0) {
        return nullptr;
    }
    if (seg_i >= mtns->seg_n) {
        return nullptr;
    }
    DpCtx ctx = {};
    ctx.m_min_tile_n = params.m_min_split_tile_n;
    ctx.m_angle_lim = params.m_split_angle_deg;
    ctx.m_min_seg_len = params.m_min_seg_len;
    const u16 w = mtns->w;
    const u16 h = mtns->h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u16* ov = mtns->overlay;
    const WatershedBorderSeg& seg = mtns->segs[seg_i];
    const u16 oid = seg.ov_idx;
    std::vector<SmallAreaMtnLineDpChain> chains;
    std::vector<std::vector<DpPt>> chain_pts;
    std::vector<std::vector<DpPt>> chain_borders;
    std::vector<u8> seen(n, 0);
    if (oid != 0) {
        for (u32 ti = 0; ti < n; ++ti) {
            if (seen[ti] || ov[ti] != oid) {
                continue;
            }
            std::vector<u32> comp;
            collect_comp(w, h, ov, oid, ti, comp);
            for (u32 ci = 0; ci < comp.size(); ++ci) {
                seen[comp[ci]] = 1;
            }
            if (comp.empty()) {
                continue;
            }
            std::vector<DpPt> tiles;
            tiles.reserve(comp.size());
            for (u32 ci = 0; ci < comp.size(); ++ci) {
                DpPt p = {};
                ti_to_pt(w, comp[ci], p);
                tiles.push_back(p);
            }
            std::vector<DpPt> pts;
            DpPt end_a = {};
            DpPt end_b = {};
            if (!build_chain_pts(ctx, w, h, tiles, end_a, end_b, pts) || pts.size() < 2u) {
                continue;
            }
            SmallAreaMtnLineDpChain ch = {};
            ch.ov_idx = oid;
            ch.basin_a = seg.basin_a;
            ch.basin_b = seg.basin_b;
            ch.end_a.x = end_a.x;
            ch.end_a.y = end_a.y;
            ch.end_b.x = end_b.x;
            ch.end_b.y = end_b.y;
            ch.border_tile_n = static_cast<u32>(tiles.size());
            ch.pt_n = static_cast<u16>(pts.size());
            chains.push_back(ch);
            chain_pts.push_back(pts);
            chain_borders.push_back(tiles);
        }
    }
    SmallAreaMtnLineDpResult* out = new SmallAreaMtnLineDpResult();
    if (out == nullptr) {
        return nullptr;
    }
    out->w = w;
    out->h = h;
    out->seg_i = seg_i;
    out->chain_n = static_cast<u16>(chains.size());
    if (out->chain_n == 0) {
        out->chains = nullptr;
        return out;
    }
    out->chains = new SmallAreaMtnLineDpChain[out->chain_n];
    if (out->chains == nullptr) {
        delete out;
        return nullptr;
    }
    for (u16 ci = 0; ci < out->chain_n; ++ci) {
        out->chains[ci] = chains[ci];
        out->chains[ci].pts = new SmallAreaMtnLineDpPt[chains[ci].pt_n];
        out->chains[ci].border_tiles = new SmallAreaMtnLineDpPt[chains[ci].border_tile_n];
        if (out->chains[ci].pts == nullptr || out->chains[ci].border_tiles == nullptr) {
            for (u16 dj = 0; dj < ci; ++dj) {
                delete[] out->chains[dj].pts;
                delete[] out->chains[dj].border_tiles;
            }
            delete[] out->chains;
            delete out;
            return nullptr;
        }
        for (u32 bi = 0; bi < chains[ci].border_tile_n; ++bi) {
            out->chains[ci].border_tiles[bi].x = chain_borders[ci][bi].x;
            out->chains[ci].border_tiles[bi].y = chain_borders[ci][bi].y;
        }
        for (u16 pi = 0; pi < chains[ci].pt_n; ++pi) {
            out->chains[ci].pts[pi].x = chain_pts[ci][pi].x;
            out->chains[ci].pts[pi].y = chain_pts[ci][pi].y;
        }
    }
    return out;
}

void Generate_SmallAreaMtnLineDp::free_result (SmallAreaMtnLineDpResult* res) {
    if (res == nullptr) {
        return;
    }
    if (res->chains != nullptr) {
        for (u16 i = 0; i < res->chain_n; ++i) {
            delete[] res->chains[i].pts;
            delete[] res->chains[i].border_tiles;
        }
        delete[] res->chains;
    }
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
