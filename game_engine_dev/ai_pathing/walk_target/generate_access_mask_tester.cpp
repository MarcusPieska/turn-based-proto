//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "civ_relations.h"
#include "civ_static_key.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_primitives.h"
#include "game_state.h"
#include "generate_access_mask.h"
#include "runtime_static_loader.h"
#include "wb_que_xy.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const cstr G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-43/terrain.ppm";
static const cstr G_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-43/climate.ppm";
static const cstr G_RIV = "/home/w/Projects/simple-map-gen/p1-seed-43/rivers.ppm";
static const cstr G_RES = "/home/w/Projects/simple-map-gen/p1-seed-43/overlay.ppm";
static const cstr G_OWN = "/home/w/Projects/simple-map-gen/access-mask-ownership.ppm";
static const cstr G_ACC = "/home/w/Projects/simple-map-gen/access-mask-access.ppm";
static const cstr G_LIB = "../../data_io/runtime_static_loader_lib.so";
static const cstr G_DATA = "../../";

static const u16 k_sec_none = 0xFFFFu;
static const u16 k_self = 0u;
static const u16 k_pct = 10u;
static const u32 k_seed = 43u;
static const u16 k_seed_cap = 256u;
static const i32 k_glyph_sc = 3;

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

static const u8 k_glyph[11][5] = {
    {0x0E, 0x11, 0x11, 0x11, 0x0E},
    {0x04, 0x0C, 0x04, 0x04, 0x0E},
    {0x0E, 0x01, 0x0E, 0x10, 0x1F},
    {0x0E, 0x01, 0x06, 0x01, 0x0E},
    {0x02, 0x06, 0x0A, 0x1F, 0x02},
    {0x1F, 0x10, 0x1E, 0x01, 0x1E},
    {0x06, 0x10, 0x1E, 0x11, 0x0E},
    {0x1F, 0x01, 0x02, 0x04, 0x04},
    {0x0E, 0x11, 0x0E, 0x11, 0x0E},
    {0x0E, 0x11, 0x0F, 0x01, 0x0C},
    {0x00, 0x04, 0x00, 0x04, 0x00},
};

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool is_wtr (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]
        || t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0];
}

static bool can_claim (u8 t) {
    return t != TERR_NONE[0] && t != TERR_MOUNTAINS[0] && !is_wtr(t);
}

static bool is_mtn (u8 t) {
    return t == TERR_MOUNTAINS[0];
}

static u32 rng_next (u32* s) {
    *s = (*s) * 1664525u + 1013904223u;
    return *s;
}

static i32 rng_jit (u32* s, i32 span) {
    if (span <= 0) {
        return 0;
    }
    return static_cast<i32>(rng_next(s) % static_cast<u32>(span + span + 1)) - span;
}

static u16 clamp_u16 (i32 v, u16 lo, u16 hi) {
    if (v < static_cast<i32>(lo)) {
        return lo;
    }
    if (v > static_cast<i32>(hi)) {
        return hi;
    }
    return static_cast<u16>(v);
}

static bool find_seed (
    const GameState& s,
    u16 w,
    u16 h,
    const u16* ov,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy) {
    const u32 ti0 = tidx(w, cx, cy);
    if (can_claim(s.m_map.get_terrain(cx, cy)) && ov[ti0] == k_sec_none) {
        *ox = cx;
        *oy = cy;
        return true;
    }
    const i32 rad = 10;
    i32 best_d = 0x7fffffff;
    i32 best_x = -1;
    i32 best_y = -1;
    for (i32 dy = -rad; dy <= rad; ++dy) {
        for (i32 dx = -rad; dx <= rad; ++dx) {
            const i32 x = static_cast<i32>(cx) + dx;
            const i32 y = static_cast<i32>(cy) + dy;
            if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                continue;
            }
            const u16 tx = static_cast<u16>(x);
            const u16 ty = static_cast<u16>(y);
            const u32 ti = tidx(w, tx, ty);
            if (!can_claim(s.m_map.get_terrain(tx, ty)) || ov[ti] != k_sec_none) {
                continue;
            }
            const i32 d = dx * dx + dy * dy;
            if (d >= best_d) {
                continue;
            }
            best_d = d;
            best_x = x;
            best_y = y;
        }
    }
    if (best_x < 0) {
        return false;
    }
    *ox = static_cast<u16>(best_x);
    *oy = static_cast<u16>(best_y);
    return true;
}

static bool mock_sectors (GameState& s, u16* ov, u16* out_sec_n, u16* seed_x, u16* seed_y) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    const u32 n = s.m_map.tile_n();
    const u32 wi = w;
    const u32 hi = h;
    for (u32 i = 0; i < n; ++i) {
        ov[i] = k_sec_none;
    }
    WB_QueXY cur;
    WB_QueXY nxt;
    if (!cur.ok() || !nxt.ok()) {
        return false;
    }
    u32 lx = (wi * static_cast<u32>(k_pct)) / 100u;
    u32 ly = (hi * static_cast<u32>(k_pct)) / 100u;
    if (lx == 0u) {
        lx = 1u;
    }
    if (ly == 0u) {
        ly = 1u;
    }
    const i32 jx_span = static_cast<i32>((lx * 40u) / 100u);
    const i32 jy_span = static_cast<i32>((ly * 40u) / 100u);
    u32 rng = k_seed;
    u16 sec_n = 0;
    for (u32 gy = 0; gy < hi; gy += ly) {
        for (u32 gx = 0; gx < wi; gx += lx) {
            const u16 cx = clamp_u16(static_cast<i32>(gx) + rng_jit(&rng, jx_span), 0u, static_cast<u16>(w - 1u));
            const u16 cy = clamp_u16(static_cast<i32>(gy) + rng_jit(&rng, jy_span), 0u, static_cast<u16>(h - 1u));
            u16 sx = 0;
            u16 sy = 0;
            if (!find_seed(s, w, h, ov, cx, cy, &sx, &sy)) {
                continue;
            }
            if (sec_n >= k_seed_cap || sec_n >= static_cast<u16>(U8_KEY_NULL)) {
                return false;
            }
            ov[tidx(w, sx, sy)] = sec_n;
            seed_x[sec_n] = sx;
            seed_y[sec_n] = sy;
            if (!cur.push(sx, sy)) {
                return false;
            }
            sec_n = static_cast<u16>(sec_n + 1u);
        }
    }
    for (;;) {
        u32 claimed = 0;
        nxt.clear();
        const u32 fn = cur.count();
        for (u32 qi = 0; qi < fn; ++qi) {
            const u16 px = cur.x_at(qi);
            const u16 py = cur.y_at(qi);
            const u16 sid = ov[tidx(w, px, py)];
            if (sid == k_sec_none) {
                continue;
            }
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + k_dx4[d];
                const i32 ny = static_cast<i32>(py) + k_dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u16 tx = static_cast<u16>(nx);
                const u16 ty = static_cast<u16>(ny);
                const u32 ni = tidx(w, tx, ty);
                if (ov[ni] != k_sec_none || !can_claim(s.m_map.get_terrain(tx, ty))) {
                    continue;
                }
                ov[ni] = sid;
                if (!nxt.push(tx, ty)) {
                    return false;
                }
                claimed++;
            }
        }
        if (claimed == 0u) {
            break;
        }
        cur.swap(nxt);
    }
    cur.clear();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 ti = tidx(w, x, y);
            if (ov[ti] == k_sec_none) {
                continue;
            }
            bool edge = false;
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(x) + k_dx4[d];
                const i32 ny = static_cast<i32>(y) + k_dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u16 tx = static_cast<u16>(nx);
                const u16 ty = static_cast<u16>(ny);
                const u32 ni = tidx(w, tx, ty);
                if (ov[ni] == k_sec_none && is_mtn(s.m_map.get_terrain(tx, ty))) {
                    edge = true;
                    break;
                }
            }
            if (edge && !cur.push(x, y)) {
                return false;
            }
        }
    }
    for (;;) {
        u32 claimed = 0;
        nxt.clear();
        const u32 fn = cur.count();
        for (u32 qi = 0; qi < fn; ++qi) {
            const u16 px = cur.x_at(qi);
            const u16 py = cur.y_at(qi);
            const u16 sid = ov[tidx(w, px, py)];
            if (sid == k_sec_none) {
                continue;
            }
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + k_dx4[d];
                const i32 ny = static_cast<i32>(py) + k_dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u16 tx = static_cast<u16>(nx);
                const u16 ty = static_cast<u16>(ny);
                const u32 ni = tidx(w, tx, ty);
                if (ov[ni] != k_sec_none || !is_mtn(s.m_map.get_terrain(tx, ty))) {
                    continue;
                }
                ov[ni] = sid;
                if (!nxt.push(tx, ty)) {
                    return false;
                }
                claimed++;
            }
        }
        if (claimed == 0u) {
            break;
        }
        cur.swap(nxt);
    }
    *out_sec_n = sec_n;
    return sec_n > 1u;
}

static void apply_owners (GameState& s, const u16* ov) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 sid = ov[tidx(w, x, y)];
            if (sid == k_sec_none) {
                s.m_map.set_civ_owner(x, y, U8_KEY_NULL);
            } else {
                s.m_map.set_civ_owner(x, y, static_cast<u8>(sid));
            }
        }
    }
}

static void terr_rgb (u8 t, u8* r, u8* g, u8* b) {
    *r = 40;
    *g = 40;
    *b = 40;
    if (t == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1];
        *g = TERR_OCEAN[2];
        *b = TERR_OCEAN[3];
    } else if (t == TERR_SEA[0]) {
        *r = TERR_SEA[1];
        *g = TERR_SEA[2];
        *b = TERR_SEA[3];
    } else if (t == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1];
        *g = TERR_COASTAL[2];
        *b = TERR_COASTAL[3];
    } else if (t == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1];
        *g = TERR_PLAINS[2];
        *b = TERR_PLAINS[3];
    } else if (t == TERR_HILLS[0]) {
        *r = TERR_HILLS[1];
        *g = TERR_HILLS[2];
        *b = TERR_HILLS[3];
    } else if (t == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1];
        *g = TERR_MOUNTAINS[2];
        *b = TERR_MOUNTAINS[3];
    }
}

static void own_rgb (u8 own, u8* r, u8* g, u8* b) {
    if (own == U8_KEY_NULL) {
        return;
    }
    if (own == 0u) {
        *r = 40;
        *g = 200;
        *b = 60;
        return;
    }
    const u32 s = static_cast<u32>(own) + 1u;
    *r = static_cast<u8>(80u + ((s * 97u) % 140u));
    *g = static_cast<u8>(40u + ((s * 57u) % 100u));
    *b = static_cast<u8>(40u + ((s * 31u) % 100u));
}

static void put_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i] = r;
    rgb[i + 1u] = g;
    rgb[i + 2u] = b;
}

static void fill_rect (u8* rgb, u16 w, u16 h, i32 x0, i32 y0, i32 rw, i32 rh, u8 r, u8 g, u8 b) {
    for (i32 y = 0; y < rh; ++y) {
        for (i32 x = 0; x < rw; ++x) {
            put_px(rgb, w, h, x0 + x, y0 + y, r, g, b);
        }
    }
}

static void blit_glyph (u8* rgb, u16 w, u16 h, i32 x0, i32 y0, u8 gi, u8 r, u8 g, u8 b) {
    if (gi > 10u) {
        return;
    }
    const i32 sc = k_glyph_sc;
    for (i32 row = 0; row < 5; ++row) {
        const u8 bits = k_glyph[gi][row];
        for (i32 col = 0; col < 5; ++col) {
            if ((bits & (1u << (4 - col))) == 0u) {
                continue;
            }
            const i32 px = x0 + col * sc;
            const i32 py = y0 + row * sc;
            fill_rect(rgb, w, h, px - 1, py - 1, sc + 2, sc + 2, 0, 0, 0);
            fill_rect(rgb, w, h, px, py, sc, sc, r, g, b);
        }
    }
}

static i32 glyph_adv () {
    return 5 * k_glyph_sc + 2;
}

static void blit_u16 (u8* rgb, u16 w, u16 h, i32* x, i32 y, u16 v, u8 r, u8 g, u8 b) {
    char buf[8];
    const int n = std::snprintf(buf, sizeof(buf), "%u", (unsigned)v);
    const i32 adv = glyph_adv();
    for (int i = 0; i < n; ++i) {
        blit_glyph(rgb, w, h, *x, y, static_cast<u8>(buf[i] - '0'), r, g, b);
        *x += adv;
    }
}

static void blit_seed_lbl (
    u8* rgb,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16 player_idx,
    u16 civ_idx) {
    const i32 adv = glyph_adv();
    const i32 y = static_cast<i32>(sy) - (5 * k_glyph_sc) / 2;
    i32 x = static_cast<i32>(sx) - adv * 2;
    blit_u16(rgb, w, h, &x, y, player_idx, 255, 255, 40);
    blit_glyph(rgb, w, h, x, y, 10u, 255, 255, 40);
    x += adv;
    blit_u16(rgb, w, h, &x, y, civ_idx, 255, 255, 40);
}

static bool save_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void paint_seed_lbls (
    u8* rgb,
    u16 w,
    u16 h,
    const GameState& s,
    const u16* seed_x,
    const u16* seed_y,
    u16 seat_n) {
    for (u16 p = 0; p < seat_n; ++p) {
        blit_seed_lbl(rgb, w, h, seed_x[p], seed_y[p], p, s.m_player_states[p].m_civ_index);
    }
}

static bool save_own_ppm (
    const GameState& s,
    const u16* seed_x,
    const u16* seed_y,
    u16 seat_n,
    cstr path) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    u8* rgb = new u8[static_cast<size_t>(w) * static_cast<size_t>(h) * 3u];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(s.m_map.get_terrain(x, y), &r, &g, &b);
            const u8 own = s.m_map.get_civ_owner(x, y);
            if (own != U8_KEY_NULL) {
                u8 orr = r;
                u8 og = g;
                u8 ob = b;
                own_rgb(own, &orr, &og, &ob);
                r = static_cast<u8>((static_cast<u32>(r) + static_cast<u32>(orr) * 2u) / 3u);
                g = static_cast<u8>((static_cast<u32>(g) + static_cast<u32>(og) * 2u) / 3u);
                b = static_cast<u8>((static_cast<u32>(b) + static_cast<u32>(ob) * 2u) / 3u);
            }
            const u32 i = tidx(w, x, y) * 3u;
            rgb[i] = r;
            rgb[i + 1u] = g;
            rgb[i + 2u] = b;
        }
    }
    paint_seed_lbls(rgb, w, h, s, seed_x, seed_y, seat_n);
    const bool ok = save_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_acc_ppm (
    const GameState& s,
    const Whiteboard_1B& msk,
    const u16* seed_x,
    const u16* seed_y,
    u16 seat_n,
    cstr path) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    u8* rgb = new u8[static_cast<size_t>(w) * static_cast<size_t>(h) * 3u];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(s.m_map.get_terrain(x, y), &r, &g, &b);
            if (msk.rd(x, y) != 0u) {
                r = static_cast<u8>((static_cast<u32>(r) + 255u) / 2u);
                g = static_cast<u8>((static_cast<u32>(g) + 255u) / 2u);
                b = static_cast<u8>((static_cast<u32>(b) + 220u) / 2u);
            } else {
                r = static_cast<u8>(r / 3u);
                g = static_cast<u8>(g / 3u);
                b = static_cast<u8>(b / 3u);
            }
            const u32 i = tidx(w, x, y) * 3u;
            rgb[i] = r;
            rgb[i + 1u] = g;
            rgb[i + 2u] = b;
        }
    }
    paint_seed_lbls(rgb, w, h, s, seed_x, seed_y, seat_n);
    const bool ok = save_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool load_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load(G_LIB, G_DATA)) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

static bool init_state (GameState* state) {
    if (state == nullptr || !load_statics()) {
        return false;
    }
    state->clear();
    state->m_statics = g_rt_statics;
    if (!Factory_GameArraySimple::load_map_gen_data(&state->m_map, G_TERR, G_CLIM, G_RIV)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&state->m_map, G_RES)) {
        return false;
    }
    return state->m_map.width() > 0 && state->m_map.height() > 0;
}

static cstr rel_name (CivRel rel) {
    switch (rel) {
    case CivRel::CIV_REL_UNDISCOVERED: return "UNDISCOVERED";
    case CivRel::CIV_REL_NONE: return "NONE";
    case CivRel::CIV_REL_WAR: return "WAR";
    case CivRel::CIV_REL_PEACE: return "PEACE";
    case CivRel::CIV_REL_PEACE_ACCESS: return "PEACE_ACCESS";
    case CivRel::CIV_REL_ALLY: return "ALLY";
    case CivRel::CIV_REL_SUBJECT: return "SUBJECT";
    }
    return "?";
}

static void print_player0_rels (const GameState& s) {
    if (s.m_player_states == nullptr || s.m_player_n == 0) {
        return;
    }
    const u16 self_civ = s.m_player_states[0].m_civ_index;
    const CivStaticDataKey sk0 = CivStaticDataKey::from_raw(self_civ);
    std::printf("player 0 civ=%u relations:\n", (u32)self_civ);
    for (u16 p = 1; p < s.m_player_n; ++p) {
        const u16 oth = s.m_player_states[p].m_civ_index;
        const CivRel rel = s.m_civ_relations.get(sk0, CivStaticDataKey::from_raw(oth));
        const bool open = rel != CivRel::CIV_REL_PEACE;
        std::printf("  -> player %u civ=%u rel=%s access=%u\n",
            (u32)p, (u32)oth, rel_name(rel), open ? 1u : 0u);
    }
}

static bool init_seats_and_rels (GameState* state, u16 seat_n) {
    if (state == nullptr || g_rt_statics == nullptr || seat_n == 0) {
        return false;
    }
    const u16 civ_n = g_rt_statics->civ().get_item_count();
    if (civ_n < seat_n) {
        return false;
    }
    PlayerState* seats = new PlayerState[seat_n];
    if (seats == nullptr) {
        return false;
    }
    for (u16 i = 0; i < seat_n; ++i) {
        seats[i].m_ai_controlled = 0;
        seats[i].m_is_active = 1;
        seats[i].m_civ_index = i;
    }
    state->m_player_states = seats;
    state->m_player_n = seat_n;
    state->m_players_remaining = seat_n;
    state->m_civ_relations.reset(civ_n);
    const CivStaticDataKey sk0 = CivStaticDataKey::from_raw(0);
    const u8 rel_mod = static_cast<u8>(CivRel::CIV_REL_SUBJECT);
    for (u16 i = 1; i < seat_n; ++i) {
        const CivRel rel = static_cast<CivRel>(static_cast<u8>(i % rel_mod));
        state->m_civ_relations.set(sk0, CivStaticDataKey::from_raw(i), rel);
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    GameState state;
    if (!init_state(&state)) {
        std::printf("*** FAILED init_state\n");
        return 1;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    WhiteboardMng::init(w, h);
    Whiteboard_2B ov_wb("access_mask_tester", "ov", 0u);
    if (!ov_wb.ok()) {
        std::printf("*** FAILED ov whiteboard\n");
        WhiteboardMng::terminate();
        return 1;
    }
    u16 seed_x[k_seed_cap];
    u16 seed_y[k_seed_cap];
    u16 sec_n = 0;
    if (!mock_sectors(state, ov_wb.get_iter_ptr(), &sec_n, seed_x, seed_y)) {
        std::printf("*** FAILED mock_sectors\n");
        WhiteboardMng::terminate();
        return 1;
    }
    if (!init_seats_and_rels(&state, sec_n)) {
        std::printf("*** FAILED init_seats_and_rels seat_n=%u\n", (u32)sec_n);
        WhiteboardMng::terminate();
        return 1;
    }
    print_player0_rels(state);
    apply_owners(state, ov_wb.get_iter_ptr());
    Whiteboard_1B msk("access_mask_tester", "msk", 0u);
    const clock_t t0 = clock();
    const bool gen_ok = GenerateAccessMask::generate(state, k_self, msk);
    const double gen_sec = static_cast<double>(clock() - t0) / CLOCKS_PER_SEC;
    if (!gen_ok) {
        std::printf("*** FAILED GenerateAccessMask (%.6f s)\n", gen_sec);
        WhiteboardMng::terminate();
        return 1;
    }
    if (!save_own_ppm(state, seed_x, seed_y, sec_n, G_OWN)) {
        std::printf("*** FAILED save %s\n", G_OWN);
        WhiteboardMng::terminate();
        return 1;
    }
    if (!save_acc_ppm(state, msk, seed_x, seed_y, sec_n, G_ACC)) {
        std::printf("*** FAILED save %s\n", G_ACC);
        WhiteboardMng::terminate();
        return 1;
    }
    std::printf("gen %.6f s sec_n=%u seats=%u own=%s access=%s\n",
        gen_sec, (u32)sec_n, (u32)sec_n, G_OWN, G_ACC);
    std::printf("*** PASSED generate_access_mask\n");
    WhiteboardMng::terminate();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
