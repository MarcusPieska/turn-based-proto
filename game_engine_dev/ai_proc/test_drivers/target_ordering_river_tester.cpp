//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "game_array_simple.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "gen_watershed.h"
#include "target_ordering_river_system.h"
#include "target_sort_by_core.h"
#include "tile_transfer.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/target-ordering-river";
static const char* G_OWN = "/home/w/Projects/simple-map-gen/target-ordering-river/ownership.bin";
static const char* G_CITIES = "/home/w/Projects/simple-map-gen/target-ordering-river/cities.txt";
static const char* G_OWN_ALT = "/home/w/Projects/simple-map-gen/target-ordering-flood/ownership.bin";
static const char* G_CITIES_ALT = "/home/w/Projects/simple-map-gen/target-ordering-flood/cities.txt";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/target-ordering-river/game_loop.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100u;
static const u32 G_TURN_CAP = 300u;
static const u16 G_TGT_CAP = 256u;
static const u16 G_SYS_CAP = 4096u;
static const u16 G_PAIR_CAP = 10u;
static const u32 G_ADJ_MAX = 48u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

static const u8 k_own_pal[][3] = {
    {220, 40, 40}, {40, 90, 220}, {40, 170, 70}, {220, 110, 30},
    {190, 40, 170}, {30, 170, 170}, {150, 70, 30}, {100, 40, 180},
    {240, 200, 40}, {40, 200, 220}, {180, 80, 80}, {80, 80, 200},
};
static const u16 k_own_pal_n = static_cast<u16>(sizeof(k_own_pal) / sizeof(k_own_pal[0]));

struct RivSys {
    u16 m_sx;
    u16 m_sy;
    u32 m_n;
};

struct PairRun {
    u16 m_pa;
    u16 m_pb;
    u16 m_sx;
    u16 m_sy;
    u32 m_riv_n;
    u16 m_a_n;
    u16 m_b_n;
    u32 m_adj;
};

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

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static bool file_exists (cstr path) {
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

static bool save_ownership (const GameState& state) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::FILE* fp = std::fopen(G_OWN, "wb");
    if (fp == nullptr) {
        return false;
    }
    if (std::fwrite(&w, sizeof(w), 1, fp) != 1 || std::fwrite(&h, sizeof(h), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 o = state.m_map.get_civ_owner(x, y);
            if (std::fwrite(&o, sizeof(o), 1, fp) != 1) {
                std::fclose(fp);
                return false;
            }
        }
    }
    std::fclose(fp);
    return true;
}

static bool load_ownership_path (GameState& state, cstr path) {
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u16 w = 0;
    u16 h = 0;
    if (std::fread(&w, sizeof(w), 1, fp) != 1 || std::fread(&h, sizeof(h), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (w != state.m_map.width() || h != state.m_map.height()) {
        std::fclose(fp);
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 o = U8_KEY_NULL;
            if (std::fread(&o, sizeof(o), 1, fp) != 1) {
                std::fclose(fp);
                return false;
            }
            state.m_map.set_civ_owner(x, y, o);
        }
    }
    std::fclose(fp);
    return true;
}

static bool save_cities (const GameState& state) {
    std::FILE* fp = std::fopen(G_CITIES, "w");
    if (fp == nullptr) {
        return false;
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        std::fprintf(fp, "%u:%u:%u:%u\n",
            static_cast<unsigned>(c->get_x()),
            static_cast<unsigned>(c->get_y()),
            static_cast<unsigned>(c->get_owner()),
            static_cast<unsigned>(c->get_current_culture()));
    }
    std::fclose(fp);
    return true;
}

static bool found_city (GameState& state, u16 x, u16 y, u16 player) {
    if (state.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
        return true;
    }
    const u16 city_idx = state.m_cities.get_next_new_city_idx();
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return false;
    }
    city->init(player, x, y);
    if (!state.m_map.set_tile_add(x, y, city_idx, BUILD_ADD_CITY)) {
        return false;
    }
    (void)state.city_net_on_found(city_idx);
    return true;
}

static City* city_at (GameState& state, u16 x, u16 y) {
    if (state.m_map.get_add_typ(x, y) != BUILD_ADD_CITY) {
        return nullptr;
    }
    return state.m_cities.get_city(state.m_map.get_add_idx(x, y));
}

static bool load_cities_path (GameState& state, cstr path, bool talk) {
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        return false;
    }
    unsigned x = 0;
    unsigned y = 0;
    unsigned own = 0;
    unsigned cult = 0;
    u32 n = 0;
    while (std::fscanf(fp, "%u:%u:%u:%u\n", &x, &y, &own, &cult) == 4) {
        if (x >= state.m_map.width() || y >= state.m_map.height() || own >= state.m_player_n) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (!found_city(state, ux, uy, static_cast<u16>(own))) {
            std::fclose(fp);
            return false;
        }
        City* c = city_at(state, ux, uy);
        if (c == nullptr) {
            std::fclose(fp);
            return false;
        }
        c->set_owner(static_cast<u16>(own));
        c->set_culture(static_cast<u16>(cult > 65535u ? 65535u : cult));
        ++n;
    }
    std::fclose(fp);
    if (talk) {
        std::printf("loaded cities=%u from %s\n", static_cast<unsigned>(n), path);
    }
    return n > 0;
}

static bool run_warmup (GameState& state) {
    GameLoop loop;
    if (!loop.begin(&state, G_TRACE)) {
        return false;
    }
    while (state.m_current_turn < G_TURN_CAP) {
        if (!loop.step()) {
            break;
        }
        std::printf("\rwarmup turn %u / %u", state.m_current_turn, G_TURN_CAP);
        std::fflush(stdout);
    }
    std::printf("\n");
    loop.end();
    return save_ownership(state) && save_cities(state);
}

static u16 count_owner_cities (const GameState& state, u16 owner) {
    u16 n = 0;
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c != nullptr && c->get_owner() == owner) {
            ++n;
        }
    }
    return n;
}

static bool is_coast_wtr (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static bool is_inland_wtr (u8 t) {
    return t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0];
}

static bool land_riv_seed (const GameArraySimple& map, u16 x, u16 y) {
    const u8 t = map.get_terrain(x, y);
    return map.get_river(x, y) != 0u && !is_coast_wtr(t) && !is_inland_wtr(t);
}

static u16 collect_systems (GameState& state, GenWatershed& ws, RivSys* out, u16 cap) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    Whiteboard_1B seen("target_ordering_river", "seen", 0u);
    if (!seen.ok()) {
        return 0;
    }
    std::memset(seen.raw(), 0, static_cast<size_t>(WhiteboardMng::tile_n()));
    u16 n = 0;
    for (u16 y = 0; y < h && n < cap; ++y) {
        for (u16 x = 0; x < w && n < cap; ++x) {
            if (!land_riv_seed(state.m_map, x, y) || seen.rd(x, y) != 0u) {
                continue;
            }
            const u32 rn = ws.fill_rivers(x, y);
            if (rn == 0u) {
                continue;
            }
            const Whiteboard_1B& ov = ws.overlay();
            for (u16 ty = 0; ty < h; ++ty) {
                for (u16 tx = 0; tx < w; ++tx) {
                    if (ov.rd(tx, ty) != 0u) {
                        seen.wr(tx, ty, 1u);
                    }
                }
            }
            out[n].m_sx = x;
            out[n].m_sy = y;
            out[n].m_n = rn;
            ++n;
        }
    }
    for (u16 a = 0; a < n; ++a) {
        for (u16 b = static_cast<u16>(a + 1u); b < n; ++b) {
            if (out[b].m_n > out[a].m_n) {
                const RivSys t = out[a];
                out[a] = out[b];
                out[b] = t;
            }
        }
    }
    return n;
}

static bool seats_on_system (
    GameState& state,
    GenWatershed& ws,
    u16 sx,
    u16 sy,
    u32 riv_n,
    PairRun* out,
    u16* out_n,
    u16 out_cap,
    const PairRun* used,
    u16 used_n) {
    if (ws.fill_rivers(sx, sy) == 0u) {
        return false;
    }
    const Whiteboard_1B& riv = ws.overlay();
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    const u16 cn = state.m_cities.get_city_count();
    static const i8 k_dx9[9] = {0, -1, 0, 1, -1, 1, -1, 0, 1};
    static const i8 k_dy9[9] = {0, -1, -1, -1, 0, 0, 1, 1, 1};
    struct Hit {
        u16 m_own;
        u16 m_x;
        u16 m_y;
        u16 m_idx;
    };
    Hit* hits = new Hit[cn + 1u];
    u16 hit_n = 0;
    Whiteboard_1B taken("target_ordering_river", "hit", 0u);
    if (!taken.ok()) {
        delete[] hits;
        return false;
    }
    std::memset(taken.raw(), 0, static_cast<size_t>(WhiteboardMng::tile_n()));
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (riv.rd(x, y) == 0u) {
                continue;
            }
            for (u8 d = 0; d < 9u; ++d) {
                const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_dx9[d]);
                const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_dy9[d]);
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                if (taken.rd(ux, uy) != 0u || state.m_map.get_add_typ(ux, uy) != BUILD_ADD_CITY) {
                    continue;
                }
                const u16 cidx = state.m_map.get_add_idx(ux, uy);
                if (cidx >= cn) {
                    continue;
                }
                City* c = state.m_cities.get_city(cidx);
                if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
                    continue;
                }
                taken.wr(ux, uy, 1u);
                hits[hit_n].m_own = static_cast<u16>(c->get_owner());
                hits[hit_n].m_x = ux;
                hits[hit_n].m_y = uy;
                hits[hit_n].m_idx = cidx;
                ++hit_n;
            }
        }
    }
    u16* seat_n = new u16[state.m_player_n];
    for (u16 p = 0; p < state.m_player_n; ++p) {
        seat_n[p] = 0;
    }
    for (u16 i = 0; i < hit_n; ++i) {
        ++seat_n[hits[i].m_own];
    }
    for (u16 a = 0; a < state.m_player_n; ++a) {
        if (seat_n[a] == 0) {
            continue;
        }
        for (u16 b = static_cast<u16>(a + 1u); b < state.m_player_n; ++b) {
            if (seat_n[b] == 0) {
                continue;
            }
            u32 best_d = 0xFFFFFFFFu;
            for (u16 i = 0; i < hit_n; ++i) {
                if (hits[i].m_own != a) {
                    continue;
                }
                for (u16 j = 0; j < hit_n; ++j) {
                    if (hits[j].m_own != b) {
                        continue;
                    }
                    const u32 adx = hits[i].m_x > hits[j].m_x
                        ? static_cast<u32>(hits[i].m_x - hits[j].m_x)
                        : static_cast<u32>(hits[j].m_x - hits[i].m_x);
                    const u32 ady = hits[i].m_y > hits[j].m_y
                        ? static_cast<u32>(hits[i].m_y - hits[j].m_y)
                        : static_cast<u32>(hits[j].m_y - hits[i].m_y);
                    const u32 d = adx + ady;
                    if (d < best_d) {
                        best_d = d;
                    }
                }
            }
            if (best_d > G_ADJ_MAX) {
                continue;
            }
            bool dup = false;
            for (u16 u = 0; u < used_n; ++u) {
                if ((used[u].m_pa == a && used[u].m_pb == b) || (used[u].m_pa == b && used[u].m_pb == a)) {
                    dup = true;
                    break;
                }
            }
            for (u16 u = 0; u < *out_n; ++u) {
                if ((out[u].m_pa == a && out[u].m_pb == b) || (out[u].m_pa == b && out[u].m_pb == a)) {
                    dup = true;
                    break;
                }
            }
            if (dup || *out_n >= out_cap) {
                continue;
            }
            PairRun& pr = out[*out_n];
            if (seat_n[a] >= seat_n[b]) {
                pr.m_pa = a;
                pr.m_pb = b;
                pr.m_a_n = seat_n[a];
                pr.m_b_n = seat_n[b];
            } else {
                pr.m_pa = b;
                pr.m_pb = a;
                pr.m_a_n = seat_n[b];
                pr.m_b_n = seat_n[a];
            }
            pr.m_sx = sx;
            pr.m_sy = sy;
            pr.m_riv_n = riv_n;
            pr.m_adj = best_d;
            *out_n = static_cast<u16>(*out_n + 1u);
        }
    }
    delete[] seat_n;
    delete[] hits;
    return *out_n > 0;
}

static u16 pick_pair_runs (GameState& state, GenWatershed& ws, PairRun* out, u16 cap) {
    RivSys* sys = new RivSys[G_SYS_CAP];
    const u16 sn = collect_systems(state, ws, sys, G_SYS_CAP);
    std::printf("river systems=%u\n", static_cast<unsigned>(sn));
    PairRun* cand = new PairRun[2048];
    u16 cn = 0;
    for (u16 i = 0; i < sn && cn < 2048u; ++i) {
        u16 bn = 0;
        PairRun buf[128];
        seats_on_system(state, ws, sys[i].m_sx, sys[i].m_sy, sys[i].m_n, buf, &bn, 128u, cand, cn);
        for (u16 j = 0; j < bn && cn < 2048u; ++j) {
            cand[cn++] = buf[j];
        }
    }
    for (u16 a = 0; a < cn; ++a) {
        for (u16 b = static_cast<u16>(a + 1u); b < cn; ++b) {
            const bool swap = cand[b].m_adj < cand[a].m_adj
                || (cand[b].m_adj == cand[a].m_adj && cand[b].m_riv_n > cand[a].m_riv_n);
            if (swap) {
                const PairRun t = cand[a];
                cand[a] = cand[b];
                cand[b] = t;
            }
        }
    }
    u16 n = 0;
    for (u16 i = 0; i < cn && n < cap; ++i) {
        bool dup = false;
        for (u16 u = 0; u < n; ++u) {
            if ((out[u].m_pa == cand[i].m_pa && out[u].m_pb == cand[i].m_pb)
                || (out[u].m_pa == cand[i].m_pb && out[u].m_pb == cand[i].m_pa)) {
                dup = true;
                break;
            }
        }
        if (dup) {
            continue;
        }
        out[n++] = cand[i];
    }
    delete[] cand;
    delete[] sys;
    return n;
}

static void terr_rgb (u8 terr, u8* r, u8* g, u8* b) {
    static const u8* k_terr[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS, TERR_MOUNTAINS, TERR_VOLCANO,
        TERR_INLAND_SEA, TERR_INLAND_LAKE
    };
    for (u16 i = 0; i < 10u; ++i) {
        if (k_terr[i][0] == terr) {
            *r = k_terr[i][1];
            *g = k_terr[i][2];
            *b = k_terr[i][3];
            return;
        }
    }
    *r = 40;
    *g = 40;
    *b = 40;
}

static const u8* seat_rgb (u8 seat, u8 attacker, u8 defender) {
    if (seat == attacker) {
        return k_own_pal[0];
    }
    if (seat == defender) {
        return k_own_pal[1];
    }
    return k_own_pal[seat % k_own_pal_n];
}

static void blend_own (u8* r, u8* g, u8* b, u8 seat, u8 attacker, u8 defender) {
    if (seat == U8_KEY_NULL) {
        return;
    }
    const u8* c = seat_rgb(seat, attacker, defender);
    *r = static_cast<u8>((static_cast<u16>(*r) + static_cast<u16>(c[0]) * 3u) / 4u);
    *g = static_cast<u8>((static_cast<u16>(*g) + static_cast<u16>(c[1]) * 3u) / 4u);
    *b = static_cast<u8>((static_cast<u16>(*b) + static_cast<u16>(c[2]) * 3u) / 4u);
}

static void paint_dot (std::vector<u8>& rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * w + x) * 3u;
    rgb[i] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static bool crop_box (
    const GameState& state,
    u8 attacker,
    u8 defender,
    const Whiteboard_1B* riv,
    u16* x0,
    u16* y0,
    u16* x1,
    u16* y1) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    u32 mn_x = w;
    u32 mn_y = h;
    u32 mx_x = 0;
    u32 mx_y = 0;
    bool any = false;
    if (riv != nullptr) {
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                if (riv->rd(x, y) == 0u) {
                    continue;
                }
                if (x < mn_x) {
                    mn_x = x;
                }
                if (y < mn_y) {
                    mn_y = y;
                }
                if (x > mx_x) {
                    mx_x = x;
                }
                if (y > mx_y) {
                    mx_y = y;
                }
                any = true;
            }
        }
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 own = c->get_owner();
        if (own != attacker && own != defender) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        if (x >= w || y >= h) {
            continue;
        }
        if (x < mn_x) {
            mn_x = x;
        }
        if (y < mn_y) {
            mn_y = y;
        }
        if (x > mx_x) {
            mx_x = x;
        }
        if (y > mx_y) {
            mx_y = y;
        }
        any = true;
    }
    if (!any) {
        *x0 = 0;
        *y0 = 0;
        *x1 = static_cast<u16>(w - 1u);
        *y1 = static_cast<u16>(h - 1u);
        return false;
    }
    *x0 = static_cast<u16>(mn_x);
    *y0 = static_cast<u16>(mn_y);
    *x1 = static_cast<u16>(mx_x);
    *y1 = static_cast<u16>(mx_y);
    return true;
}

static bool write_owner_ppm (
    cstr path,
    const GameState& state,
    const std::vector<u8>& cap,
    u8 attacker,
    u8 defender,
    const Whiteboard_1B* riv,
    u16 x0,
    u16 y0,
    u16 x1,
    u16 y1) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    if (x1 >= w) {
        x1 = static_cast<u16>(w - 1u);
    }
    if (y1 >= h) {
        y1 = static_cast<u16>(h - 1u);
    }
    if (x0 > x1 || y0 > y1) {
        x0 = 0;
        y0 = 0;
        x1 = static_cast<u16>(w - 1u);
        y1 = static_cast<u16>(h - 1u);
    }
    const u16 cw = static_cast<u16>(x1 - x0 + 1u);
    const u16 ch = static_cast<u16>(y1 - y0 + 1u);
    std::vector<u8> rgb(static_cast<size_t>(cw) * static_cast<size_t>(ch) * 3u);
    const u8* atk = k_own_pal[0];
    for (u16 y = y0; y <= y1; ++y) {
        for (u16 x = x0; x <= x1; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(state.m_map.get_terrain(x, y), &r, &g, &b);
            if (state.m_map.get_river(x, y) != 0u) {
                r = 40;
                g = 90;
                b = 200;
            }
            blend_own(&r, &g, &b, state.m_map.get_civ_owner(x, y), attacker, defender);
            if (riv != nullptr && riv->rd(x, y) != 0u) {
                r = 255;
                g = 255;
                b = 255;
            }
            const u32 i = (static_cast<u32>(y - y0) * cw + static_cast<u32>(x - x0)) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        if (cx < x0 || cy < y0 || cx > x1 || cy > y1) {
            continue;
        }
        const bool taken = i < cap.size() && cap[i] != 0u;
        if (taken) {
            paint_dot(rgb, cw, ch, static_cast<u16>(cx - x0), static_cast<u16>(cy - y0), atk[0], atk[1], atk[2]);
        } else {
            paint_dot(rgb, cw, ch, static_cast<u16>(cx - x0), static_cast<u16>(cy - y0), 0u, 0u, 0u);
        }
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(cw), static_cast<unsigned>(ch));
    const bool ok = std::fwrite(rgb.data(), 1, rgb.size(), fp) == rgb.size();
    std::fclose(fp);
    return ok;
}

static bool run_pair (
    GameState& state,
    const PairRun& pr,
    cstr out_dir,
    cstr prefix,
    cstr own_path,
    cstr cities_path) {
    if (!load_ownership_path(state, own_path) || !load_cities_path(state, cities_path, false)) {
        std::printf("FAIL: reload cache for pair\n");
        return false;
    }
    GenWatershed ws;
    if (!ws.begin(state.m_map) || ws.fill_rivers(pr.m_sx, pr.m_sy) == 0u) {
        std::printf("FAIL: river overlay\n");
        return false;
    }
    Whiteboard_1B riv_keep("target_ordering_river", "keep", 0u);
    if (!riv_keep.ok()) {
        return false;
    }
    std::memcpy(riv_keep.raw(), ws.overlay().raw(), static_cast<size_t>(WhiteboardMng::tile_n()));

    u16 x0 = 0;
    u16 y0 = 0;
    u16 x1 = 0;
    u16 y1 = 0;
    crop_box(state, static_cast<u8>(pr.m_pa), static_cast<u8>(pr.m_pb), &riv_keep, &x0, &y0, &x1, &y1);
    std::printf("  crop=(%u,%u)-(%u,%u)\n",
        static_cast<unsigned>(x0), static_cast<unsigned>(y0),
        static_cast<unsigned>(x1), static_cast<unsigned>(y1));

    u16 tgts[G_TGT_CAP];
    TargetOrderingRiverSystem ord;
    const auto t0 = std::chrono::steady_clock::now();
    const u16 tn = ord.fill(state, pr.m_sx, pr.m_sy, static_cast<u8>(pr.m_pb), tgts, G_TGT_CAP);
    const auto t1 = std::chrono::steady_clock::now();
    const f64 fill_us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
    std::printf("  targets=%u fill_us=%.2f\n", static_cast<unsigned>(tn), fill_us);
    if (tn == 0) {
        return false;
    }
    TargetSortByCore::sort(state, static_cast<u8>(pr.m_pa), tgts, tn);

    std::vector<u8> cap(state.m_cities.get_city_count(), 0u);
    char ppm[420];
    std::snprintf(ppm, sizeof(ppm), "%s/%s_0000.ppm", out_dir, prefix);
    write_owner_ppm(ppm, state, cap, static_cast<u8>(pr.m_pa), static_cast<u8>(pr.m_pb), &riv_keep, x0, y0, x1, y1);

    for (u16 i = 0; i < tn; ++i) {
        const u16 cidx = tgts[i];
        City* c = state.m_cities.get_city(cidx);
        if (c == nullptr || c->get_owner() != pr.m_pb) {
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        const u8 from = static_cast<u8>(pr.m_pb);
        c->set_owner(pr.m_pa);
        const u32 flipped = TileTransfer::apply(state, cidx, from, static_cast<u8>(pr.m_pa), nullptr);
        if (cidx >= cap.size()) {
            cap.resize(static_cast<size_t>(cidx) + 1u, 0u);
        }
        cap[cidx] = 1u;
        std::snprintf(ppm, sizeof(ppm), "%s/%s_%04u.ppm", out_dir, prefix, static_cast<unsigned>(i + 1u));
        write_owner_ppm(ppm, state, cap, static_cast<u8>(pr.m_pa), static_cast<u8>(pr.m_pb), &riv_keep, x0, y0, x1, y1);
        std::printf("  target %u city=(%u,%u) idx=%u tiles=%u\n",
            static_cast<unsigned>(i + 1u),
            static_cast<unsigned>(cx), static_cast<unsigned>(cy),
            static_cast<unsigned>(cidx),
            static_cast<unsigned>(flipped));
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths() || !ensure_out_dir()) {
        std::printf("path/out failed\n");
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
        std::printf("setup_new_game failed\n");
        return 1;
    }
    CityBorder::bind_map(&state.m_map);

    const bool have_cache = file_exists(G_OWN) && file_exists(G_CITIES);
    const bool have_alt = file_exists(G_OWN_ALT) && file_exists(G_CITIES_ALT);
    if (have_cache) {
        std::printf("cache hit: loading ownership + cities\n");
        if (!load_ownership_path(state, G_OWN) || !load_cities_path(state, G_CITIES, true)) {
            std::printf("FAIL: cache load\n");
            state.clear();
            return 1;
        }
    } else if (have_alt) {
        std::printf("cache hit (flood alt): loading ownership + cities\n");
        if (!load_ownership_path(state, G_OWN_ALT) || !load_cities_path(state, G_CITIES_ALT, true)) {
            std::printf("FAIL: alt cache load\n");
            state.clear();
            return 1;
        }
        (void)save_ownership(state);
        (void)save_cities(state);
    } else {
        std::printf("cache miss: running %u turns with %u players\n", G_TURN_CAP, G_PLAYERS);
        if (!run_warmup(state)) {
            std::printf("FAIL: warmup/save\n");
            state.clear();
            return 1;
        }
        std::printf("saved %s and %s\n", G_OWN, G_CITIES);
    }

    WhiteboardMng::init(state.m_map.width(), state.m_map.height());
    GenWatershed ws;
    if (!ws.begin(state.m_map)) {
        std::printf("FAIL: GenWatershed::begin\n");
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }

    cstr own_path = G_OWN;
    cstr cities_path = G_CITIES;
    if (!file_exists(G_OWN) || !file_exists(G_CITIES)) {
        own_path = G_OWN_ALT;
        cities_path = G_CITIES_ALT;
    }

    PairRun pairs[G_PAIR_CAP];
    const u16 pn = pick_pair_runs(state, ws, pairs, G_PAIR_CAP);
    std::printf("pair runs=%u (adj_max=%u)\n", static_cast<unsigned>(pn), static_cast<unsigned>(G_ADJ_MAX));
    if (pn == 0) {
        std::printf("FAIL: no adjacent shared-river pairs\n");
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }

    for (u16 i = 0; i < pn; ++i) {
        const PairRun& pr = pairs[i];
        char prefix[64];
        std::snprintf(prefix, sizeof(prefix), "pair%02u_a%u_b%u",
            static_cast<unsigned>(i + 1u),
            static_cast<unsigned>(pr.m_pa),
            static_cast<unsigned>(pr.m_pb));
        std::printf("pair %u attacker=%u defender=%u adj=%u river_tiles=%u seed=(%u,%u) prefix=%s\n",
            static_cast<unsigned>(i + 1u),
            static_cast<unsigned>(pr.m_pa),
            static_cast<unsigned>(pr.m_pb),
            static_cast<unsigned>(pr.m_adj),
            static_cast<unsigned>(pr.m_riv_n),
            static_cast<unsigned>(pr.m_sx),
            static_cast<unsigned>(pr.m_sy),
            prefix);
        if (!run_pair(state, pr, G_OUT_DIR, prefix, own_path, cities_path)) {
            std::printf("FAIL: pair %u\n", static_cast<unsigned>(i + 1u));
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
    }

    std::printf("done pairs=%u out=%s\n", static_cast<unsigned>(pn), G_OUT_DIR);
    WhiteboardMng::terminate();
    state.clear();
    setup.release_map_gen();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
