//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "bit_array.h"
#include "game_setup.h"
#include "game_state.h"
#include "resource_effector.h"
#include "resource_ledger.h"
#include "resource_static_key.h"
#include "resource_turn_handler.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 1u;
static const u16 G_LOOPS = 5u;

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

static u32 expect_sum_after_loops (const GameState& state, u16 loops) {
    const u16 res_n = state.m_player_states[0].m_res_ledger.count();
    u32* counts = new u32[res_n]();
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 res = state.m_map.get_res(x, y);
            if (res == U16_KEY_NULL || res >= res_n) {
                continue;
            }
            if (state.m_map.get_civ_owner(x, y) != 0u) {
                continue;
            }
            counts[res] = static_cast<u32>(counts[res] + 1u);
        }
    }
    u32 sum = 0;
    for (u16 i = 0; i < res_n; ++i) {
        const u32 raw = counts[i] * static_cast<u32>(loops);
        const u32 capped = raw > ResourceLedger::CAP ? ResourceLedger::CAP : raw;
        sum = static_cast<u32>(sum + capped);
    }
    delete[] counts;
    return sum;
}

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

    const u32 coords = ResourceTurnHandler::coord_n();
    if (coords == 0) {
        std::printf("FAIL: no resource coords after setup\n");
        state.clear();
        return 1;
    }

    claim_whole_map(state, 0);
    unlock_all_tech(state);
    std::printf("resource_effector any_n=%u entry_n=%u res_n=%u\n",
        static_cast<unsigned>(ResourceEffector::any_n()),
        static_cast<unsigned>(ResourceEffector::entry_n()),
        static_cast<unsigned>(ResourceEffector::res_n()));
    if (ResourceEffector::any_n() == 0 && ResourceEffector::res_n() == 0) {
        std::printf("FAIL: ResourceEffector not set up\n");
        state.clear();
        return 1;
    }
    ResourceLedger& led = state.m_player_states[0].m_res_ledger;
    const u32 before = ledger_sum(led);
    u32 prev = before;
    f64 total_ms = 0.0;
    for (u16 t = 0; t < G_LOOPS; ++t) {
        const auto t0 = std::chrono::steady_clock::now();
        ResourceTurnHandler::handle(state);
        const auto t1 = std::chrono::steady_clock::now();
        const f64 ms = std::chrono::duration<f64, std::milli>(t1 - t0).count();
        total_ms += ms;
        const u32 now = ledger_sum(led);
        std::printf("loop %u ledger_sum=%u handle_ms=%.3f\n",
            static_cast<unsigned>(t + 1u), static_cast<unsigned>(now), ms);
        if (t == 0 && now <= prev) {
            std::printf("FAIL: ledger did not increase on first loop\n");
            state.clear();
            return 1;
        }
        prev = now;
    }
    const u32 after = ledger_sum(led);
    const u32 expect = expect_sum_after_loops(state, G_LOOPS);
    std::printf("coords=%u before=%u after=%u expect=%u handle_ms_avg=%.3f handle_ms_tot=%.3f\n",
        static_cast<unsigned>(coords),
        static_cast<unsigned>(before),
        static_cast<unsigned>(after),
        static_cast<unsigned>(expect),
        total_ms / static_cast<f64>(G_LOOPS),
        total_ms);

    const RuntimeStatics& st = *state.m_statics;
    const u16 res_n = led.count();
    for (u16 i = 0; i < res_n; ++i) {
        const u16 v = led.get(i);
        if (v == 0) {
            continue;
        }
        if (v > ResourceLedger::CAP) {
            std::printf("FAIL: stock above CAP\n");
            state.clear();
            return 1;
        }
        const char* nm = st.resource().get_name(ResourceStaticDataKey::from_raw(i));
        if (nm == nullptr) {
            nm = "?";
        }
        std::printf("  %s=%u\n", nm, static_cast<unsigned>(v));
    }

    if (after != expect) {
        std::printf("FAIL: ledger sum mismatch vs capped expectation\n");
        state.clear();
        return 1;
    }
    if (after <= before) {
        std::printf("FAIL: ledger did not increase\n");
        state.clear();
        return 1;
    }

    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
