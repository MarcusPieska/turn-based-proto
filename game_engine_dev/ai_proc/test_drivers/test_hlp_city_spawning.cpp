//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "test_hlp_city_spawning.h"

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"

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

static bool can_city (const GameState& s, u16 x, u16 y) {
    const u8 t = s.m_map.get_terrain(x, y);
    if (!can_claim(t)) {
        return false;
    }
    if (s.m_map.get_climate(x, y) == CLIMATE_DESERT) {
        return false;
    }
    if (s.m_map.get_add_typ(x, y) != 0u) {
        return false;
    }
    return true;
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

//================================================================================================================================
//=> - TestHlpCitySpawning -
//================================================================================================================================

u16 TestHlpCitySpawning::spawn_lattice (
    GameState& s,
    const u16* ov,
    u16 pa,
    u16 pb,
    u16 lat,
    u16 jit_pct,
    u32 seed) {
    if (ov == nullptr || lat == 0u) {
        return 0;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    const i32 jit = static_cast<i32>((static_cast<u32>(lat) * static_cast<u32>(jit_pct)) / 100u);
    u32 rng = seed ^ 0xC0FFEEu;
    u16 placed = 0;
    for (u16 gy = 0; gy < h; gy = static_cast<u16>(gy + lat)) {
        for (u16 gx = 0; gx < w; gx = static_cast<u16>(gx + lat)) {
            const u16 x = clamp_u16(static_cast<i32>(gx) + rng_jit(&rng, jit), 0u, static_cast<u16>(w - 1u));
            const u16 y = clamp_u16(static_cast<i32>(gy) + rng_jit(&rng, jit), 0u, static_cast<u16>(h - 1u));
            const u16 sid = ov[tidx(w, x, y)];
            if (sid != pa && sid != pb) {
                continue;
            }
            if (!can_city(s, x, y)) {
                continue;
            }
            const u16 idx = s.m_cities.get_next_new_city_idx();
            City* city = s.m_cities.get_city(idx);
            if (city == nullptr) {
                continue;
            }
            city->init(sid, x, y);
            if (!s.m_map.set_tile_add(x, y, idx, BUILD_ADD_CITY)) {
                continue;
            }
            placed = static_cast<u16>(placed + 1u);
        }
    }
    return placed;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
