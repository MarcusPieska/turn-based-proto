//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "game_state.h"
#include "item_effects.h"
#include "mvt_pts_boost_mng.h"
#include "runtime_static_loader.h"
#include "tech_static_data.h"
#include "tech_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_ld;
static RuntimeStatics* g_st = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool ensure_statics () {
    if (g_st != nullptr) {
        return true;
    }
    if (!g_ld.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_st = &g_ld.statics();
    return g_st != nullptr;
}

static bool tech_has_land_mvt (u16 tech_idx) {
    const TechStaticDataStruct& t = g_st->tech().get_item(TechStaticDataKey::from_raw(tech_idx));
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (t.effects.items[j].type != static_cast<u16>(ItemEffectType::BOOSTER)) {
            continue;
        }
        if (t.effects.items[j].effect.booster.target_id == ItemEffectBoosterType::LAND_MVT_PTS) {
            return true;
        }
    }
    return false;
}

static void clr_seat (PlayerState& ps, u16 sw_n) {
    if (ps.m_small_wonder_city != nullptr) {
        for (u16 i = 0; i < sw_n; ++i) {
            ps.m_small_wonder_city[i] = U16_KEY_NULL;
        }
    }
    if (ps.m_techs_researched != nullptr) {
        ps.m_techs_researched->clear_all();
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!ensure_statics()) {
        std::printf("mvt_pts_boost_mng_tester: load statics failed\n");
        return 1;
    }
    const u16 tech_n = g_st->tech().get_item_count();
    const u16 wonder_n = g_st->wonder().get_item_count();
    const u16 sw_n = g_st->small_wonder().get_item_count();
    if (tech_n == 0u) {
        std::printf("mvt_pts_boost_mng_tester: no techs\n");
        return 1;
    }

    GameState* state = static_cast<GameState*>(std::calloc(1, sizeof(GameState)));
    if (state == nullptr) {
        std::printf("mvt_pts_boost_mng_tester: calloc failed\n");
        return 1;
    }
    state->m_statics = g_st;
    state->m_player_n = 1u;
    state->m_player_states = new PlayerState[1];
    state->m_player_states[0].m_techs_researched = new BitArrayCL(tech_n);
    state->m_small_wonder_count = sw_n;
    if (sw_n != 0u) {
        state->m_player_states[0].m_small_wonder_city = new u16[sw_n];
    }
    state->m_wonder_count = wonder_n;
    if (wonder_n != 0u) {
        state->m_wonder_city = new u16[wonder_n];
        for (u16 i = 0; i < wonder_n; ++i) {
            state->m_wonder_city[i] = U16_KEY_NULL;
        }
    }
    clr_seat(state->m_player_states[0], sw_n);

    if (!MvtPtsBoostMng::setup(1u)) {
        std::printf("mvt_pts_boost_mng_tester: setup failed\n");
        return 1;
    }
    MvtPtsBoostMng::refresh(*state);
    u16 prev = MvtPtsBoostMng::land(0u);
    const u16 base = g_st->config().get_mov_pt_per_turn();
    std::printf("base_path_mp_turn=%u land_boost_start=%u\n", base, prev);

    u16 hit_n = 0u;
    for (u16 ti = 0; ti < tech_n; ++ti) {
        state->m_player_states[0].m_techs_researched->set_bit(ti);
        MvtPtsBoostMng::refresh(*state);
        const u16 cur = MvtPtsBoostMng::land(0u);
        if (cur != prev) {
            cstr nm = g_st->tech().get_name(TechStaticDataKey::from_raw(ti));
            const u16 sample_pts = 1u;
            const u16 total = static_cast<u16>(sample_pts * base + cur);
            std::printf("tech=%u name=%s land_boost=%u total_mp_for_1pt_unit=%u has_land_fx=%u\n",
                ti, nm != nullptr ? nm : "?", cur, total, tech_has_land_mvt(ti) ? 1u : 0u);
            prev = cur;
            hit_n = static_cast<u16>(hit_n + 1u);
        }
    }

    std::printf("mvt_pts_boost_mng_tester ok hits=%u final_land_boost=%u\n", hit_n, MvtPtsBoostMng::land(0u));
    MvtPtsBoostMng::clear();
    delete state->m_player_states[0].m_techs_researched;
    delete state->m_player_states[0].m_civics_adopted;
    delete[] state->m_player_states[0].m_small_wonder_city;
    delete[] state->m_player_states;
    delete[] state->m_wonder_city;
    std::free(state);
    return hit_n > 0u ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
