//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_fort_locations_mk1.h"

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

#define GFL_MIN_PATCH_TILES 50
#define GFL_MIN_STEP_DELTA 20
#define GFL_PASS_SKIP_START 3
#define GFL_WATER_GRACE 20
#define GFL_WIN_R 2 

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

static bool is_land (u8 terr) {
    if (terr == TERR_NONE[0]) {
        return false;
    }
    return !overlay_is_water_terr(terr);
}

static bool is_land_non_mtn (u8 terr) {
    return is_land(terr) && !is_mtn(terr);
}

static u32 tidx (u16 w, u32 x, u32 y) {
    return y * static_cast<u32>(w) + x;
}

static bool is_walk_land (const GameArraySimple& map, const Whiteboard_1B& pass, u16 x, u16 y) {
    if (pass.rd(x, y) != 0u) {
        return true;
    }
    return is_land_non_mtn(map.get_terrain(x, y));
}

static bool mtn_adj8 (const GameArraySimple& map, const Whiteboard_1B& pass, u16 x, u16 y) {
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

static bool is_shore_land (const GameArraySimple& map, const Whiteboard_1B& pass, u16 x, u16 y) {
    return is_walk_land(map, pass, x, y) && mtn_adj8(map, pass, x, y);
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

static bool tile_borders_patch_core (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    const Whiteboard_1B& pass,
    u16 pid,
    u16 x,
    u16 y)
{
    if (patch_core_at(map, patch, pid, x, y)) {
        return true;
    }
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
            if (patch_core_at(map, patch, pid, ux, uy)) {
                return true;
            }
            if (pass.rd(ux, uy) == 0u) {
                continue;
            }
            for (i32 dy2 = -1; dy2 <= 1; ++dy2) {
                for (i32 dx2 = -1; dx2 <= 1; ++dx2) {
                    if (dx2 == 0 && dy2 == 0) {
                        continue;
                    }
                    const i32 nx2 = nx + dx2;
                    const i32 ny2 = ny + dy2;
                    if (nx2 < 0 || ny2 < 0 || nx2 >= static_cast<i32>(w) || ny2 >= static_cast<i32>(h)) {
                        continue;
                    }
                    if (patch_core_at(map, patch, pid, static_cast<u16>(nx2), static_cast<u16>(ny2))) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

static bool adj_water8 (const GameArraySimple& map, u16 x, u16 y) {
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
            if (overlay_is_water_terr(map.get_terrain(static_cast<u16>(nx), static_cast<u16>(ny)))) {
                return true;
            }
        }
    }
    return false;
}

static bool is_wedge_land (const GameArraySimple& map, const Whiteboard_1B& pass, u16 x, u16 y) {
    if (!is_walk_land(map, pass, x, y)) {
        return false;
    }
    if (overlay_is_water_terr(map.get_terrain(x, y))) {
        return false;
    }
    return adj_water8(map, x, y);
}

static bool is_shore_for_patch (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    const Whiteboard_1B& pass,
    u16 pid,
    u16 x,
    u16 y)
{
    if (!is_walk_land(map, pass, x, y)) {
        return false;
    }
    return tile_borders_patch_core(map, patch, pass, pid, x, y);
}

static bool flood_mtn (
    const GameArraySimple& map,
    Whiteboard_2B& ov,
    u16 idx,
    u32 seed_i,
    u32* q)
{
    GAME_EXPECT(q != nullptr, "GenFortLocations flood_mtn got nullptr q");
    if (ov.rd_i(seed_i) != GFL_IDX_NONE) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const u16 sx = static_cast<u16>(seed_i % wi);
    const u16 sy = static_cast<u16>(seed_i / wi);
    if (!is_mtn(map.get_terrain(sx, sy))) {
        return false;
    }
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    u32 qn = 0;
    ov.wr_i(seed_i, idx);
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
            if (ov.rd_i(ni) != GFL_IDX_NONE) {
                continue;
            }
            if (!is_patch_fill(map.get_terrain(static_cast<u16>(nx), static_cast<u16>(ny)))) {
                continue;
            }
            ov.wr_i(ni, idx);
            q[qn++] = ni;
        }
    }
    return true;
}

static bool flood_edge (
    const GameArraySimple& map,
    Whiteboard_2B& edge,
    Whiteboard_2B& frag,
    u16 idx,
    u32 seed_i,
    u32* q)
{
    GAME_EXPECT(q != nullptr, "GenFortLocations flood_edge got nullptr q");
    if (edge.rd_i(seed_i) == 0u || frag.rd_i(seed_i) != GFL_IDX_NONE) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    u32 qn = 0;
    frag.wr_i(seed_i, idx);
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
            if (edge.rd_i(ni) == 0u || frag.rd_i(ni) != GFL_IDX_NONE) {
                continue;
            }
            frag.wr_i(ni, idx);
            q[qn++] = ni;
        }
    }
    return true;
}

static u8 frag_deg8 (const Whiteboard_2B& memb, u16 fid, u16 w, u16 h, u32 i) {
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const u32 py = i / wi;
    const u32 px = i - py * wi;
    u8 deg = 0;
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(px) + dx;
            const i32 ny = static_cast<i32>(py) + dy;
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            if (memb.rd(static_cast<u32>(nx), static_cast<u32>(ny)) == fid) {
                deg = static_cast<u8>(deg + 1u);
            }
        }
    }
    return deg;
}

static bool in_et_tile (u32 i, const u32* et, u32 en) {
    for (u32 k = 0; k < en; ++k) {
        if (et[k] == i) {
            return true;
        }
    }
    return false;
}

static u32 et_step4 (
    u16 w,
    u32 wi,
    u32 hi,
    u32 i,
    i32 d,
    const u32* et,
    u32 en,
    const Whiteboard_2B& frag,
    u16 fid,
    u32 skip)
{
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
    if (ni == skip) {
        return 0xFFFFFFFFu;
    }
    if (!in_et_tile(ni, et, en) || frag.rd_i(ni) != fid) {
        return 0xFFFFFFFFu;
    }
    return ni;
}

static u8 et_deg4 (
    u16 w,
    u32 wi,
    u32 hi,
    u32 i,
    const u32* et,
    u32 en,
    const Whiteboard_2B& frag,
    u16 fid)
{
    u8 deg = 0;
    for (i32 d = 0; d < 4; ++d) {
        if (et_step4(w, wi, hi, i, d, et, en, frag, fid, 0xFFFFFFFFu) != 0xFFFFFFFFu) {
            deg = static_cast<u8>(deg + 1u);
        }
    }
    return deg;
}

static void push_walk_nbrs (
    const GameArraySimple& map,
    const Whiteboard_1B& pass,
    const Whiteboard_2B& patch,
    const Whiteboard_2B& memb,
    Whiteboard_2B& walk,
    u16 fid,
    u16 pid,
    u32 i,
    u16 cur,
    u32* q,
    u32* qn,
    u16* max_c,
    u32* grace_n)
{
    static const i32 k_card[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    const u16 w = memb.w();
    const u16 h = memb.h();
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const u32 py = i / wi;
    const u32 px = i - py * wi;
    const u16 sx = static_cast<u16>(px);
    const u16 sy = static_cast<u16>(py);
    const u32 qn0 = *qn;
    for (i32 d = 0; d < 4; ++d) {
        const i32 nx = static_cast<i32>(px) + k_card[d][0];
        const i32 ny = static_cast<i32>(py) + k_card[d][1];
        if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        const u32 ni = tidx(w, static_cast<u32>(ux), static_cast<u32>(uy));
        if (walk.rd_i(ni) != 0u || memb.rd_i(ni) != fid) {
            continue;
        }
        const u16 nxt = static_cast<u16>(cur + 1u);
        walk.wr_i(ni, nxt);
        if (nxt > *max_c) {
            *max_c = nxt;
        }
        q[(*qn)++] = ni;
    }
    if (*qn > qn0 || pid == 0u || !adj_water8(map, sx, sy)) {
        return;
    }
    if (*grace_n >= static_cast<u32>(GFL_WATER_GRACE)) {
        return;
    }
    for (i32 d = 0; d < 4; ++d) {
        const i32 nx = static_cast<i32>(px) + k_card[d][0];
        const i32 ny = static_cast<i32>(py) + k_card[d][1];
        if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        const u32 ni = tidx(w, static_cast<u32>(ux), static_cast<u32>(uy));
        if (walk.rd_i(ni) != 0u) {
            continue;
        }
        if (is_shore_for_patch(map, patch, pass, pid, ux, uy)) {
            const u16 nxt = static_cast<u16>(cur + 1u);
            walk.wr_i(ni, nxt);
            if (nxt > *max_c) {
                *max_c = nxt;
            }
            q[(*qn)++] = ni;
            *grace_n = *grace_n + 1u;
            continue;
        }
        if (!is_wedge_land(map, pass, ux, uy)) {
            continue;
        }
        const u16 nxt = static_cast<u16>(cur + 1u);
        walk.wr_i(ni, nxt);
        if (nxt > *max_c) {
            *max_c = nxt;
        }
        q[(*qn)++] = ni;
        *grace_n = *grace_n + 1u;
    }
}

static void walk_et_chain (
    Whiteboard_2B& walk,
    u16 w,
    u32 wi,
    u32 hi,
    u32 start,
    u32 from,
    u16 start_step,
    const u32* et,
    u32 en,
    const Whiteboard_2B& frag,
    u16 fid,
    u16* max_c)
{
    u32 prev = from;
    u32 cur = start;
    u16 step = start_step;
    walk.wr_i(cur, step);
    if (step > *max_c) {
        *max_c = step;
    }
    for (;;) {
        u32 nxt = 0xFFFFFFFFu;
        for (i32 d = 0; d < 4; ++d) {
            const u32 ni = et_step4(w, wi, hi, cur, d, et, en, frag, fid, prev);
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
        walk.wr_i(nxt, step);
        if (step > *max_c) {
            *max_c = step;
        }
        prev = cur;
        cur = nxt;
    }
}

static u16 walk_fill_etile (
    const GameArraySimple& map,
    const Whiteboard_1B& pass,
    const Whiteboard_2B& patch,
    Whiteboard_2B& memb,
    Whiteboard_2B& walk,
    u16 fid,
    u16 pid,
    u32 seed_i,
    const u32* et,
    u32 en,
    u32* q)
{
    GAME_EXPECT(q != nullptr, "GenFortLocations walk_fill_etile got nullptr q");
    const u16 w = memb.w();
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(memb.h());
    u32 qn = 0;
    u16 max_c = 0;
    u32 grace_n = 0;
    u32 ep = 0xFFFFFFFFu;
    for (u32 k = 0; k < en; ++k) {
        if (et_deg4(w, wi, hi, et[k], et, en, memb, fid) == 1u) {
            ep = et[k];
            break;
        }
    }
    const u32 root = ep != 0xFFFFFFFFu ? ep : seed_i;
    if (ep != 0xFFFFFFFFu) {
        walk_et_chain(walk, w, wi, hi, root, 0xFFFFFFFFu, 1u, et, en, memb, fid, &max_c);
    } else {
        walk.wr_i(root, 1u);
        max_c = 1u;
        for (i32 d = 0; d < 4; ++d) {
            const u32 ni = et_step4(w, wi, hi, root, d, et, en, memb, fid, 0xFFFFFFFFu);
            if (ni == 0xFFFFFFFFu || walk.rd_i(ni) != 0u) {
                continue;
            }
            walk_et_chain(walk, w, wi, hi, ni, root, 2u, et, en, memb, fid, &max_c);
        }
    }
    auto drain_q = [&] (u32 qstart) {
        for (u32 qh = qstart; qh < qn; ++qh) {
            const u32 i = q[qh];
            const u16 cur = walk.rd_i(i);
            push_walk_nbrs(map, pass, patch, memb, walk, fid, pid, i, cur, q, &qn, &max_c, &grace_n);
        }
    };
    for (;;) {
        bool attach = false;
        const u32 qstart = qn;
        for (u32 k = 0; k < en; ++k) {
            const u32 i = et[k];
            if (walk.rd_i(i) != 0u) {
                continue;
            }
            const u32 py = i / wi;
            const u32 px = i - py * wi;
            u16 mx = 0;
            static const i32 dx4[4] = {-1, 1, 0, 0};
            static const i32 dy4[4] = {0, 0, -1, 1};
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + dx4[d];
                const i32 ny = static_cast<i32>(py) + dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
                const u16 wv = walk.rd_i(ni);
                if (wv == 0u || wv <= mx) {
                    continue;
                }
                mx = wv;
            }
            if (mx == 0u) {
                continue;
            }
            const u16 nxt = static_cast<u16>(mx + 1u);
            walk.wr_i(i, nxt);
            if (nxt > max_c) {
                max_c = nxt;
            }
            q[qn++] = i;
            attach = true;
        }
        if (!attach) {
            break;
        }
        grace_n = 0;
        drain_q(qstart);
    }
    for (u32 k = 0; k < en; ++k) {
        const u32 i = et[k];
        if (walk.rd_i(i) != 0u) {
            continue;
        }
        const u16 base = max_c;
        walk.wr_i(i, static_cast<u16>(base + 1u));
        qn = 0;
        q[qn++] = i;
        if (base + 1u > max_c) {
            max_c = static_cast<u16>(base + 1u);
        }
        grace_n = 0;
        drain_q(0);
    }
    return max_c;
}

static u16 walk_one (
    const GameArraySimple& map,
    const Whiteboard_1B& pass,
    const Whiteboard_2B& patch,
    Whiteboard_2B& memb,
    Whiteboard_2B& walk,
    u16 fid,
    u16 pid,
    u32 seed_i,
    u32* q)
{
    GAME_EXPECT(q != nullptr, "GenFortLocations walk_one got nullptr q");
    u32 qn = 0;
    walk.wr_i(seed_i, 1u);
    q[qn++] = seed_i;
    u16 max_c = 1u;
    u32 grace_n = 0;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 cur = walk.rd_i(i);
        push_walk_nbrs(map, pass, patch, memb, walk, fid, pid, i, cur, q, &qn, &max_c, &grace_n);
    }
    return max_c;
}

static bool in_win (u16 x, u16 y, u16 cx, u16 cy) {
    const i32 dx = static_cast<i32>(x) - static_cast<i32>(cx);
    const i32 dy = static_cast<i32>(y) - static_cast<i32>(cy);
    return dx >= -GFL_WIN_R && dx <= GFL_WIN_R && dy >= -GFL_WIN_R && dy <= GFL_WIN_R;
}

static bool mtn_pass_ok (
    const GameArraySimple& map,
    const Whiteboard_2B& frag,
    u16 w,
    u16 h,
    u32 ni,
    u32 a_i,
    u32 b_i)
{
    const u16 x = static_cast<u16>(ni % static_cast<u32>(w));
    const u16 y = static_cast<u16>(ni / static_cast<u32>(w));
    if (!is_mtn(map.get_terrain(x, y))) {
        return true;
    }
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    const u32 wi = static_cast<u32>(w);
    const u32 py = ni / wi;
    const u32 px = ni - py * wi;
    for (i32 d = 0; d < 4; ++d) {
        const i32 nx = static_cast<i32>(px) + dx4[d];
        const i32 ny = static_cast<i32>(py) + dy4[d];
        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
            continue;
        }
        const u32 li = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
        if (li == a_i || li == b_i) {
            continue;
        }
        if (frag.rd_i(li) == GFL_IDX_NONE) {
            continue;
        }
        if (!is_land_non_mtn(map.get_terrain(static_cast<u16>(nx), static_cast<u16>(ny)))) {
            continue;
        }
        return false;
    }
    return true;
}

static u32 stamp_pass_path (
    const GameArraySimple& map,
    const Whiteboard_2B& frag,
    Whiteboard_1B& pass,
    u16 w,
    u16 h,
    u16 cx,
    u16 cy,
    u32 a_i,
    u32 b_i,
    u16 step_d,
    u32* q,
    u32* par)
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        par[i] = 0xFFFFFFFFu;
    }
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    u32 qn = 0;
    par[a_i] = a_i;
    q[qn++] = a_i;
    bool hit = false;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        if (i == b_i) {
            hit = true;
            break;
        }
        const u32 py = i / static_cast<u32>(w);
        const u32 px = i - py * static_cast<u32>(w);
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!in_win(ux, uy, cx, cy)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u32>(ux), static_cast<u32>(uy));
            if (par[ni] != 0xFFFFFFFFu) {
                continue;
            }
            const bool endpt = (ni == a_i || ni == b_i);
            const bool mtn = is_mtn(map.get_terrain(ux, uy));
            if (!endpt && !mtn) {
                continue;
            }
            if (mtn && !mtn_pass_ok(map, frag, w, h, ni, a_i, b_i)) {
                continue;
            }
            par[ni] = i;
            q[qn++] = ni;
        }
    }
    if (!hit || par[b_i] == 0xFFFFFFFFu) {
        return 0;
    }
    u32 path[25];
    u32 pn = 0;
    u32 cur = b_i;
    while (pn < 25u) {
        path[pn++] = cur;
        if (cur == a_i) {
            break;
        }
        const u32 p = par[cur];
        if (p == cur || p == 0xFFFFFFFFu) {
            return 0;
        }
        cur = p;
    }
    if (cur != a_i) {
        return 0;
    }
    u32 ma = 0xFFFFFFFFu;
    u32 mb = 0xFFFFFFFFu;
    for (u32 k = 0; k < pn; ++k) {
        const u16 x = static_cast<u16>(path[k] % static_cast<u32>(w));
        const u16 y = static_cast<u16>(path[k] / static_cast<u32>(w));
        if (is_mtn(map.get_terrain(x, y))) {
            mb = path[k];
            break;
        }
    }
    for (u32 k = pn; k > 0u; --k) {
        const u16 x = static_cast<u16>(path[k - 1u] % static_cast<u32>(w));
        const u16 y = static_cast<u16>(path[k - 1u] / static_cast<u32>(w));
        if (is_mtn(map.get_terrain(x, y))) {
            ma = path[k - 1u];
            break;
        }
    }
    if (ma == 0xFFFFFFFFu || mb == 0xFFFFFFFFu || ma == mb) {
        return 0;
    }
    u32 ib = 0xFFFFFFFFu;
    u32 ia = 0xFFFFFFFFu;
    for (u32 k = 0; k < pn; ++k) {
        if (path[k] == mb) {
            ib = k;
            break;
        }
    }
    for (u32 k = pn; k > 0u; --k) {
        if (path[k - 1u] == ma) {
            ia = k - 1u;
            break;
        }
    }
    if (ib == 0xFFFFFFFFu || ia == 0xFFFFFFFFu || ib > ia) {
        return 0;
    }
    u32 added = 0;
    for (u32 k = ib; k <= ia; ++k) {
        const u32 ti = path[k];
        const u16 x = static_cast<u16>(ti % static_cast<u32>(w));
        const u16 y = static_cast<u16>(ti / static_cast<u32>(w));
        if (!is_mtn(map.get_terrain(x, y)) || pass.rd_i(ti) != 0u) {
            continue;
        }
        const bool at_ep = (ti == ma || ti == mb);
        const bool tight = step_d < static_cast<u16>(GFL_PASS_SKIP_START);
        if ((!at_ep || !tight) && mtn_pass_ok(map, frag, w, h, ti, a_i, b_i)) {
            pass.wr_i(ti, 1u);
            ++added;
        }
    }
    return added;
}

static u32 flood_collect_patch (
    const GameArraySimple& map,
    const Whiteboard_2B& patch,
    u16 pid,
    u32 seed_i,
    u32* q,
    u8* vis,
    u32** out_t,
    u32** out_m,
    u32* out_tn,
    u32* out_mn)
{
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 wi = static_cast<u32>(w);
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    u32 qn = 0;
    q[qn++] = seed_i;
    vis[seed_i] = 1u;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u32 py = i / wi;
        const u32 px = i - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= h) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
            if (vis[ni] != 0u || patch.rd_i(ni) != pid) {
                continue;
            }
            vis[ni] = 1u;
            q[qn++] = ni;
        }
    }
    u32 mcsz = 0;
    for (u32 k = 0; k < qn; ++k) {
        const u16 x = static_cast<u16>(q[k] % wi);
        const u16 y = static_cast<u16>(q[k] / wi);
        if (is_mtn(map.get_terrain(x, y))) {
            mcsz = mcsz + 1u;
        }
    }
    u32* tiles = new u32[qn];
    u32* mtns = mcsz > 0u ? new u32[mcsz] : nullptr;
    u32 ti = 0;
    u32 mi = 0;
    for (u32 k = 0; k < qn; ++k) {
        const u32 i = q[k];
        tiles[ti++] = i;
        vis[i] = 0u;
        const u16 x = static_cast<u16>(i % wi);
        const u16 y = static_cast<u16>(i / wi);
        if (is_mtn(map.get_terrain(x, y))) {
            mtns[mi++] = i;
        }
    }
    *out_t = tiles;
    *out_m = mtns;
    *out_tn = qn;
    *out_mn = mcsz;
    return qn;
}

//================================================================================================================================
//=> - GenFortLocations -
//================================================================================================================================

GenFortLocationsMk1::GenFortLocationsMk1 () :
    m_patch("GenFortLocations", "patch", 0u),
    m_frag("GenFortLocations", "frag", 0u),
    m_edge("GenFortLocations", "edge", 0u),
    m_walk("GenFortLocations", "walk", 0u),
    m_pass("GenFortLocations", "pass", 0u),
    m_info(nullptr),
    m_psz(nullptr),
    m_ptile(nullptr),
    m_pmtn(nullptr),
    m_pmnn(nullptr),
    m_etile(nullptr),
    m_etn(nullptr),
    m_plist_cap(0),
    m_patch_n(0),
    m_frag_n(0),
    m_pass_n(0),
    m_ok(false) {
}

GenFortLocationsMk1::~GenFortLocationsMk1 () {
    clr_info();
    clr_psz();
    clr_ptiles();
    clr_etiles();
}

void GenFortLocationsMk1::clr_info () {
    delete[] m_info;
    m_info = nullptr;
}

void GenFortLocationsMk1::clr_psz () {
    delete[] m_psz;
    m_psz = nullptr;
}

void GenFortLocationsMk1::clr_ptiles () {
    if (m_ptile != nullptr) {
        for (u16 p = 1u; p <= m_plist_cap; ++p) {
            delete[] m_ptile[p];
            delete[] m_pmtn[p];
        }
        delete[] m_ptile;
        delete[] m_pmtn;
        m_ptile = nullptr;
        m_pmtn = nullptr;
    }
    delete[] m_pmnn;
    m_pmnn = nullptr;
    m_plist_cap = 0;
}

void GenFortLocationsMk1::clr_etiles () {
    if (m_etile != nullptr) {
        for (u16 p = 1u; p <= m_plist_cap; ++p) {
            delete[] m_etile[p];
        }
        delete[] m_etile;
        m_etile = nullptr;
    }
    delete[] m_etn;
    m_etn = nullptr;
}

void GenFortLocationsMk1::grow_plists (u16 need_n) {
    if (need_n <= m_plist_cap) {
        return;
    }
    u16 cap = m_plist_cap == 0u ? need_n : static_cast<u16>(m_plist_cap * 2u);
    if (cap < need_n) {
        cap = need_n;
    }
    u32** nt = new u32*[static_cast<u32>(cap) + 1u]();
    u32** nm = new u32*[static_cast<u32>(cap) + 1u]();
    u32** ne = new u32*[static_cast<u32>(cap) + 1u]();
    u32* nn = new u32[static_cast<u32>(cap) + 1u]();
    u32* nmn = new u32[static_cast<u32>(cap) + 1u]();
    u32* nen = new u32[static_cast<u32>(cap) + 1u]();
    for (u16 p = 1u; p <= m_plist_cap; ++p) {
        nt[p] = m_ptile != nullptr ? m_ptile[p] : nullptr;
        nm[p] = m_pmtn != nullptr ? m_pmtn[p] : nullptr;
        ne[p] = m_etile != nullptr ? m_etile[p] : nullptr;
        nn[p] = m_psz != nullptr ? m_psz[p] : 0u;
        nmn[p] = m_pmnn != nullptr ? m_pmnn[p] : 0u;
        nen[p] = m_etn != nullptr ? m_etn[p] : 0u;
    }
    delete[] m_ptile;
    delete[] m_pmtn;
    delete[] m_etile;
    delete[] m_psz;
    delete[] m_pmnn;
    delete[] m_etn;
    m_ptile = nt;
    m_pmtn = nm;
    m_etile = ne;
    m_psz = nn;
    m_pmnn = nmn;
    m_etn = nen;
    m_plist_cap = cap;
}

bool GenFortLocationsMk1::collect_patch_tiles (
    const GameArraySimple& map,
    u16 pid,
    u32 seed_i,
    u32* q,
    u8* vis)
{
    delete[] m_ptile[pid];
    delete[] m_pmtn[pid];
    m_ptile[pid] = nullptr;
    m_pmtn[pid] = nullptr;
    m_psz[pid] = 0;
    m_pmnn[pid] = 0;
    if (m_patch.rd_i(seed_i) != pid) {
        return false;
    }
    flood_collect_patch(map, m_patch, pid, seed_i, q, vis, &m_ptile[pid], &m_pmtn[pid], &m_psz[pid], &m_pmnn[pid]);
    return m_psz[pid] > 0u;
}

u32 GenFortLocationsMk1::split_patch (
    const GameArraySimple& map,
    u16 pid,
    u32* q,
    u8* vis,
    u16* out_new)
{
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 wi = static_cast<u32>(w);
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    const u32 tn = m_psz[pid];
    const u32* tiles = m_ptile[pid];
    for (u32 k = 0; k < tn; ++k) {
        vis[tiles[k]] = 0u;
    }
    for (u32 k = 0; k < tn; ++k) {
        const u32 i = tiles[k];
        if (m_patch.rd_i(i) == pid && m_pass.rd_i(i) != 0u) {
            m_patch.wr_i(i, GFL_IDX_NONE);
        }
    }
    u32* seeds = new u32[tn];
    u32* sizes = new u32[tn];
    u32 comp_n = 0;
    for (u32 k = 0; k < tn; ++k) {
        const u32 i = tiles[k];
        if (m_patch.rd_i(i) != pid || vis[i] != 0u) {
            continue;
        }
        const u8 lab = static_cast<u8>(comp_n + 1u);
        u32 qn = 0;
        u32 csz = 0;
        q[qn++] = i;
        vis[i] = lab;
        for (u32 qh = 0; qh < qn; ++qh) {
            const u32 ci = q[qh];
            ++csz;
            const u32 py = ci / wi;
            const u32 px = ci - py * wi;
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + dx4[d];
                const i32 ny = static_cast<i32>(py) + dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= h) {
                    continue;
                }
                const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
                if (vis[ni] != 0u || m_patch.rd_i(ni) != pid) {
                    continue;
                }
                vis[ni] = lab;
                q[qn++] = ni;
            }
        }
        seeds[comp_n] = i;
        sizes[comp_n] = csz;
        ++comp_n;
    }
    out_new[0] = pid;
    out_new[1] = GFL_IDX_NONE;
    if (comp_n == 0u) {
        delete[] m_ptile[pid];
        delete[] m_pmtn[pid];
        m_ptile[pid] = nullptr;
        m_pmtn[pid] = nullptr;
        m_psz[pid] = 0;
        m_pmnn[pid] = 0;
        delete[] seeds;
        delete[] sizes;
        return 0;
    }
    if (comp_n == 1u) {
        collect_patch_tiles(map, pid, seeds[0], q, vis);
        vis[seeds[0]] = 0u;
        delete[] seeds;
        delete[] sizes;
        return 1;
    }
    u32 best = 0;
    for (u32 c = 1; c < comp_n; ++c) {
        if (sizes[c] > sizes[best]) {
            best = c;
        }
    }
    u16 second = GFL_IDX_NONE;
    for (u32 c = 0; c < comp_n; ++c) {
        u16 cid = pid;
        if (c != best) {
            if (m_patch_n == U16_KEY_NULL) {
                delete[] seeds;
                delete[] sizes;
                return 0;
            }
            grow_plists(static_cast<u16>(m_patch_n + 1u));
            m_patch_n = static_cast<u16>(m_patch_n + 1u);
            cid = m_patch_n;
            if (second == GFL_IDX_NONE) {
                second = cid;
            }
        }
        const u8 lab = static_cast<u8>(c + 1u);
        u32 qn = 0;
        vis[seeds[c]] = 0u;
        m_patch.wr_i(seeds[c], cid);
        q[qn++] = seeds[c];
        for (u32 qh = 0; qh < qn; ++qh) {
            const u32 ci = q[qh];
            const u32 py = ci / wi;
            const u32 px = ci - py * wi;
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + dx4[d];
                const i32 ny = static_cast<i32>(py) + dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= h) {
                    continue;
                }
                const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
                if (vis[ni] != lab || m_patch.rd_i(ni) != pid) {
                    continue;
                }
                vis[ni] = 0u;
                m_patch.wr_i(ni, cid);
                q[qn++] = ni;
            }
        }
        collect_patch_tiles(map, cid, seeds[c], q, vis);
    }
    out_new[0] = pid;
    out_new[1] = second;
    delete[] seeds;
    delete[] sizes;
    return comp_n;
}

bool GenFortLocationsMk1::begin (const GameArraySimple& map) {
    (void)map;
    m_ok = m_patch.ok() && m_frag.ok() && m_edge.ok() && m_walk.ok() && m_pass.ok();
    GAME_EXPECT(m_ok, "GenFortLocations begin whiteboard checkout failed");
    clr();
    return m_ok;
}

void GenFortLocationsMk1::clr () {
    m_patch_n = 0;
    m_frag_n = 0;
    m_pass_n = 0;
    clr_info();
    clr_psz();
    clr_ptiles();
    clr_etiles();
    if (!m_patch.ok() || !m_frag.ok() || !m_edge.ok() || !m_walk.ok() || !m_pass.ok()) {
        m_ok = false;
        return;
    }
    const u32 n = WhiteboardMng::tile_n();
    std::memset(m_patch.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(m_frag.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(m_edge.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(m_walk.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(m_pass.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
}

bool GenFortLocationsMk1::ok () const {
    return m_ok;
}

u16 GenFortLocationsMk1::patch_n () const {
    return m_patch_n;
}

u16 GenFortLocationsMk1::frag_n () const {
    return m_frag_n;
}

u32 GenFortLocationsMk1::pass_n () const {
    return m_pass_n;
}

const Whiteboard_2B& GenFortLocationsMk1::patches () const {
    return m_patch;
}

const Whiteboard_2B& GenFortLocationsMk1::frags () const {
    return m_frag;
}

const Whiteboard_2B& GenFortLocationsMk1::edges () const {
    return m_edge;
}

const Whiteboard_2B& GenFortLocationsMk1::walks () const {
    return m_walk;
}

const Whiteboard_1B& GenFortLocationsMk1::passes () const {
    return m_pass;
}

const GflFragInfo* GenFortLocationsMk1::frag_info (u16 frag_id) const {
    if (m_info == nullptr || frag_id == 0u || frag_id > m_frag_n) {
        return nullptr;
    }
    return &m_info[frag_id];
}

bool GenFortLocationsMk1::index_patches (const GameArraySimple& map) {
    GAME_EXPECT_RET(m_ok, false, "GenFortLocations index_patches not begun");
    GAME_EXPECT_RET(map.width() == m_patch.w() && map.height() == m_patch.h(), false,
        "GenFortLocations index_patches map size mismatch");
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* q = new u32[n];
    u8* vis = new u8[n];
    std::memset(m_patch.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(vis, 0, static_cast<size_t>(n));
    u16 idx = 0;
    for (u32 i = 0; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (!is_mtn(map.get_terrain(x, y)) || m_patch.rd_i(i) != GFL_IDX_NONE) {
            continue;
        }
        if (idx == U16_KEY_NULL) {
            delete[] q;
            delete[] vis;
            return false;
        }
        idx = static_cast<u16>(idx + 1u);
        grow_plists(idx);
        if (!flood_mtn(map, m_patch, idx, i, q)) {
            delete[] q;
            delete[] vis;
            return false;
        }
        if (!collect_patch_tiles(map, idx, i, q, vis)) {
            delete[] q;
            delete[] vis;
            return false;
        }
    }
    m_patch_n = idx;
    delete[] vis;
    delete[] q;
    return true;
}

bool GenFortLocationsMk1::mark_frags (const GameArraySimple& map) {
    GAME_EXPECT_RET(m_ok, false, "GenFortLocations mark_frags not begun");
    GAME_EXPECT_RET(map.width() == m_frag.w() && map.height() == m_frag.h(), false,
        "GenFortLocations mark_frags map size mismatch");
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(m_frag.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(m_edge.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    for (u32 i = 0; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (is_shore_land(map, m_pass, x, y)) {
            m_edge.wr_i(i, 1u);
        }
    }
    u32* q = new u32[n];
    u16 idx = 0;
    for (u32 i = 0; i < n; ++i) {
        if (m_edge.rd_i(i) == 0u || m_frag.rd_i(i) != GFL_IDX_NONE) {
            continue;
        }
        if (idx == U16_KEY_NULL) {
            delete[] q;
            return false;
        }
        idx = static_cast<u16>(idx + 1u);
        if (!flood_edge(map, m_edge, m_frag, idx, i, q)) {
            delete[] q;
            return false;
        }
    }
    m_frag_n = idx;
    delete[] q;
    return true;
}

bool GenFortLocationsMk1::walk_frags (const GameArraySimple& map) {
    GAME_EXPECT_RET(m_ok, false, "GenFortLocations walk_frags not begun");
    GAME_EXPECT_RET(m_frag_n > 0u, false, "GenFortLocations walk_frags no fragments");
    GAME_EXPECT_RET(map.width() == m_frag.w() && map.height() == m_frag.h(), false,
        "GenFortLocations walk_frags map size mismatch");
    const u16 w = m_frag.w();
    const u16 h = m_frag.h();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(m_walk.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    clr_info();
    m_info = new GflFragInfo[static_cast<u32>(m_frag_n) + 1u]();
    u32* q = new u32[n];
    u8* has_end = new u8[static_cast<u32>(m_frag_n) + 1u];
    u8* has_any = new u8[static_cast<u32>(m_frag_n) + 1u];
    std::memset(has_end, 0, static_cast<u32>(m_frag_n) + 1u);
    std::memset(has_any, 0, static_cast<u32>(m_frag_n) + 1u);
    u32* end_i = new u32[static_cast<u32>(m_frag_n) + 1u];
    u32* any_i = new u32[static_cast<u32>(m_frag_n) + 1u];
    std::memset(end_i, 0, (static_cast<u32>(m_frag_n) + 1u) * sizeof(u32));
    std::memset(any_i, 0, (static_cast<u32>(m_frag_n) + 1u) * sizeof(u32));
    for (u32 i = 0; i < n; ++i) {
        const u16 fid = m_frag.rd_i(i);
        if (fid == GFL_IDX_NONE || fid > m_frag_n) {
            continue;
        }
        if (has_any[fid] == 0u) {
            has_any[fid] = 1u;
            any_i[fid] = i;
        }
        if (frag_deg8(m_frag, fid, w, h, i) == 1u && has_end[fid] == 0u) {
            has_end[fid] = 1u;
            end_i[fid] = i;
        }
    }
    for (u16 f = 1u; f <= m_frag_n; ++f) {
        if (has_any[f] == 0u) {
            continue;
        }
        const u32 si = has_end[f] != 0u ? end_i[f] : any_i[f];
        const u16 sx = static_cast<u16>(si % static_cast<u32>(w));
        const u16 sy = static_cast<u16>(si / static_cast<u32>(w));
        m_info[f].m_loop = has_end[f] == 0u ? 1u : 0u;
        m_info[f].m_sx = sx;
        m_info[f].m_sy = sy;
        m_info[f].m_pid = 0u;
        m_info[f].m_steps = walk_one(map, m_pass, m_patch, m_frag, m_walk, f, 0u, si, q);
    }
    delete[] q;
    delete[] has_end;
    delete[] has_any;
    delete[] end_i;
    delete[] any_i;
    return true;
}

bool GenFortLocationsMk1::walk_frag (const GameArraySimple& map, u16 f) {
    GAME_EXPECT_RET(m_ok, false, "GenFortLocations walk_frag not begun");
    GAME_EXPECT_RET(f > 0u && f <= m_frag_n, false, "GenFortLocations walk_frag bad frag");
    GAME_EXPECT_RET(m_info != nullptr, false, "GenFortLocations walk_frag no info");
    GAME_EXPECT_RET(map.width() == m_frag.w() && map.height() == m_frag.h(), false,
        "GenFortLocations walk_frag map size mismatch");
    const u16 w = m_frag.w();
    const u16 h = m_frag.h();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* q = new u32[n];
    const u32 en = m_etn[f];
    const u32* et = m_etile[f];
    if (et == nullptr || en == 0u) {
        m_info[f].m_loop = 1u;
        m_info[f].m_steps = 0u;
        delete[] q;
        return true;
    }
    u8 has_end = 0;
    u32 end_i = 0;
    u32 any_i = 0;
    for (u32 k = 0; k < en; ++k) {
        const u32 i = et[k];
        m_walk.wr_i(i, 0u);
        if (any_i == 0u) {
            any_i = i;
        }
        if (frag_deg8(m_frag, f, w, h, i) == 1u && has_end == 0u) {
            has_end = 1u;
            end_i = i;
        }
    }
    const u32 si = has_end != 0u ? end_i : any_i;
    m_info[f].m_loop = has_end == 0u ? 1u : 0u;
    m_info[f].m_sx = static_cast<u16>(si % static_cast<u32>(w));
    m_info[f].m_sy = static_cast<u16>(si / static_cast<u32>(w));
    const u16 pid = m_info[f].m_pid;
    m_info[f].m_steps = walk_fill_etile(map, m_pass, m_patch, m_frag, m_walk, f, pid, si, et, en, q);
    delete[] q;
    return true;
}

void GenFortLocationsMk1::grow_info (u16 need_n) {
    if (m_info != nullptr && need_n <= m_frag_n) {
        return;
    }
    const u16 cap = need_n > m_frag_n ? need_n : m_frag_n;
    GflFragInfo* ni = new GflFragInfo[static_cast<u32>(cap) + 1u]();
    if (m_info != nullptr) {
        std::memcpy(ni, m_info, (static_cast<u32>(m_frag_n) + 1u) * sizeof(GflFragInfo));
        delete[] m_info;
    }
    m_info = ni;
}

bool GenFortLocationsMk1::mark_patch_frags (const GameArraySimple& map, u16 pid, u32* q) {
    GAME_EXPECT_RET(m_ok, false, "GenFortLocations mark_patch_frags not begun");
    GAME_EXPECT_RET(map.width() == m_frag.w() && map.height() == m_frag.h(), false,
        "GenFortLocations mark_patch_frags map size mismatch");
    GAME_EXPECT_RET(q != nullptr, false, "GenFortLocations mark_patch_frags got nullptr q");
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 wi = static_cast<u32>(w);
    const u32 tn = m_psz[pid];
    const u32* tiles = m_ptile[pid];
    if (tiles == nullptr || tn == 0u) {
        return false;
    }
    u32* shore = new u32[tn * 8u];
    u32 sn = 0;
    for (u32 k = 0; k < tn; ++k) {
        const u32 pi = tiles[k];
        const u32 py = pi / wi;
        const u32 px = pi - py * wi;
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) {
                    continue;
                }
                const i32 nx = static_cast<i32>(px) + dx;
                const i32 ny = static_cast<i32>(py) + dy;
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= h) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                if (!is_shore_for_patch(map, m_patch, m_pass, pid, ux, uy)) {
                    continue;
                }
                const u32 ni = tidx(w, static_cast<u32>(ux), static_cast<u32>(uy));
                if (m_edge.rd_i(ni) != 0u) {
                    continue;
                }
                m_edge.wr_i(ni, 1u);
                m_frag.wr_i(ni, GFL_IDX_NONE);
                m_walk.wr_i(ni, 0u);
                shore[sn++] = ni;
            }
        }
    }
    const u16 fid = pid;
    grow_info(pid);
    for (u32 si = 0; si < sn; ++si) {
        const u32 i = shore[si];
        if (m_edge.rd_i(i) == 0u || m_frag.rd_i(i) != GFL_IDX_NONE) {
            continue;
        }
        if (!flood_edge(map, m_edge, m_frag, fid, i, q)) {
            delete[] shore;
            return false;
        }
    }
    delete[] m_etile[pid];
    m_etile[pid] = nullptr;
    m_etn[pid] = 0;
    u32 en = 0;
    for (u32 si = 0; si < sn; ++si) {
        if (m_frag.rd_i(shore[si]) == fid) {
            ++en;
        }
    }
    if (en > 0u) {
        u32* et = new u32[en];
        u32 ei = 0;
        for (u32 si = 0; si < sn; ++si) {
            const u32 i = shore[si];
            if (m_frag.rd_i(i) == fid) {
                et[ei++] = i;
            }
            m_edge.wr_i(i, 0u);
        }
        m_etile[pid] = et;
        m_etn[pid] = en;
    } else {
        for (u32 si = 0; si < sn; ++si) {
            m_edge.wr_i(shore[si], 0u);
        }
    }
    delete[] shore;
    m_info[pid].m_pid = pid;
    if (pid > m_frag_n) {
        m_frag_n = pid;
    }
    return true;
}

bool GenFortLocationsMk1::walk_patch_frags (const GameArraySimple& map, u16 pid) {
    return walk_frag(map, pid);
}

bool GenFortLocationsMk1::find_best_pass (
    const GameArraySimple& map,
    u16 pid,
    u16* out_d,
    u32* out_a,
    u32* out_b,
    u16* out_cx,
    u16* out_cy,
    u32* q,
    u32* par)
{
    (void)q;
    (void)par;
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 wi = static_cast<u32>(w);
    struct Hit {
        u16 m_fid;
        u16 m_step;
        u32 m_i;
    };
    Hit hits[25];
    u16 best_d = 0;
    u32 best_a = 0;
    u32 best_b = 0;
    u16 best_cx = 0;
    u16 best_cy = 0;
    const u32 mn = m_pmnn[pid];
    const u32* mt = m_pmtn[pid];
    if (mt == nullptr || mn == 0u) {
        *out_d = 0;
        *out_a = 0;
        *out_b = 0;
        *out_cx = 0;
        *out_cy = 0;
        return false;
    }
    for (u32 k = 0; k < mn; ++k) {
        const u32 i = mt[k];
        const u16 cx = static_cast<u16>(i % wi);
        const u16 cy = static_cast<u16>(i / wi);
        u32 hn = 0;
        for (i32 dy = -GFL_WIN_R; dy <= GFL_WIN_R; ++dy) {
            for (i32 dx = -GFL_WIN_R; dx <= GFL_WIN_R; ++dx) {
                const i32 nx = static_cast<i32>(cx) + dx;
                const i32 ny = static_cast<i32>(cy) + dy;
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
                if (m_frag.rd_i(ni) != pid) {
                    continue;
                }
                const u16 step = m_walk.rd_i(ni);
                if (step == 0u) {
                    continue;
                }
                hits[hn].m_fid = pid;
                hits[hn].m_step = step;
                hits[hn].m_i = ni;
                ++hn;
            }
        }
        for (u32 a = 0; a < hn; ++a) {
            for (u32 b = a + 1u; b < hn; ++b) {
                if (hits[a].m_fid != hits[b].m_fid) {
                    continue;
                }
                const i32 dlt = static_cast<i32>(hits[a].m_step) - static_cast<i32>(hits[b].m_step);
                const u16 ad = static_cast<u16>(dlt < 0 ? -dlt : dlt);
                if (ad < static_cast<u16>(GFL_MIN_STEP_DELTA) || ad <= best_d) {
                    continue;
                }
                best_d = ad;
                best_a = hits[a].m_i;
                best_b = hits[b].m_i;
                best_cx = cx;
                best_cy = cy;
            }
        }
    }
    *out_d = best_d;
    *out_a = best_a;
    *out_b = best_b;
    *out_cx = best_cx;
    *out_cy = best_cy;
    return best_d >= static_cast<u16>(GFL_MIN_STEP_DELTA);
}

bool GenFortLocationsMk1::proc_patch (
    const GameArraySimple& map,
    u16 pid,
    u32 n,
    u32* q,
    u32* par,
    u8* vis)
{
    if (pid == GFL_IDX_NONE || pid > m_patch_n) {
        return true;
    }
    if (m_psz[pid] < static_cast<u32>(GFL_MIN_PATCH_TILES)) {
        return true;
    }
    if (!mark_patch_frags(map, pid, q)) {
        return false;
    }
    m_info[pid].m_pid = pid;
    if (!walk_patch_frags(map, pid)) {
        return false;
    }
    u16 best_d = 0;
    u32 best_a = 0;
    u32 best_b = 0;
    u16 best_cx = 0;
    u16 best_cy = 0;
    if (!find_best_pass(map, pid, &best_d, &best_a, &best_b, &best_cx, &best_cy, q, par)) {
        return true;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 add = stamp_pass_path(map, m_frag, m_pass, w, h, best_cx, best_cy, best_a, best_b, best_d, q, par);
    if (add == 0u) {
        return true;
    }
    m_pass_n = m_pass_n + add;
    u16 pids[2] = {pid, GFL_IDX_NONE};
    const u32 comp_n = split_patch(map, pid, q, vis, pids);
    if (comp_n == 0u) {
        return false;
    }
    if (comp_n == 1u) {
        if (!mark_patch_frags(map, pid, q)) {
            return false;
        }
        return walk_patch_frags(map, pid);
    }
    for (u8 k = 0; k < 2u; ++k) {
        const u16 kid = pids[k];
        if (kid == GFL_IDX_NONE || kid > m_patch_n) {
            continue;
        }
        if (m_psz[kid] < static_cast<u32>(GFL_MIN_PATCH_TILES)) {
            continue;
        }
        if (!proc_patch(map, kid, n, q, par, vis)) {
            return false;
        }
    }
    return true;
}

bool GenFortLocationsMk1::find_passes (const GameArraySimple& map) {
    GAME_EXPECT_RET(m_ok, false, "GenFortLocations find_passes not begun");
    GAME_EXPECT_RET(m_psz != nullptr && m_patch_n > 0u, false, "GenFortLocations find_passes no patches");
    GAME_EXPECT_RET(map.width() == m_patch.w() && map.height() == m_patch.h(), false,
        "GenFortLocations find_passes map size mismatch");
    const u32 n = static_cast<u32>(map.width()) * static_cast<u32>(map.height());
    u32* q = new u32[n];
    u32* par = new u32[n];
    u8* vis = new u8[n];
    m_pass_n = 0;
    m_frag_n = 0;
    std::memset(m_pass.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
    std::memset(m_frag.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(m_walk.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(vis, 0, static_cast<size_t>(n));
    clr_info();
    grow_info(m_patch_n);
    const u16 init_pn = m_patch_n;
    for (u16 pid = 1u; pid <= init_pn; ++pid) {
        if (m_psz[pid] < static_cast<u32>(GFL_MIN_PATCH_TILES)) {
            continue;
        }
        if (!proc_patch(map, pid, n, q, par, vis)) {
            delete[] q;
            delete[] par;
            delete[] vis;
            return false;
        }
    }
    delete[] q;
    delete[] par;
    delete[] vis;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
