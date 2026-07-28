//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "test_hlp_tile_ownership.h"

#include "game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "wb_que_xy.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_pct = 10u;
static const u32 k_seed = 43u;
static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

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

static bool is_mtn (u8 t) {
    return t == TERR_MOUNTAINS[0];
}

static bool can_claim (u8 t) {
    return t != TERR_NONE[0] && !is_mtn(t) && !is_wtr(t);
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
    if (can_claim(s.m_map.get_terrain(cx, cy)) && ov[ti0] == TestHlpTileOwnership::k_sec_none) {
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
            if (!can_claim(s.m_map.get_terrain(tx, ty)) || ov[ti] != TestHlpTileOwnership::k_sec_none) {
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

//================================================================================================================================
//=> - TestHlpTileOwnership -
//================================================================================================================================

bool TestHlpTileOwnership::mock_sectors (GameState& s, u16* ov, u16* out_sec_n, u16* seed_x, u16* seed_y) {
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

void TestHlpTileOwnership::apply_owners (GameState& s, const u16* ov) {
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

bool TestHlpTileOwnership::find_wide_border (
    const u16* ov,
    u16 w,
    u16 h,
    u16 sec_n,
    u16* pa,
    u16* pb,
    u32* out_n) {
    if (sec_n < 2u || pa == nullptr || pb == nullptr || out_n == nullptr) {
        return false;
    }
    u32* cnt = new u32[static_cast<u32>(sec_n) * static_cast<u32>(sec_n)];
    if (cnt == nullptr) {
        return false;
    }
    for (u32 i = 0; i < static_cast<u32>(sec_n) * static_cast<u32>(sec_n); ++i) {
        cnt[i] = 0u;
    }
    const u32 wi = w;
    const u32 hi = h;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 a = ov[tidx(w, x, y)];
            if (a == k_sec_none || a >= sec_n) {
                continue;
            }
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(x) + k_dx4[d];
                const i32 ny = static_cast<i32>(y) + k_dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u16 b = ov[tidx(w, static_cast<u16>(nx), static_cast<u16>(ny))];
                if (b == k_sec_none || b >= sec_n || b <= a) {
                    continue;
                }
                cnt[static_cast<u32>(a) * static_cast<u32>(sec_n) + static_cast<u32>(b)]++;
            }
        }
    }
    u32 best = 0u;
    u16 ba = 0;
    u16 bb = 1;
    for (u16 a = 0; a < sec_n; ++a) {
        for (u16 b = static_cast<u16>(a + 1u); b < sec_n; ++b) {
            const u32 c = cnt[static_cast<u32>(a) * static_cast<u32>(sec_n) + static_cast<u32>(b)];
            if (c > best) {
                best = c;
                ba = a;
                bb = b;
            }
        }
    }
    delete[] cnt;
    if (best == 0u) {
        return false;
    }
    *pa = ba;
    *pb = bb;
    *out_n = best;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
