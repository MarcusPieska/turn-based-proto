//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "bit_array.h"
#include "game_array_simple.h"
#include "game_setup.h"
#include "game_state.h"
#include "map_overlay_enum.h"
#include "resource_ledger.h"
#include "resource_turn_handler.h"
#include "runtime_statics.h"
#include "tile_imp_helper.h"
#include "worker_job_enum.h"
#include "worker_job_imp_static_key.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 1u;

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

static void claim_whole_map (GameState& state, u8 owner) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            state.m_map.set_civ_owner(x, y, owner);
        }
    }
}

static void unlock_all_tech (GameState& state) {
    if (state.m_statics == nullptr || state.m_player_states == nullptr) {
        return;
    }
    const u16 tn = state.m_statics->tech().get_item_count();
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        if (ps.m_techs_researched == nullptr) {
            ps.m_techs_researched = new BitArrayCL(tn);
        }
        for (u16 i = 0; i < tn; ++i) {
            ps.m_techs_researched->set_bit(i);
        }
    }
}

static u32 ledger_sum (const ResourceLedger& led) {
    u32 sum = 0;
    const u16 n = led.count();
    for (u16 i = 0; i < n; ++i) {
        sum = static_cast<u32>(sum + led.get(i));
    }
    return sum;
}

static bool reset_ledger (ResourceLedger& led) {
    const u16 n = led.count();
    return led.setup(n);
}

struct Loc {
    u16 m_x;
    u16 m_y;
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("path build failed\n");
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
    if (state.m_statics == nullptr || state.m_player_states == nullptr || state.m_player_n == 0) {
        std::printf("state incomplete\n");
        state.clear();
        return 1;
    }

    claim_whole_map(state, 0);
    unlock_all_tech(state);

    const RuntimeStatics& st = *state.m_statics;
    const u16 ov = static_cast<u16>(G_OV);
    const u16 job = static_cast<u16>(G_JOB);
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    const u16 res_n = st.resource().get_item_count();

    u32 loc_cap = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 res = state.m_map.get_res(x, y);
            if (res == U16_KEY_NULL || res >= res_n) {
                continue;
            }
            if (st.resource().get_item(ResourceStaticDataKey::from_raw(res)).worker_job_idx != job) {
                continue;
            }
            ++loc_cap;
        }
    }
    if (loc_cap == 0) {
        std::printf("FAIL: no %s resource tiles\n", G_LABEL);
        state.clear();
        return 1;
    }

    Loc* locs = new Loc[loc_cap];
    u32 loc_n = 0;
    u32 stamp_fail = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 res = state.m_map.get_res(x, y);
            if (res == U16_KEY_NULL || res >= res_n) {
                continue;
            }
            if (st.resource().get_item(ResourceStaticDataKey::from_raw(res)).worker_job_idx != job) {
                continue;
            }
            if (!state.m_map.set_overlay(x, y, ov)) {
                ++stamp_fail;
                continue;
            }
            locs[loc_n].m_x = x;
            locs[loc_n].m_y = y;
            ++loc_n;
        }
    }
    if (loc_n == 0) {
        std::printf("FAIL: overlay stamp failed on all %s tiles (fail=%u)\n", G_LABEL,
            static_cast<unsigned>(stamp_fail));
        delete[] locs;
        state.clear();
        return 1;
    }

    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    const u16 imp_n = ix.imp_n(ov);
    const u16* imps = ix.imps(ov);
    if (imp_n == 0 || imps == nullptr) {
        std::printf("FAIL: no %s improvements in index\n", G_LABEL);
        delete[] locs;
        state.clear();
        return 1;
    }

    ResourceLedger& led = state.m_player_states[0].m_res_ledger;
    std::printf("%s loc_n=%u stamp_fail=%u imp_n=%u coords=%u\n",
        G_LABEL,
        static_cast<unsigned>(loc_n),
        static_cast<unsigned>(stamp_fail),
        static_cast<unsigned>(imp_n),
        static_cast<unsigned>(ResourceTurnHandler::coord_n()));

    u32 prev = 0;
    f64 total_ms = 0.0;
    u16 boost_n = 0;
    const u16 turns = static_cast<u16>(imp_n + 1u);
    for (u16 t = 0; t < turns; ++t) {
        if (t > 0) {
            const u16 imp_idx = imps[t - 1u];
            u32 set_ok = 0;
            for (u32 i = 0; i < loc_n; ++i) {
                GameTileSimple* tile = state.m_map.tile(locs[i].m_x, locs[i].m_y);
                if (tile != nullptr && TileImpHelper::set_imp(tile, st, imp_idx)) {
                    ++set_ok;
                }
            }
            if (set_ok == 0) {
                std::printf("FAIL: set_imp applied nowhere\n");
                delete[] locs;
                state.clear();
                return 1;
            }
        }
        if (!reset_ledger(led)) {
            std::printf("FAIL: ledger reset\n");
            delete[] locs;
            state.clear();
            return 1;
        }
        const auto t0 = std::chrono::steady_clock::now();
        ResourceTurnHandler::handle(state);
        const auto t1 = std::chrono::steady_clock::now();
        const f64 ms = std::chrono::duration<f64, std::milli>(t1 - t0).count();
        total_ms += ms;
        const u32 now = ledger_sum(led);
        const i32 inc = (t == 0) ? 0 : static_cast<i32>(now) - static_cast<i32>(prev);
        const char* nm = "baseline";
        if (t > 0) {
            nm = st.worker_job_imp().get_name(WorkerJobImpStaticDataKey::from_raw(imps[t - 1u]));
            if (nm == nullptr) {
                nm = "?";
            }
        }
        std::printf("turn %u %s ledger_sum=%u ", static_cast<unsigned>(t), nm, static_cast<unsigned>(now));
        if (inc > 0) {
            const f64 per = static_cast<f64>(inc) / static_cast<f64>(loc_n);
            std::printf("\033[32mincrement=+%d\033[0m ~%.3f/tile handle_ms=%.3f\n",
                static_cast<int>(inc), per, ms);
            ++boost_n;
        } else {
            std::printf("increment=%d handle_ms=%.3f\n", static_cast<int>(inc), ms);
        }
        if (t == 0 && now == 0) {
            std::printf("FAIL: baseline extract is zero\n");
            delete[] locs;
            state.clear();
            return 1;
        }
        prev = now;
    }

    std::printf("%s inferred_boost_imps=%u/%u handle_ms_avg=%.3f handle_ms_tot=%.3f\n",
        G_LABEL,
        static_cast<unsigned>(boost_n),
        static_cast<unsigned>(imp_n),
        total_ms / static_cast<f64>(turns),
        total_ms);
    if (boost_n == 0) {
        std::printf("FAIL: no %s improvement changed resource yield\n", G_LABEL);
        delete[] locs;
        state.clear();
        return 1;
    }

    delete[] locs;
    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
