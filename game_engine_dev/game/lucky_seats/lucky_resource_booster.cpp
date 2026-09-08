//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_resource_booster.h"

#include <vector>

#include "game_array_simple.h"
#include "game_map_defs.h"
#include "res_type_enum.h"
#include "resource_static_key.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Disk offsets (built once) -
//================================================================================================================================

struct ResOff {
    i16 m_dx;
    i16 m_dy;
};

static ResOff* g_outer = nullptr;
static ResOff* g_inner = nullptr;
static u32 g_outer_n = 0u;
static u32 g_inner_n = 0u;
static bool g_offs_ok = false;

static u32 fill_disk (u16 rad, ResOff** out) {
    if (out == nullptr || rad == 0u) {
        return 0u;
    }
    const i32 r = static_cast<i32>(rad);
    const i32 r2 = r * r;
    u32 n = 0u;
    for (i32 dy = -r; dy <= r; ++dy) {
        for (i32 dx = -r; dx <= r; ++dx) {
            if (dx * dx + dy * dy <= r2) {
                ++n;
            }
        }
    }
    ResOff* buf = new ResOff[n];
    u32 wri = 0u;
    for (i32 dy = -r; dy <= r; ++dy) {
        for (i32 dx = -r; dx <= r; ++dx) {
            if (dx * dx + dy * dy <= r2) {
                buf[wri].m_dx = static_cast<i16>(dx);
                buf[wri].m_dy = static_cast<i16>(dy);
                ++wri;
            }
        }
    }
    *out = buf;
    return n;
}

static bool ensure_offs () {
    if (g_offs_ok) {
        return true;
    }
    g_outer_n = fill_disk(LuckyResourceBooster::k_r_outer, &g_outer);
    g_inner_n = fill_disk(LuckyResourceBooster::k_r_inner, &g_inner);
    g_offs_ok = g_outer != nullptr && g_inner != nullptr && g_outer_n > 0u && g_inner_n > 0u;
    return g_offs_ok;
}

//================================================================================================================================
//=> - Shared helpers -
//================================================================================================================================

static bool is_land (u8 terr) {
    if (terr == TERR_NONE[0]) {
        return false;
    }
    return !overlay_is_water_terr(terr);
}

static bool ok_plant (const GameArraySimple& map, u16 x, u16 y) {
    if (!is_land(map.get_terrain(x, y))) {
        return false;
    }
    if (map.get_climate(x, y) == CLIMATE_DESERT) {
        return false;
    }
    return map.get_res(x, y) == U16_KEY_NULL;
}

static u16 g_place_dir = 0u;

static bool find_near (
    GameArraySimple& map,
    u16 sx,
    u16 sy,
    u16* ox,
    u16* oy)
{
    if (ox == nullptr || oy == nullptr) {
        return false;
    }
    static const i8 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const i8 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 start = static_cast<u16>(g_place_dir % 8u);
    for (u16 rad = 1u; rad <= 8u; ++rad) {
        for (u16 k = 0u; k < 8u; ++k) {
            const u16 d = static_cast<u16>((start + k) % 8u);
            const i32 x = static_cast<i32>(sx) + static_cast<i32>(k_dx[d]) * static_cast<i32>(rad);
            const i32 y = static_cast<i32>(sy) + static_cast<i32>(k_dy[d]) * static_cast<i32>(rad);
            if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(x);
            const u16 uy = static_cast<u16>(y);
            if (!is_land(map.get_terrain(ux, uy))) {
                continue;
            }
            if (map.get_res(ux, uy) != U16_KEY_NULL) {
                continue;
            }
            *ox = ux;
            *oy = uy;
            g_place_dir = static_cast<u16>((start + 1u) % 8u);
            return true;
        }
    }
    return false;
}

static u32 dup_disk (
    GameArraySimple& map,
    const u16* snap,
    u16 cx,
    u16 cy,
    const ResOff* offs,
    u32 off_n)
{
    if (snap == nullptr || offs == nullptr || off_n == 0u) {
        return 0u;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    u32 added = 0u;
    for (u32 i = 0; i < off_n; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(offs[i].m_dx);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(offs[i].m_dy);
        if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        const u32 ti = static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux);
        const u16 res = snap[ti];
        if (res == U16_KEY_NULL) {
            continue;
        }
        u16 nx = 0u;
        u16 ny = 0u;
        if (!find_near(map, ux, uy, &nx, &ny)) {
            continue;
        }
        if (!map.set_res(nx, ny, res)) {
            continue;
        }
        ++added;
    }
    return added;
}

static u16 res_type_of (const RuntimeStatics& st, u16 res) {
    if (res == U16_KEY_NULL || res >= st.resource().get_item_count()) {
        return U16_KEY_NULL;
    }
    return st.resource().get_item(ResourceStaticDataKey::from_raw(res)).type;
}

static void find_food_live (
    const GameArraySimple& map,
    const RuntimeStatics& st,
    u16 cx,
    u16 cy,
    u16* food,
    u16* live)
{
    *food = U16_KEY_NULL;
    *live = U16_KEY_NULL;
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 r_chk = map.get_res(cx, cy);
    const u16 t0 = res_type_of(st, r_chk);
    if (t0 == static_cast<u16>(ResType::FOOD_CROP)) {
        *food = r_chk;
    } else if (t0 == static_cast<u16>(ResType::LIVESTOCK)) {
        *live = r_chk;
    }
    const u16 lim = (w > h) ? w : h;
    for (u16 rad = 1u; rad <= lim && (*food == U16_KEY_NULL || *live == U16_KEY_NULL); ++rad) {
        for (i32 dy = -static_cast<i32>(rad); dy <= static_cast<i32>(rad); ++dy) {
            for (i32 dx = -static_cast<i32>(rad); dx <= static_cast<i32>(rad); ++dx) {
                if (dx != -static_cast<i32>(rad) && dx != static_cast<i32>(rad)
                    && dy != -static_cast<i32>(rad) && dy != static_cast<i32>(rad)) {
                    continue;
                }
                const i32 x = static_cast<i32>(cx) + dx;
                const i32 y = static_cast<i32>(cy) + dy;
                if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 res = map.get_res(static_cast<u16>(x), static_cast<u16>(y));
                const u16 typ = res_type_of(st, res);
                if (*food == U16_KEY_NULL && typ == static_cast<u16>(ResType::FOOD_CROP)) {
                    *food = res;
                }
                if (*live == U16_KEY_NULL && typ == static_cast<u16>(ResType::LIVESTOCK)) {
                    *live = res;
                }
                if (*food != U16_KEY_NULL && *live != U16_KEY_NULL) {
                    return;
                }
            }
        }
    }
}

static const u16 k_riv_stride = LuckyResourceBooster::k_riv_stride;

static void mark_riv_comp (const GameArraySimple& map, u16 rx, u16 ry, std::vector<u8>& used) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tn = map.tile_n();
    std::vector<u32> q(tn);
    u32 qn = 0u;
    const u32 s = static_cast<u32>(ry) * static_cast<u32>(w) + static_cast<u32>(rx);
    if (used[s] != 0u) {
        return;
    }
    used[s] = 1u;
    q[qn++] = s;
    static const i8 k_dx[4] = {0, 1, 0, -1};
    static const i8 k_dy[4] = {-1, 0, 1, 0};
    for (u32 qi = 0u; qi < qn; ++qi) {
        const u32 i = q[qi];
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        for (u16 d = 0; d < 4u; ++d) {
            const i32 nx = static_cast<i32>(x) + k_dx[d];
            const i32 ny = static_cast<i32>(y) + k_dy[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const u32 ni = static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux);
            if (used[ni] != 0u || map.get_river(ux, uy) == 0u) {
                continue;
            }
            used[ni] = 1u;
            q[qn++] = ni;
        }
    }
}

static bool nearest_riv (
    const GameArraySimple& map,
    u16 cx,
    u16 cy,
    const std::vector<u8>* used,
    u16* rx,
    u16* ry)
{
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tn = map.tile_n();
    std::vector<u8> seen(tn, 0u);
    std::vector<u32> q(tn);
    u32 qn = 0u;
    const u32 s = static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx);
    seen[s] = 1u;
    q[qn++] = s;
    static const i8 k_dx[4] = {0, 1, 0, -1};
    static const i8 k_dy[4] = {-1, 0, 1, 0};
    for (u32 qi = 0u; qi < qn; ++qi) {
        const u32 i = q[qi];
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (map.get_river(x, y) != 0u && (used == nullptr || (*used)[i] == 0u)) {
            *rx = x;
            *ry = y;
            return true;
        }
        for (u16 d = 0; d < 4u; ++d) {
            const i32 nx = static_cast<i32>(x) + k_dx[d];
            const i32 ny = static_cast<i32>(y) + k_dy[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (seen[ni] != 0u) {
                continue;
            }
            seen[ni] = 1u;
            q[qn++] = ni;
        }
    }
    return false;
}

static void collect_riv_cands (
    const GameArraySimple& map,
    u16 rx,
    u16 ry,
    std::vector<SpgCoordPair>* out)
{
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tn = map.tile_n();
    std::vector<u8> riv_seen(tn, 0u);
    std::vector<u32> q(tn);
    u32 qn = 0u;
    const u32 s = static_cast<u32>(ry) * static_cast<u32>(w) + static_cast<u32>(rx);
    riv_seen[s] = 1u;
    q[qn++] = s;
    static const i8 k_dx[4] = {0, 1, 0, -1};
    static const i8 k_dy[4] = {-1, 0, 1, 0};
    std::vector<u8> cand(tn, 0u);
    for (u32 qi = 0u; qi < qn; ++qi) {
        const u32 i = q[qi];
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                const i32 ax = static_cast<i32>(x) + dx;
                const i32 ay = static_cast<i32>(y) + dy;
                if (ax < 0 || ay < 0 || ax >= static_cast<i32>(w) || ay >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(ax);
                const u16 uy = static_cast<u16>(ay);
                const u32 ai = static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux);
                if (cand[ai] != 0u) {
                    continue;
                }
                if (!ok_plant(map, ux, uy)) {
                    continue;
                }
                cand[ai] = 1u;
                SpgCoordPair p;
                p.x = ux;
                p.y = uy;
                out->push_back(p);
            }
        }
        for (u16 d = 0; d < 4u; ++d) {
            const i32 nx = static_cast<i32>(x) + k_dx[d];
            const i32 ny = static_cast<i32>(y) + k_dy[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const u32 ni = static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux);
            if (riv_seen[ni] != 0u) {
                continue;
            }
            if (map.get_river(ux, uy) == 0u) {
                continue;
            }
            riv_seen[ni] = 1u;
            q[qn++] = ni;
        }
    }
}

static bool append_riv_sys (
    const GameArraySimple& map,
    u16 cx,
    u16 cy,
    std::vector<u8>& used,
    std::vector<SpgCoordPair>* cands)
{
    u16 rx = 0u;
    u16 ry = 0u;
    if (!nearest_riv(map, cx, cy, &used, &rx, &ry)) {
        return false;
    }
    mark_riv_comp(map, rx, ry, used);
    const u32 before = static_cast<u32>(cands->size());
    collect_riv_cands(map, rx, ry, cands);
    return static_cast<u32>(cands->size()) > before;
}

static u32 scatter_stride (
    GameArraySimple& map,
    u16 res,
    u16 want,
    const std::vector<SpgCoordPair>& cands,
    u16 stride)
{
    if (res == U16_KEY_NULL || want == 0u || cands.empty() || stride == 0u) {
        return 0u;
    }
    u32 added = 0u;
    for (u16 off = 0u; off < stride && added < want; ++off) {
        for (u32 i = off; i < static_cast<u32>(cands.size()) && added < want; i += stride) {
            const SpgCoordPair& p = cands[i];
            if (!ok_plant(map, p.x, p.y)) {
                continue;
            }
            if (!map.set_res(p.x, p.y, res)) {
                continue;
            }
            ++added;
        }
    }
    return added;
}

static u32 place_along_rivers (
    GameArraySimple& map,
    u16 cx,
    u16 cy,
    u16 res,
    u16 want)
{
    if (res == U16_KEY_NULL || want == 0u) {
        return 0u;
    }
    const u32 tn = map.tile_n();
    std::vector<u8> used(tn, 0u);
    std::vector<SpgCoordPair> cands;
    const u32 prefer = static_cast<u32>(want) * static_cast<u32>(k_riv_stride);
    while (static_cast<u32>(cands.size()) < prefer) {
        if (!append_riv_sys(map, cx, cy, used, &cands)) {
            break;
        }
    }
    while (static_cast<u32>(cands.size()) < want) {
        if (!append_riv_sys(map, cx, cy, used, &cands)) {
            break;
        }
    }
    if (cands.empty()) {
        return 0u;
    }
    u32 added = scatter_stride(map, res, want, cands, k_riv_stride);
    if (added < want) {
        added += scatter_stride(map, res, static_cast<u16>(want - added), cands, 1u);
    }
    return added;
}

//================================================================================================================================
//=> - LuckyResourceBooster -
//================================================================================================================================

bool LuckyResourceBooster::boost_local (GameArraySimple& map, const SpgCoordPair* pts, u16 n, u32* out_added) {
    if (pts == nullptr || n == 0u) {
        if (out_added != nullptr) {
            *out_added = 0u;
        }
        return pts != nullptr;
    }
    if (!ensure_offs()) {
        return false;
    }
    g_place_dir = 0u;
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0u || h == 0u) {
        return false;
    }
    const u32 tn = map.tile_n();
    std::vector<u16> snap(tn);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            snap[i] = map.get_res(x, y);
        }
    }
    u32 added = 0u;
    for (u16 i = 0; i < n; ++i) {
        const u16 cx = pts[i].x;
        const u16 cy = pts[i].y;
        if (cx >= w || cy >= h) {
            continue;
        }
        added += dup_disk(map, snap.data(), cx, cy, g_outer, g_outer_n);
        added += dup_disk(map, snap.data(), cx, cy, g_inner, g_inner_n);
    }
    if (out_added != nullptr) {
        *out_added = added;
    }
    return true;
}

bool LuckyResourceBooster::boost_river (
    GameArraySimple& map,
    const RuntimeStatics& st,
    const SpgCoordPair* pts,
    u16 n,
    u32* out_added)
{
    if (pts == nullptr || n == 0u) {
        if (out_added != nullptr) {
            *out_added = 0u;
        }
        return pts != nullptr;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0u || h == 0u) {
        return false;
    }
    u32 added = 0u;
    for (u16 i = 0; i < n; ++i) {
        const u16 cx = pts[i].x;
        const u16 cy = pts[i].y;
        if (cx >= w || cy >= h) {
            continue;
        }
        u16 food = U16_KEY_NULL;
        u16 live = U16_KEY_NULL;
        find_food_live(map, st, cx, cy, &food, &live);
        if (food != U16_KEY_NULL) {
            if (map.get_climate(cx, cy) != CLIMATE_DESERT) {
                const u16 prev = map.get_res(cx, cy);
                if (map.set_res(cx, cy, food) && prev == U16_KEY_NULL) {
                    ++added;
                }
            }
        }
        added += place_along_rivers(map, cx, cy, food, LuckyResourceBooster::k_river_n);
        added += place_along_rivers(map, cx, cy, live, LuckyResourceBooster::k_river_n);
    }
    if (out_added != nullptr) {
        *out_added = added;
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
