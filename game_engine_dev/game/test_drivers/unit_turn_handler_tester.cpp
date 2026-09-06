//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/unit-turn-handler/game_loop.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 4u;
static const u32 G_TURNS = 8u;
static const u8 G_START_HP = 40u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

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
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool is_land (const GameState& state, u16 x, u16 y) {
    if (x >= state.m_map.width() || y >= state.m_map.height()) {
        return false;
    }
    return !overlay_is_water_terr(state.m_map.get_terrain(x, y));
}

static bool find_city (GameState& state, u16* cx, u16* cy, u16* owner) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        *cx = c->get_x();
        *cy = c->get_y();
        *owner = c->get_owner();
        return true;
    }
    return false;
}

static bool find_adj_land (const GameState& state, u16 cx, u16 cy, u16* ox, u16* oy) {
    static const i16 k_dx[4] = {1, -1, 0, 0};
    static const i16 k_dy[4] = {0, 0, 1, -1};
    for (u16 i = 0; i < 4u; ++i) {
        const i32 nx = static_cast<i32>(cx) + k_dx[i];
        const i32 ny = static_cast<i32>(cy) + k_dy[i];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 x = static_cast<u16>(nx);
        const u16 y = static_cast<u16>(ny);
        if (!is_land(state, x, y)) {
            continue;
        }
        if (state.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
            continue;
        }
        *ox = x;
        *oy = y;
        return true;
    }
    return false;
}

static u16 find_unit_typ (const RuntimeStatics& st, cstr name) {
    const u16 un = st.unit().get_item_count();
    for (u16 i = 0; i < un; ++i) {
        cstr nm = st.unit().get_name(UnitStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool clear_tile (GameState& state, u16 x, u16 y) {
    for (;;) {
        const u16 hd = state.m_map.get_unit_hd(x, y);
        if (hd == U16_KEY_NULL) {
            return true;
        }
        if (!UnitMovementMng::destroy_unit(state, UnitAddKey::from_raw(hd))) {
            return false;
        }
    }
}

static bool place_damaged (
    GameState& state,
    u16 x,
    u16 y,
    u16 seat,
    u16 typ,
    u8 hp,
    u16* out_idx)
{
    UnitAddKey key = UnitAddKey::None();
    if (!UnitMovementMng::place_on_tile(state, x, y, seat, typ, &key)) {
        return false;
    }
    UnitAddStruct* u = state.m_units.get_unit_add(key);
    if (u == nullptr) {
        return false;
    }
    u->m_health = hp;
    *out_idx = key.value();
    return true;
}

static u8 unit_hp (const GameState& state, u16 idx) {
    const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(idx));
    return (u != nullptr) ? u->m_health : 0u;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("*** FAILED paths\n");
        return 1;
    }
    GameSetup setup;
    GameState state;
    MapPpmPaths paths = {};
    paths.m_terr = g_terr;
    paths.m_clim = g_clim;
    paths.m_riv = g_riv;
    paths.m_ov = g_ov;
    paths.m_res = g_res;
    if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
        std::printf("*** FAILED setup_new_game\n");
        return 1;
    }
    CityBorder::bind_map(&state.m_map);
    if (state.m_statics == nullptr
        || !TileAttrTables::setup(*state.m_statics)
        || !UnitMovementMng::setup_mvt_costs(*state.m_statics)) {
        std::printf("*** FAILED statics/mvt setup\n");
        state.clear();
        return 1;
    }

    u16 cx = 0;
    u16 cy = 0;
    u16 owner = 0;
    u16 ox = 0;
    u16 oy = 0;
    if (!find_city(state, &cx, &cy, &owner) || !find_adj_land(state, cx, cy, &ox, &oy)) {
        std::printf("*** FAILED find city/adj\n");
        state.clear();
        return 1;
    }
    const u16 typ = find_unit_typ(*state.m_statics, "Warrior");
    if (typ == U16_KEY_NULL) {
        std::printf("*** FAILED find Warrior\n");
        state.clear();
        return 1;
    }
    if (!clear_tile(state, cx, cy) || !clear_tile(state, ox, oy)) {
        std::printf("*** FAILED clear tiles\n");
        state.clear();
        return 1;
    }
    u16 in_idx = U16_KEY_NULL;
    u16 out_idx = U16_KEY_NULL;
    if (!place_damaged(state, cx, cy, owner, typ, G_START_HP, &in_idx)
        || !place_damaged(state, ox, oy, owner, typ, G_START_HP, &out_idx)) {
        std::printf("*** FAILED place units\n");
        state.clear();
        return 1;
    }
    std::printf("city=(%u,%u) open=(%u,%u) in_idx=%u out_idx=%u start_hp=%u\n",
        static_cast<unsigned>(cx), static_cast<unsigned>(cy),
        static_cast<unsigned>(ox), static_cast<unsigned>(oy),
        static_cast<unsigned>(in_idx), static_cast<unsigned>(out_idx),
        static_cast<unsigned>(G_START_HP));

    GameLoop loop;
    if (!loop.begin(&state, G_TRACE)) {
        std::printf("*** FAILED GameLoop::begin\n");
        state.clear();
        return 1;
    }
    for (u32 t = 0; t < G_TURNS; ++t) {
        if (!loop.step()) {
            std::printf("*** FAILED GameLoop::step turn=%u\n", static_cast<unsigned>(t));
            loop.end();
            state.clear();
            return 1;
        }
        std::printf("turn %u in_city=%u open=%u\n",
            static_cast<unsigned>(state.m_current_turn),
            static_cast<unsigned>(unit_hp(state, in_idx)),
            static_cast<unsigned>(unit_hp(state, out_idx)));
    }
    loop.end();
    state.clear();
    setup.release_map_gen();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
