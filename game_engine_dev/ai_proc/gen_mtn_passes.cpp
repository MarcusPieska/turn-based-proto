//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_mtn_passes.h"

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

#define GFL_MIN_PATCH_TILES 50
#define GFL_MIN_STEP_DELTA 30
#define GFL_PASS_COOL 20
#define GFL_WIN_R 5
#define GFL_SIDE_R 5
#define GFL_PATH_MAX 64
#define GFL_DIR_NONE 0u
#define GFL_VIS_ET 1u
#define GFL_VIS_ET_MTN 2u

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_mtn (u8 terr) {
    return terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0];
}

static bool is_inland_water (u8 terr) {
    return terr == TERR_INLAND_SEA[0] || terr == TERR_INLAND_LAKE[0];
}

static bool is_patch_fill (u8 terr) {
    return is_mtn(terr) || is_inland_water(terr);
}

static bool is_ext_water (u8 terr) {
    return terr == TERR_OCEAN[0] || terr == TERR_SEA[0] || terr == TERR_COASTAL[0];
}

static bool is_edge_terr (u8 terr) {
    if (terr == TERR_NONE[0] || is_mtn(terr) || is_inland_water(terr)) {
        return false;
    }
    return is_ext_water(terr) || !overlay_is_water_terr(terr);
}

static u32 tidx (u16 w, u32 x, u32 y) {
    return y * static_cast<u32>(w) + x;
}

static bool patch_core_at (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    u16 pid,
    u16 x,
    u16 y)
{
    return patch.rd(x, y) == pid && is_patch_fill(map.get_terrain(x, y));
}

static bool borders_core8 (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    u16 pid,
    u16 x,
    u16 y)
{
    const u16 w = map.width();
    const u16 h = map.height();
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(x) + dx;
            const i32 ny = static_cast<i32>(y) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            if (patch_core_at(map, patch, pid, static_cast<u16>(nx), static_cast<u16>(ny))) {
                return true;
            }
        }
    }
    return false;
}

static bool is_edge_tile (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    u16 pid,
    u16 x,
    u16 y)
{
    if (patch_core_at(map, patch, pid, x, y)) {
        return false;
    }
    const u8 terr = map.get_terrain(x, y);
    if (!is_edge_terr(terr)) {
        return false;
    }
    return borders_core8(map, patch, pid, x, y);
}

static bool perim_mtn_adj8_terr (const GameArraySimple& map, u16 x, u16 y) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(x) + dx;
            const i32 ny = static_cast<i32>(y) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            if (is_mtn(map.get_terrain(static_cast<u16>(nx), static_cast<u16>(ny)))) {
                return true;
            }
        }
    }
    return false;
}

static bool perim_mtn_adj8 (const GameArraySimple& map, const Whiteboard_1B& pass, u16 x, u16 y) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(x) + dx;
            const i32 ny = static_cast<i32>(y) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (pass.rd(ux, uy) != 0u) {
                continue;
            }
            if (is_mtn(map.get_terrain(ux, uy))) {
                return true;
            }
        }
    }
    return false;
}

static const i32 g_gfl_dx8[9] = {0, 0, 1, 1, 1, 0, -1, -1, -1};
static const i32 g_gfl_dy8[9] = {0, -1, -1, 0, 1, 1, 1, 0, -1};

static u8 dir_from_delta (i32 dx, i32 dy) {
    if (dx == 0 && dy == 0) {
        return GFL_DIR_NONE;
    }
    i32 sx = 0;
    i32 sy = 0;
    if (dx > 0) {
        sx = 1;
    } else if (dx < 0) {
        sx = -1;
    }
    if (dy > 0) {
        sy = 1;
    } else if (dy < 0) {
        sy = -1;
    }
    for (u8 d = 1u; d <= 8u; ++d) {
        if (g_gfl_dx8[d] == sx && g_gfl_dy8[d] == sy) {
            return d;
        }
    }
    return GFL_DIR_NONE;
}

static bool is_new_win_cell (i32 ox, i32 oy, u8 dir, i32 r) {
    if (dir == GFL_DIR_NONE || dir > 8u) {
        return true;
    }
    if (ox < -r || ox > r || oy < -r || oy > r) {
        return false;
    }
    const i32 px = ox + g_gfl_dx8[dir];
    const i32 py = oy + g_gfl_dy8[dir];
    if (px >= -r && px <= r && py >= -r && py <= r) {
        return false;
    }
    return true;
}

static void wr_wdir (
    Whiteboard_1B& wdir,
    u16 w,
    u32 from_i,
    u32 to_i)
{
    const u32 wi = static_cast<u32>(w);
    const u16 fx = static_cast<u16>(from_i % wi);
    const u16 fy = static_cast<u16>(from_i / wi);
    const u16 tx = static_cast<u16>(to_i % wi);
    const u16 ty = static_cast<u16>(to_i / wi);
    wdir.wr_i(to_i, dir_from_delta(static_cast<i32>(tx) - static_cast<i32>(fx),
        static_cast<i32>(ty) - static_cast<i32>(fy)));
}

static bool is_et_vis (u8 v) {
    return v >= GFL_VIS_ET;
}

static u32 et_step4 (u16 w, u32 wi, u32 hi, u32 i, i32 d, const u8* vis, u32 skip) {
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    const u32 py = i / wi;
    const u32 px = i - py * wi;
    const i32 nx = static_cast<i32>(px) + dx4[d];
    const i32 ny = static_cast<i32>(py) + dy4[d];
    if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
        return 0xFFFFFFFFu;
    }
    const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
    if (ni == skip || !is_et_vis(vis[ni])) {
        return 0xFFFFFFFFu;
    }
    return ni;
}

static void wr_walk_step (Whiteboard_2B& walk, u32 i, u16 step) {
    walk.wr_i(i, step);
}

static void fill_step_tile (const u32* et, u32 en, const Whiteboard_2B& walk, u32* step_tile) {
    for (u32 k = 0; k < en; ++k) {
        const u16 st = walk.rd_i(et[k]);
        if (st == 0u || step_tile[st] != 0u) {
            continue;
        }
        step_tile[st] = et[k];
    }
}

static bool step_on_side (u16 step, u16 anchor, u16 max_c) {
    if (step == 0u || anchor == 0u || max_c == 0u) {
        return false;
    }
    for (i32 d = -static_cast<i32>(GFL_SIDE_R); d <= static_cast<i32>(GFL_SIDE_R); ++d) {
        i32 t = static_cast<i32>(anchor) + d;
        while (t < 1) {
            t += static_cast<i32>(max_c);
        }
        while (t > static_cast<i32>(max_c)) {
            t -= static_cast<i32>(max_c);
        }
        if (static_cast<u16>(t) == step) {
            return true;
        }
    }
    return false;
}

static bool is_open_terr (u8 terr) {
    if (terr == TERR_NONE[0] || is_mtn(terr) || is_inland_water(terr) || is_ext_water(terr)) {
        return false;
    }
    return true;
}

static bool mtn_adj_a_side (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    const Whiteboard_2B& walk,
    u16 pid,
    u16 w,
    u16 h,
    u32 ni,
    u16 max_c,
    u16 step_a)
{
    const u16 x = static_cast<u16>(ni % static_cast<u32>(w));
    const u16 y = static_cast<u16>(ni / static_cast<u32>(w));
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(x) + dx;
            const i32 ny = static_cast<i32>(y) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!is_edge_tile(map, patch, pid, ux, uy)) {
                continue;
            }
            if (step_on_side(walk.rd(ux, uy), step_a, max_c)) {
                return true;
            }
        }
    }
    return false;
}

static bool mtn_adj_open_not_a (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    const Whiteboard_2B& walk,
    u16 pid,
    u16 w,
    u16 h,
    u32 ni,
    u16 max_c,
    u16 step_a,
    u16 step_b,
    bool* on_b)
{
    *on_b = false;
    const u16 x = static_cast<u16>(ni % static_cast<u32>(w));
    const u16 y = static_cast<u16>(ni / static_cast<u32>(w));
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(x) + dx;
            const i32 ny = static_cast<i32>(y) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!is_edge_tile(map, patch, pid, ux, uy)) {
                continue;
            }
            if (!is_open_terr(map.get_terrain(ux, uy))) {
                continue;
            }
            if (step_on_side(walk.rd(ux, uy), step_a, max_c)) {
                continue;
            }
            if (step_on_side(walk.rd(ux, uy), step_b, max_c)) {
                *on_b = true;
            }
            return true;
        }
    }
    return false;
}

static u32 trim_a_side_path (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    const Whiteboard_2B& walk,
    u16 pid,
    u16 w,
    u16 h,
    u16 max_c,
    u16 step_a,
    u32* path,
    u32 pn)
{
    u32 i = 1u;
    while (i < pn) {
        if (mtn_adj_a_side(map, patch, walk, pid, w, h, path[i], max_c, step_a)) {
            for (u32 k = i - 1u; k + 1u < pn; ++k) {
                path[k] = path[k + 1u];
            }
            --pn;
            if (i > 1u) {
                --i;
            }
        } else {
            ++i;
        }
    }
    return pn;
}

static void walk_chain (
    Whiteboard_2B& walk,
    Whiteboard_1B& wdir,
    u16 w,
    u32 wi,
    u32 hi,
    u32 start,
    u32 from,
    u16 start_step,
    const u8* vis,
    u16* max_c)
{
    u32 prev = from;
    u32 cur = start;
    u16 step = start_step;
    wr_walk_step(walk, cur, step);
    wr_wdir(wdir, w, prev, cur);
    if (step > *max_c) {
        *max_c = step;
    }
    for (;;) {
        u32 nxt = 0xFFFFFFFFu;
        for (i32 d = 0; d < 4; ++d) {
            const u32 ni = et_step4(w, wi, hi, cur, d, vis, prev);
            if (ni == 0xFFFFFFFFu || walk.rd_i(ni) != 0u) {
                continue;
            }
            nxt = ni;
            break;
        }
        if (nxt == 0xFFFFFFFFu) {
            break;
        }
        step = static_cast<u16>(step + 1u);
        wr_walk_step(walk, nxt, step);
        wr_wdir(wdir, w, cur, nxt);
        if (step > *max_c) {
            *max_c = step;
        }
        prev = cur;
        cur = nxt;
    }
}

static u16 walk_perim (
    Whiteboard_2B& walk,
    Whiteboard_1B& wdir,
    u16 w,
    u16 h,
    const u32* et,
    u32 en,
    const u8* vis,
    u32 seed_i)
{
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    u16 max_c = 0;
    const u32 root = seed_i;
    wr_walk_step(walk, root, 1u);
    wdir.wr_i(root, GFL_DIR_NONE);
    max_c = 1u;
    for (i32 d = 0; d < 4; ++d) {
        const u32 ni = et_step4(w, wi, hi, root, d, vis, 0xFFFFFFFFu);
        if (ni == 0xFFFFFFFFu || walk.rd_i(ni) != 0u) {
            continue;
        }
        walk_chain(walk, wdir, w, wi, hi, ni, root, 2u, vis, &max_c);
    }
    for (;;) {
        bool attach = false;
        for (u32 k = 0; k < en; ++k) {
            const u32 i = et[k];
            if (walk.rd_i(i) != 0u) {
                continue;
            }
            u32 best_n = 0xFFFFFFFFu;
            u16 mx = 0;
            for (i32 d = 0; d < 4; ++d) {
                const u32 ni = et_step4(w, wi, hi, i, d, vis, 0xFFFFFFFFu);
                if (ni == 0xFFFFFFFFu) {
                    continue;
                }
                const u16 wv = walk.rd_i(ni);
                if (wv == 0u || wv <= mx) {
                    continue;
                }
                mx = wv;
                best_n = ni;
            }
            if (mx == 0u || best_n == 0xFFFFFFFFu) {
                continue;
            }
            const u16 nxt = static_cast<u16>(mx + 1u);
            wr_walk_step(walk, i, nxt);
            wr_wdir(wdir, w, best_n, i);
            if (nxt > max_c) {
                max_c = nxt;
            }
            attach = true;
        }
        if (!attach) {
            break;
        }
    }
    return max_c;
}

static u32 flood_patch (
    const GameArraySimple& map,
    Whiteboard_2B& patch,
    u16 pid,
    u32 seed_i,
    u32* q)
{
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const u16 sx = static_cast<u16>(seed_i % wi);
    const u16 sy = static_cast<u16>(seed_i / wi);
    if (!is_mtn(map.get_terrain(sx, sy)) || patch.rd_i(seed_i) != GFL_IDX_NONE) {
        return 0u;
    }
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    u32 qn = 0;
    patch.wr_i(seed_i, pid);
    q[qn++] = seed_i;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u32 py = i / wi;
        const u32 px = i - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
            if (patch.rd_i(ni) != GFL_IDX_NONE) {
                continue;
            }
            if (!is_patch_fill(map.get_terrain(static_cast<u16>(nx), static_cast<u16>(ny)))) {
                continue;
            }
            patch.wr_i(ni, pid);
            q[qn++] = ni;
        }
    }
    return qn;
}

static bool pick_pass_fort (
    const GameArraySimple& map,
    const u32* path,
    u32 pn,
    u16 w,
    u16* ox,
    u16* oy)
{
    if (pn == 0u) {
        return false;
    }
    const u32 wi = static_cast<u32>(w);
    const u32 mid = pn / 2u;
    for (u32 d = 0u; d < pn; ++d) {
        u32 i = mid;
        if (d != 0u) {
            const u32 off = (d + 1u) / 2u;
            i = (d & 1u) ? mid + off : mid - off;
        }
        if (i >= pn) {
            continue;
        }
        const u32 ti = path[i];
        const u16 x = static_cast<u16>(ti % wi);
        const u16 y = static_cast<u16>(ti / wi);
        if (map.get_res(x, y) == U16_KEY_NULL) {
            *ox = x;
            *oy = y;
            return true;
        }
    }
    return false;
}

static u32 stamp_pass (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    const Whiteboard_2B& walk,
    Whiteboard_1B& pass,
    Whiteboard_1B& pass_fort,
    u32* pass_fort_n,
    u16 pid,
    u16 max_c,
    u16 step_a,
    u16 step_b,
    u16 ax,
    u16 ay,
    u16 bx,
    u16 by,
    u32* q,
    u32* par)
{
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 wi = static_cast<u32>(w);
    const i32 r = static_cast<i32>(GFL_WIN_R);
    const i32 x0 = static_cast<i32>(bx) - r;
    const i32 y0 = static_cast<i32>(by) - r;
    const i32 x1 = static_cast<i32>(bx) + r;
    const i32 y1 = static_cast<i32>(by) + r;
    auto in_win = [&] (i32 x, i32 y) {
        return x >= x0 && x <= x1 && y >= y0 && y <= y1;
    };
    for (i32 wy = y0; wy <= y1; ++wy) {
        for (i32 wx = x0; wx <= x1; ++wx) {
            if (wx < 0 || wy < 0 || wx >= static_cast<i32>(w) || wy >= static_cast<i32>(h)) {
                continue;
            }
            par[tidx(w, static_cast<u32>(wx), static_cast<u32>(wy))] = 0xFFFFFFFFu;
        }
    }
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    u32 qn = 0;
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(ax) + dx;
            const i32 ny = static_cast<i32>(ay) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if ((ux == ax && uy == ay) || (ux == bx && uy == by)) {
                continue;
            }
            if (patch.rd(ux, uy) != pid || !is_mtn(map.get_terrain(ux, uy)) || pass.rd(ux, uy) != 0u) {
                continue;
            }
            const u32 ti = tidx(w, static_cast<u32>(ux), static_cast<u32>(uy));
            par[ti] = ti;
            q[qn++] = ti;
        }
    }
    if (qn == 0u) {
        return 0u;
    }
    u32 goal = 0xFFFFFFFFu;
    for (u32 qh = 0; qh < qn && goal == 0xFFFFFFFFu; ++qh) {
        const u32 i = q[qh];
        bool on_b = false;
        if (mtn_adj_open_not_a(map, patch, walk, pid, w, h, i, max_c, step_a, step_b, &on_b)) {
            if (on_b) {
                goal = i;
            }
            continue;
        }
        const u32 py = i / wi;
        const u32 px = i - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (!in_win(nx, ny) || nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if ((ux == ax && uy == ay) || (ux == bx && uy == by)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u32>(ux), static_cast<u32>(uy));
            if (patch.rd(ux, uy) != pid || !is_mtn(map.get_terrain(ux, uy)) || pass.rd(ux, uy) != 0u) {
                continue;
            }
            if (par[ni] != 0xFFFFFFFFu) {
                continue;
            }
            par[ni] = i;
            q[qn++] = ni;
        }
    }
    if (goal == 0xFFFFFFFFu) {
        return 0u;
    }
    u32 path[GFL_PATH_MAX];
    u32 pn = 0;
    u32 cur = goal;
    while (pn < GFL_PATH_MAX) {
        path[pn++] = cur;
        if (par[cur] == cur) {
            break;
        }
        cur = par[cur];
    }
    if (pn == 0u) {
        return 0u;
    }
    for (u32 a = 0; a < pn / 2u; ++a) {
        const u32 t = path[a];
        path[a] = path[pn - 1u - a];
        path[pn - 1u - a] = t;
    }
    pn = trim_a_side_path(map, patch, walk, pid, w, h, max_c, step_a, path, pn);
    if (pn == 0u) {
        return 0u;
    }
    u32 added = 0;
    for (u32 k = 0; k < pn; ++k) {
        const u32 ti = path[k];
        const u16 x = static_cast<u16>(ti % wi);
        const u16 y = static_cast<u16>(ti / wi);
        if (pass.rd(x, y) == 0u) {
            pass.wr(x, y, 1u);
            ++added;
        }
    }
    u16 fx = 0;
    u16 fy = 0;
    if (pick_pass_fort(map, path, pn, w, &fx, &fy) && pass_fort.rd(fx, fy) == 0u) {
        pass_fort.wr(fx, fy, 1u);
        if (pass_fort_n != nullptr) {
            *pass_fort_n = *pass_fort_n + 1u;
        }
    }
    return added;
}

static bool pass_pair_ok (u16 s, u16 ws, u16 max_c) {
    if (ws == 0u || ws >= s) {
        return false;
    }
    const u16 da = static_cast<u16>(s - ws);
    if (da < static_cast<u16>(GFL_MIN_STEP_DELTA)) {
        return false;
    }
    if (max_c <= da) {
        return false;
    }
    if (static_cast<u16>(max_c - da) < static_cast<u16>(GFL_MIN_STEP_DELTA)) {
        return false;
    }
    return true;
}

static u32 scan_passes (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    const Whiteboard_2B& walk,
    const Whiteboard_1B& wdir,
    Whiteboard_1B& pass,
    Whiteboard_1B& pass_fort,
    u32* pass_fort_n,
    u16 pid,
    u16 max_c,
    const u8* vis,
    const u32* step_tile,
    u32* q,
    u32* par)
{
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 wi = static_cast<u32>(w);
    u32 added = 0;
    u16 cool_until = 0;
    u16 last_s = 0;
    const i32 r = static_cast<i32>(GFL_WIN_R);
    for (u16 s = 1u; s <= max_c; ++s) {
        if (s < cool_until) {
            last_s = 0;
            continue;
        }
        const u32 ti = step_tile[s];
        if (ti == 0u) {
            last_s = 0;
            continue;
        }
        const u16 cx = static_cast<u16>(ti % wi);
        const u16 cy = static_cast<u16>(ti / wi);
        if (vis[ti] < GFL_VIS_ET_MTN) {
            last_s = 0;
            continue;
        }
        if (!perim_mtn_adj8(map, pass, cx, cy)) {
            last_s = 0;
            continue;
        }
        const u8 mv = wdir.rd(cx, cy);
        const bool part = (last_s != 0u && s == static_cast<u16>(last_s + 1u) && mv != GFL_DIR_NONE);
        bool hit = false;
        for (i32 dy = -r; dy <= r && !hit; ++dy) {
            for (i32 dx = -r; dx <= r && !hit; ++dx) {
                if (part && !is_new_win_cell(dx, dy, mv, r)) {
                    continue;
                }
                const i32 nx = static_cast<i32>(cx) + dx;
                const i32 ny = static_cast<i32>(cy) + dy;
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                const u32 ji = tidx(w, static_cast<u32>(ux), static_cast<u32>(uy));
                if (!is_et_vis(vis[ji])) {
                    continue;
                }
                if (vis[ji] < GFL_VIS_ET_MTN) {
                    continue;
                }
                const u16 ws = walk.rd(ux, uy);
                if (!pass_pair_ok(s, ws, max_c)) {
                    continue;
                }
                if (!perim_mtn_adj8(map, pass, ux, uy)) {
                    continue;
                }
                added += stamp_pass(map, patch, walk, pass, pass_fort, pass_fort_n, pid, max_c, ws, s, ux, uy, cx, cy, q, par);
                cool_until = static_cast<u16>(s + static_cast<u16>(GFL_PASS_COOL));
                hit = true;
            }
        }
        last_s = s;
    }
    return added;
}

//================================================================================================================================
//=> - GenMtnPasses -
//================================================================================================================================

GenMtnPasses::GenMtnPasses () :
    m_patch("GenMtnPasses", "patch", 0u),
    m_walk("GenMtnPasses", "walk", 0u),
    m_wdir("GenMtnPasses", "wdir", 0u),
    m_pass("GenMtnPasses", "pass", 0u),
    m_pass_fort("GenMtnPasses", "pass_fort", 0u),
    m_psz(nullptr),
    m_psz_cap(0),
    m_patch_n(0),
    m_pass_n(0),
    m_pass_fort_n(0),
    m_max_walk(0),
    m_ok(false) {
}

GenMtnPasses::~GenMtnPasses () {
    clr_psz();
}

void GenMtnPasses::clr_psz () {
    delete[] m_psz;
    m_psz = nullptr;
    m_psz_cap = 0;
}

void GenMtnPasses::grow_psz (u16 need_n) {
    if (need_n <= m_psz_cap) {
        return;
    }
    u16 cap = m_psz_cap == 0u ? need_n : static_cast<u16>(m_psz_cap * 2u);
    if (cap < need_n) {
        cap = need_n;
    }
    u32* np = new u32[static_cast<u32>(cap) + 1u]();
    if (m_psz != nullptr) {
        std::memcpy(np, m_psz, (static_cast<u32>(m_psz_cap) + 1u) * sizeof(u32));
        delete[] m_psz;
    }
    m_psz = np;
    m_psz_cap = cap;
}

bool GenMtnPasses::begin (const GameArraySimple& map) {
    (void)map;
    m_ok = m_patch.ok() && m_walk.ok() && m_wdir.ok() && m_pass.ok() && m_pass_fort.ok();
    GAME_EXPECT(m_ok, "GenMtnPasses begin whiteboard checkout failed");
    clr();
    return m_ok;
}

void GenMtnPasses::clr () {
    m_patch_n = 0;
    m_pass_n = 0;
    m_pass_fort_n = 0;
    m_max_walk = 0;
    clr_psz();
    if (!m_patch.ok() || !m_walk.ok() || !m_wdir.ok() || !m_pass.ok() || !m_pass_fort.ok()) {
        m_ok = false;
        return;
    }
    const u32 n = WhiteboardMng::tile_n();
    std::memset(m_patch.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(m_walk.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(m_wdir.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
    std::memset(m_pass.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
    std::memset(m_pass_fort.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
}

bool GenMtnPasses::ok () const {
    return m_ok;
}

u16 GenMtnPasses::patch_n () const {
    return m_patch_n;
}

u32 GenMtnPasses::pass_n () const {
    return m_pass_n;
}

u32 GenMtnPasses::pass_fort_n () const {
    return m_pass_fort_n;
}

u32 GenMtnPasses::patch_sz (u16 pid) const {
    if (m_psz == nullptr || pid == 0u || pid > m_patch_n) {
        return 0u;
    }
    return m_psz[pid];
}

u16 GenMtnPasses::max_walk () const {
    return m_max_walk;
}

const Whiteboard_2B& GenMtnPasses::patches () const {
    return m_patch;
}

const Whiteboard_2B& GenMtnPasses::walks () const {
    return m_walk;
}

const Whiteboard_1B& GenMtnPasses::passes () const {
    return m_pass;
}

const Whiteboard_1B& GenMtnPasses::pass_forts () const {
    return m_pass_fort;
}

bool GenMtnPasses::index_patches (const GameArraySimple& map) {
    GAME_EXPECT_RET(m_ok, false, "GenMtnPasses index_patches not begun");
    GAME_EXPECT_RET(map.width() == m_patch.w() && map.height() == m_patch.h(), false,
        "GenMtnPasses index_patches map size mismatch");
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* q = new u32[n];
    std::memset(m_patch.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    u16 idx = 0;
    for (u32 i = 0; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (!is_mtn(map.get_terrain(x, y)) || m_patch.rd_i(i) != GFL_IDX_NONE) {
            continue;
        }
        if (idx == U16_KEY_NULL) {
            delete[] q;
            return false;
        }
        idx = static_cast<u16>(idx + 1u);
        grow_psz(idx);
        const u32 sz = flood_patch(map, m_patch, idx, i, q);
        if (sz == 0u) {
            delete[] q;
            return false;
        }
        m_psz[idx] = sz;
    }
    m_patch_n = idx;
    delete[] q;
    return true;
}

bool GenMtnPasses::proc_patch (const GameArraySimple& map, u16 pid, u32* q, u8* vis, u32* par) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 psz = m_psz[pid];
    if (psz < static_cast<u32>(GFL_MIN_PATCH_TILES)) {
        return true;
    }
    u32* core = new u32[psz];
    u32 cn = 0;
    for (u32 i = 0; i < n; ++i) {
        if (m_patch.rd_i(i) == pid) {
            core[cn++] = i;
        }
    }
    u32* et = new u32[cn * 8u];
    u32 en = 0;
    std::memset(vis, 0, static_cast<size_t>(n));
    for (u32 k = 0; k < cn; ++k) {
        const u32 ci = core[k];
        const u16 cx = static_cast<u16>(ci % static_cast<u32>(w));
        const u16 cy = static_cast<u16>(ci / static_cast<u32>(w));
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) {
                    continue;
                }
                const i32 nx = static_cast<i32>(cx) + dx;
                const i32 ny = static_cast<i32>(cy) + dy;
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                if (!is_edge_tile(map, m_patch, pid, ux, uy)) {
                    continue;
                }
                const u32 ni = tidx(w, static_cast<u32>(ux), static_cast<u32>(uy));
                if (vis[ni] != 0u) {
                    continue;
                }
                vis[ni] = GFL_VIS_ET;
                et[en++] = ni;
            }
        }
    }
    delete[] core;
    if (en == 0u) {
        delete[] et;
        return true;
    }
    for (u32 k = 0; k < en; ++k) {
        const u32 ei = et[k];
        if (perim_mtn_adj8_terr(map, static_cast<u16>(ei % static_cast<u32>(w)),
                static_cast<u16>(ei / static_cast<u32>(w)))) {
            vis[ei] = GFL_VIS_ET_MTN;
        }
    }
    u32* step_tile = new u32[static_cast<u32>(en) + 1u];
    std::memset(step_tile, 0, (static_cast<u32>(en) + 1u) * sizeof(u32));
    for (u32 k = 0; k < en; ++k) {
        m_walk.wr_i(et[k], 0u);
        m_wdir.wr_i(et[k], GFL_DIR_NONE);
    }
    const u16 max_c = walk_perim(m_walk, m_wdir, w, h, et, en, vis, et[0]);
    fill_step_tile(et, en, m_walk, step_tile);
    if (max_c > m_max_walk) {
        m_max_walk = max_c;
    }
    m_pass_n += scan_passes(map, m_patch, m_walk, m_wdir, m_pass, m_pass_fort, &m_pass_fort_n, pid, max_c, vis, step_tile, q, par);
    delete[] step_tile;
    delete[] et;
    return true;
}

bool GenMtnPasses::find_passes (const GameArraySimple& map) {
    GAME_EXPECT_RET(m_ok, false, "GenMtnPasses find_passes not begun");
    GAME_EXPECT_RET(m_patch_n > 0u, false, "GenMtnPasses find_passes no patches");
    GAME_EXPECT_RET(map.width() == m_patch.w() && map.height() == m_patch.h(), false,
        "GenMtnPasses find_passes map size mismatch");
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(m_walk.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(m_wdir.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
    std::memset(m_pass.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
    std::memset(m_pass_fort.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
    m_pass_n = 0;
    m_pass_fort_n = 0;
    m_max_walk = 0;
    u32* q = new u32[n];
    u8* vis = new u8[n];
    u32* par = new u32[n];
    for (u16 pid = 1u; pid <= m_patch_n; ++pid) {
        if (!proc_patch(map, pid, q, vis, par)) {
            delete[] par;
            delete[] vis;
            delete[] q;
            return false;
        }
    }
    delete[] par;
    delete[] vis;
    delete[] q;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
